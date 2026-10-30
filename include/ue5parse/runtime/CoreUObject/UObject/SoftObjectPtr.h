#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtr.h"
#include "ue5parse/runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class UClass;

// FTopLevelAssetPath: "/Game/A/B" and "B".
struct FTopLevelAssetPath {
    FName PackageName;
    FName AssetName;

    bool IsNull() const { return PackageName.isNone(); }
    bool operator==(const FTopLevelAssetPath& Other) const = default;
};

// FSoftObjectPath: an asset path and a sub-object path ("/Game/A/B.B:Sub.Object").
struct FSoftObjectPath {
    FSoftObjectPath() = default;
    // Parses "/Game/A/B.B" or "/Game/A/B.B:Sub"; a string that is no path gives the null path.
    explicit FSoftObjectPath(const FString& Path) { SetPath(Path); }
    FSoftObjectPath(FTopLevelAssetPath AssetPath, FString SubPathString)
        : AssetPath(AssetPath), SubPathString(std::move(SubPathString)) {}
    explicit FSoftObjectPath(const UObject* Object);

    void SetPath(const FString& Path);
    FString ToString() const;
    bool IsNull() const { return AssetPath.IsNull(); }
    bool IsValid() const { return !IsNull(); }
    void Reset() {
        AssetPath = FTopLevelAssetPath();
        SubPathString.Empty();
    }
    // The loaded object, without loading.
    UObject* ResolveObject() const;
    // Loads the object (and its package) when needed.
    UObject* TryLoad() const;
    bool operator==(const FSoftObjectPath& Other) const {
        return AssetPath == Other.AssetPath && SubPathString.Equals(Other.SubPathString);
    }

    FTopLevelAssetPath AssetPath;
    FString SubPathString;
};

uint32 GetTypeHash(const FSoftObjectPath& Path);

// FSoftClassPath: a soft path to a class (UE's SoftObjectPath.h).
struct FSoftClassPath : FSoftObjectPath {
    using FSoftObjectPath::FSoftObjectPath;
};

// FSoftObjectPtr: a soft path and a weak pointer to the object once it is resolved.
struct FSoftObjectPtr {
    FSoftObjectPtr() = default;
    FSoftObjectPtr(const FSoftObjectPath& Path) : ObjectID(Path) {}
    explicit FSoftObjectPtr(const UObject* Object) : WeakPtr(Object), ObjectID(Object) {}

    UObject* Get() const;
    UObject* LoadSynchronous() const;
    const FSoftObjectPath& GetUniqueID() const { return ObjectID; }
    FSoftObjectPath& GetUniqueID() { return ObjectID; }
    bool IsNull() const { return ObjectID.IsNull(); }
    bool IsValid() const { return Get() != nullptr; }
    bool IsPending() const { return Get() == nullptr && !IsNull(); }
    void Reset() {
        WeakPtr.Reset();
        ObjectID.Reset();
    }
    bool operator==(const FSoftObjectPtr& Other) const { return ObjectID == Other.ObjectID; }

    mutable FWeakObjectPtr WeakPtr;
    FSoftObjectPath ObjectID;
};

}

namespace ue5parse::runtime {

// TSoftObjectPtr / TSoftClassPtr: the typed FSoftObjectPtr, same memory (a reflected FSoftObjectProperty or
// FSoftClassProperty).
template <class T>
class TSoftObjectPtr {
public:
    TSoftObjectPtr() = default;
    TSoftObjectPtr(const FSoftObjectPath& Path) : SoftObjectPtr(Path) {}
    explicit TSoftObjectPtr(const T* Object) : SoftObjectPtr(reinterpret_cast<const UObject*>(Object)) {}

    T* Get() const { return static_cast<T*>(CastToT(SoftObjectPtr.Get())); }
    T* LoadSynchronous() const { return static_cast<T*>(CastToT(SoftObjectPtr.LoadSynchronous())); }
    const FSoftObjectPath& ToSoftObjectPath() const { return SoftObjectPtr.GetUniqueID(); }
    FString ToString() const { return ToSoftObjectPath().ToString(); }
    bool IsNull() const { return SoftObjectPtr.IsNull(); }
    bool IsValid() const { return Get() != nullptr; }
    bool IsPending() const { return SoftObjectPtr.IsPending(); }
    void Reset() { SoftObjectPtr.Reset(); }
    bool operator==(const TSoftObjectPtr& Other) const { return SoftObjectPtr == Other.SoftObjectPtr; }

private:
    static UObject* CastToT(UObject* Object);
    FSoftObjectPtr SoftObjectPtr;
};

template <class T>
class TSoftClassPtr {
public:
    TSoftClassPtr() = default;
    TSoftClassPtr(const FSoftObjectPath& Path) : SoftObjectPtr(Path) {}

    UClass* Get() const { return CastToClass(SoftObjectPtr.Get()); }
    UClass* LoadSynchronous() const { return CastToClass(SoftObjectPtr.LoadSynchronous()); }
    const FSoftObjectPath& ToSoftObjectPath() const { return SoftObjectPtr.GetUniqueID(); }
    FString ToString() const { return ToSoftObjectPath().ToString(); }
    bool IsNull() const { return SoftObjectPtr.IsNull(); }
    bool IsValid() const { return Get() != nullptr; }
    void Reset() { SoftObjectPtr.Reset(); }
    bool operator==(const TSoftClassPtr& Other) const { return SoftObjectPtr == Other.SoftObjectPtr; }

private:
    static UClass* CastToClass(UObject* Object);
    FSoftObjectPtr SoftObjectPtr;
};

}
