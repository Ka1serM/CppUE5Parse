#pragma once

#include <variant>

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

struct FMovieSceneTimeWarpVariant {
    std::variant<double, UObject*, FInstancedStruct> Value{1.0};

    bool operator==(const FMovieSceneTimeWarpVariant&) const = default;

    bool Serialize(FArchive& Ar) {
        bool IsLiteral = true;
        Ar << IsLiteral;
        if (Ar.IsError())
            return true;
        if (IsLiteral)
            return ReadLiteral<double>(Ar);

        uint8 Type = 0;
        Ar << Type;
        if (Ar.IsError())
            return true;
        if (Type == 1) {
            Ar << Value.emplace<UObject*>(nullptr);
            return true;
        }
        static constexpr FName PayloadNames[] = {
            "MovieSceneTimeWarpFixedFrame", "FrameRate", "MovieSceneTimeWarpLoop",
            "MovieSceneTimeWarpClamp", "MovieSceneTimeWarpLoopFloat", "MovieSceneTimeWarpClampFloat",
        };
        if (Type < 2 || Type >= 2 + std::size(PayloadNames)) {
            Ar.SetError(ErrorCode::InvalidData, "MovieSceneTimeWarpVariant.Type");
            return true;
        }
        const UScriptStruct* Struct = FindNativeStruct(PayloadNames[Type - 2]);
        if (Struct == nullptr) {
            Ar.SetError(ErrorCode::MissingSchema, "MovieSceneTimeWarpVariant.Payload");
            return true;
        }
        FInstancedStruct& Payload = Value.emplace<FInstancedStruct>();
        Payload.InitializeAs(Struct);
        Struct->SerializeItem(Ar, Payload.GetMutableMemory());
        return true;
    }

    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
        if (Tag.Type == FName("DoubleProperty"))
            return ReadLiteral<double>(Ar);
        if (Tag.Type == FName("FloatProperty"))
            return ReadLiteral<float>(Ar);
        if (Tag.Type == FName("ByteProperty") || Tag.Type == FName("Int32Property") || Tag.Type == FName("IntProperty"))
            return ReadLiteral<int32>(Ar);
        if (Tag.Type == FName("Int16Property"))
            return ReadLiteral<int16>(Ar);
        if (Tag.Type == FName("Int8Property"))
            return ReadLiteral<int8>(Ar);
        if (Tag.Type == FName("UInt32Property"))
            return ReadLiteral<uint32>(Ar);
        if (Tag.Type == FName("UInt16Property"))
            return ReadLiteral<uint16>(Ar);
        return false;
    }

private:
    template <class T>
    bool ReadLiteral(FArchive& Ar) {
        T Literal{};
        Ar << Literal;
        Value = static_cast<double>(Literal);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMovieSceneTimeWarpVariant> : TStructOpsTypeTraitsBase2<FMovieSceneTimeWarpVariant> {
    enum { WithSerializer = true, WithSerializeFromMismatchedTag = true, WithIdenticalViaEquality = true };
};

}
