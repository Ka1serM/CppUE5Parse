#pragma once

#include <cstring>
#include <span>
#include <type_traits>

#include "CppUE5Parse/Core/ByteReader.h"
#include "CppUE5Parse/Core/FGuid.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class UObject;
class FObjectPtr;
class FText;
struct FSoftObjectPath;
struct FSoftObjectPtr;
struct FWeakObjectPtr;
class FLinkerLoad;

using FGuid = ue5parse::FGuid;

class FArchive {
public:
    FArchive() = default;
    virtual ~FArchive() = default;
    FArchive(const FArchive&) = delete;
    FArchive& operator=(const FArchive&) = delete;

    virtual void Serialize(void* Data, int64 Length) = 0;
    virtual int64 Tell() const = 0;
    virtual int64 TotalSize() const = 0;
    virtual void Seek(int64 InPos) = 0;
    bool AtEnd() const { return Tell() >= TotalSize(); }
    int64 GetRemaining() const { return TotalSize() - Tell(); }

    virtual FArchive& operator<<(FName& Value);
    virtual FArchive& operator<<(UObject*& Value);
    virtual FArchive& operator<<(FObjectPtr& Value);
    virtual FArchive& operator<<(FSoftObjectPath& Value);
    virtual FArchive& operator<<(FSoftObjectPtr& Value);
    virtual FArchive& operator<<(FWeakObjectPtr& Value);
    virtual FLinkerLoad* GetLinker() { return nullptr; }
    virtual std::span<const std::byte> ReadView(int64 Length) {
        SetError(ErrorCode::InvalidData, "FArchive.ReadView");
        return {};
    }

    bool UseUnversionedPropertySerialization() const { return bUnversioned; }
    EGame GetGame() const { return Game; }
    virtual int64 GetCookedPosition() const { return Tell(); }
    virtual int64 GetExportChunkPosition() const { return -1; }

    void SetUEVer(EUnrealEngineObjectUE5Version InVersion) { ObjectVersionUE5 = InVersion; }
    void SetUseUnversionedPropertySerialization(bool bInUnversioned) { bUnversioned = bInUnversioned; }
    void SetGame(EGame InGame) { Game = InGame; }
    void CopyStateFrom(const FArchive& Other) {
        ObjectVersionUE5 = Other.ObjectVersionUE5;
        bUnversioned = Other.bUnversioned;
        Game = Other.Game;
    }

    bool IsError() const { return bError; }
    const Error& GetError() const { return ErrorInfo; }
    void SetError(ErrorCode Code, const char* Where) {
        if (bError)
            return;
        bError = true;
        ErrorInfo = Error{Code, Where, static_cast<uint64>(Tell())};
    }

    int32 ReadCount(int32 MinElementSize = 1) {
        int32 Count = 0;
        Serialize(&Count, sizeof(Count));
        if (IsError())
            return 0;
        if (Count < 0) {
            SetError(ErrorCode::InvalidData, "FArchive.ReadCount");
            return 0;
        }
        if (MinElementSize > 0 && static_cast<int64>(Count) > GetRemaining() / MinElementSize) {
            SetError(ErrorCode::ReadPastEnd, "FArchive.ReadCount");
            return 0;
        }
        return Count;
    }
    FString ReadUtf8String();
    void SkipBytes(int64 Count) {
        if (Count < 0 || Count > GetRemaining()) {
            SetError(ErrorCode::ReadPastEnd, "FArchive.SkipBytes");
            return;
        }
        Seek(Tell() + Count);
    }
    template <class T>
        requires std::is_trivially_copyable_v<T>
    T Read() {
        T Value{};
        Serialize(&Value, sizeof(T));
        return Value;
    }

private:
    EUnrealEngineObjectUE5Version ObjectVersionUE5 = EUnrealEngineObjectUE5Version::IMPORT_TYPE_HIERARCHIES;
    bool bUnversioned = true;
    EGame Game = EGame::GAME_UE5_LATEST;
    bool bError = false;
    Error ErrorInfo{};
};

class FMemoryReaderView : public FArchive {
public:
    explicit FMemoryReaderView(std::span<const std::byte> InBytes) : Bytes(InBytes) {}

    void Serialize(void* Data, int64 Length) override {
        if (Length == 0)
            return;
        if (IsError() || Length < 0 || static_cast<uint64>(Length) > Bytes.size() - Offset) {
            std::memset(Data, 0, Length > 0 ? static_cast<std::size_t>(Length) : 0);
            SetError(ErrorCode::ReadPastEnd, "FMemoryReaderView.Serialize");
            return;
        }
        std::memcpy(Data, Bytes.data() + Offset, static_cast<std::size_t>(Length));
        Offset += static_cast<std::size_t>(Length);
    }
    int64 Tell() const override { return static_cast<int64>(Offset); }
    int64 TotalSize() const override { return static_cast<int64>(Bytes.size()); }
    void Seek(int64 InPos) override {
        if (InPos < 0 || static_cast<uint64>(InPos) > Bytes.size()) {
            SetError(ErrorCode::ReadPastEnd, "FMemoryReaderView.Seek");
            return;
        }
        Offset = static_cast<std::size_t>(InPos);
    }
    std::span<const std::byte> ReadView(int64 Length) override {
        if (IsError() || Length < 0 || static_cast<uint64>(Length) > Bytes.size() - Offset) {
            SetError(ErrorCode::ReadPastEnd, "FMemoryReaderView.ReadView");
            return {};
        }
        std::span<const std::byte> View = Bytes.subspan(Offset, static_cast<std::size_t>(Length));
        Offset += static_cast<std::size_t>(Length);
        return View;
    }

private:
    std::span<const std::byte> Bytes;
    std::size_t Offset = 0;
};

template <class T>
    requires(std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) || std::is_enum_v<T>
inline FArchive& operator<<(FArchive& Ar, T& Value) {
    Ar.Serialize(&Value, sizeof(T));
    return Ar;
}
inline FArchive& operator<<(FArchive& Ar, bool& Value) {
    uint32 Stored = 0;
    Ar.Serialize(&Stored, sizeof(Stored));
    if (Stored > 1)
        Ar.SetError(ErrorCode::InvalidData, "FArchive.bool");
    Value = Stored != 0;
    return Ar;
}
FArchive& operator<<(FArchive& Ar, FString& Value);
template <class TEnum>
FArchive& operator<<(FArchive& Ar, TEnumAsByte<TEnum>& Value) {
    uint8 Byte = Value.GetIntValue();
    Ar.Serialize(&Byte, 1);
    Value = TEnumAsByte<TEnum>(Byte);
    return Ar;
}
inline FArchive& operator<<(FArchive& Ar, FGuid& Value) {
    return Ar << Value.a << Value.b << Value.c << Value.d;
}

template <class T>
FArchive& operator<<(FArchive& Ar, TVector<T>& V) {
    return Ar << V.X << V.Y << V.Z;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TVector2<T>& V) {
    return Ar << V.X << V.Y;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TVector4<T>& V) {
    return Ar << V.X << V.Y << V.Z << V.W;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TRotator<T>& R) {
    return Ar << R.Pitch << R.Yaw << R.Roll;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TQuat<T>& Q) {
    return Ar << Q.X << Q.Y << Q.Z << Q.W;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TTransform<T>& Transform) {
    return Ar << Transform.Rotation << Transform.Translation << Transform.Scale3D;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TPlane<T>& P) {
    return Ar << P.X << P.Y << P.Z << P.W;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TMatrix<T>& M) {
    for (int32 i = 0; i < 4; i++)
        for (int32 j = 0; j < 4; j++)
            Ar << M.M[i][j];
    return Ar;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TBox<T>& Box) {
    return Ar << Box.Min << Box.Max << Box.IsValid;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TBox2<T>& Box) {
    uint8 Valid = 0;
    Ar << Box.Min << Box.Max << Valid;
    Box.bIsValid = Valid != 0;
    return Ar;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TSphere<T>& Sphere) {
    return Ar << Sphere.Center << Sphere.W;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TBoxSphereBounds<T>& Bounds) {
    return Ar << Bounds.Origin << Bounds.BoxExtent << Bounds.SphereRadius;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TIntPoint<T>& P) {
    return Ar << P.X << P.Y;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TIntVector2<T>& V) {
    return Ar << V.X << V.Y;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TIntVector3<T>& V) {
    return Ar << V.X << V.Y << V.Z;
}
template <class T>
FArchive& operator<<(FArchive& Ar, TIntVector4<T>& V) {
    return Ar << V.X << V.Y << V.Z << V.W;
}
inline FArchive& operator<<(FArchive& Ar, FColor& C) {
    Ar.Serialize(&C, sizeof(FColor));
    return Ar;
}
inline FArchive& operator<<(FArchive& Ar, FLinearColor& C) {
    return Ar << C.R << C.G << C.B << C.A;
}

template <class T>
FArchive& operator<<(FArchive& Ar, TArray<T>& Array) {
    int32 Count = Ar.ReadCount(1);
    Array.Empty(Count);
    if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
        if constexpr (!std::is_same_v<T, bool>) {
            if (Count > 0 && static_cast<int64>(Count) * static_cast<int64>(sizeof(T)) > Ar.GetRemaining()) {
                Ar.SetError(ErrorCode::ReadPastEnd, "FArchive.TArray");
                return Ar;
            }
            Array.AddUninitialized(Count);
            Ar.Serialize(Array.GetData(), static_cast<int64>(Count) * sizeof(T));
            return Ar;
        }
    }
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        T& Element = Array[Array.AddDefaulted()];
        Ar << Element;
    }
    if (Ar.IsError())
        Array.Empty();
    return Ar;
}

template <class T>
    requires std::is_trivially_copyable_v<T>
void BulkSerialize(FArchive& Ar, TArray<T>& Array) {
    int32 SerializedElementSize = 0;
    Ar << SerializedElementSize;
    if (!Ar.IsError() && SerializedElementSize != static_cast<int32>(sizeof(T))) {
        Ar.SetError(ErrorCode::InvalidData, "BulkSerialize.ElementSize");
        return;
    }
    int32 Count = Ar.ReadCount(static_cast<int32>(sizeof(T)));
    Array.Empty(Count);
    if (Count == 0)
        return;
    Array.AddUninitialized(Count);
    Ar.Serialize(Array.GetData(), static_cast<int64>(Count) * sizeof(T));
    if (Ar.IsError())
        Array.Empty();
}

template <class T>
    requires std::is_trivially_copyable_v<T>
void SerializeRaw(FArchive& Ar, TArray<T>& Array, int32 Count) {
    Array.Empty(Count);
    if (Count <= 0)
        return;
    if (static_cast<int64>(Count) * static_cast<int64>(sizeof(T)) > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "SerializeRaw");
        return;
    }
    Array.AddUninitialized(Count);
    Ar.Serialize(Array.GetData(), static_cast<int64>(Count) * sizeof(T));
}

}
