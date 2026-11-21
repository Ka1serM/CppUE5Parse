#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

class FUniversalObjectLocatorFragment {
public:
    const UScriptStruct* GetFragmentStruct() const { return Payload.GetScriptStruct(); }
    const void* GetPayload() const { return Payload.GetMemory(); }
    bool operator==(const FUniversalObjectLocatorFragment&) const = default;

    bool Serialize(FArchive& Ar) {
        FName FragmentTypeID;
        Ar << FragmentTypeID;
        if (Ar.IsError())
            return true;
        if (FragmentTypeID.isNone()) {
            Payload.Reset();
            return true;
        }

        static constexpr struct {
            FName ID;
            FName PayloadName;
        } Types[] = {
            {"actor", "ActorLocatorFragment"},
            {"asset", "AssetLocatorFragment"},
            {"animinst", "AnimInstanceLocatorFragment"},
            {"subobj", "SubObjectLocator"},
            {"uobj", "DirectPathObjectLocator"},
            {"ls_lazy_obj_ptr", "LegacyLazyObjectPtrFragment"},
        };
        for (const auto& Type : Types) {
            if (Type.ID != FragmentTypeID)
                continue;
            const UScriptStruct* Struct = FindNativeStruct(Type.PayloadName);
            if (Struct == nullptr) {
                Ar.SetError(ErrorCode::MissingSchema, "UniversalObjectLocatorFragment.Payload");
                return true;
            }
            Payload.InitializeAs(Struct);
            Struct->SerializeItem(Ar, Payload.GetMutableMemory());
            return true;
        }
        Ar.SetError(ErrorCode::MissingSchema, "UniversalObjectLocatorFragment.FragmentTypeID");
        return true;
    }

private:
    FInstancedStruct Payload;
};

template <>
struct TStructOpsTypeTraits<FUniversalObjectLocatorFragment> : TStructOpsTypeTraitsBase2<FUniversalObjectLocatorFragment> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
