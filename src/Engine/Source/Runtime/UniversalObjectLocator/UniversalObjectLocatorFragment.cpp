#include "Engine/Source/Runtime/UniversalObjectLocator/UniversalObjectLocatorFragment.h"

#include <array>
#include <utility>

namespace ue5parse::runtime {

namespace {

//! The fragment types that the engine, UniversalObjectLocator and LevelSequence modules register at startup.
constexpr std::array<std::pair<const char*, const char*>, 6> FragmentTypes{{
    {"actor", "ActorLocatorFragment"},
    {"asset", "AssetLocatorFragment"},
    {"animinst", "AnimInstanceLocatorFragment"},
    {"subobj", "SubObjectLocator"},
    {"uobj", "DirectPathObjectLocator"},
    {"ls_lazy_obj_ptr", "LegacyLazyObjectPtrFragment"},
}};

const UScriptStruct* FindPayloadStruct(FName FragmentTypeID) {
    for (const auto& [ID, StructName] : FragmentTypes)
        if (FragmentTypeID == FName(ID))
            return FindNativeStruct(FName(StructName));
    return FUniversalObjectLocatorEmptyPayload::StaticStruct();
}

}

bool FUniversalObjectLocatorFragment::Serialize(FArchive& Ar) {
    Ar << FragmentTypeID;
    Payload.Reset();
    if (FragmentTypeID.isNone())
        return true;
    Payload.InitializeAs(FindPayloadStruct(FragmentTypeID));
    Payload.GetScriptStruct()->SerializeItem(Ar, Payload.GetMutableMemory());
    return true;
}

}
