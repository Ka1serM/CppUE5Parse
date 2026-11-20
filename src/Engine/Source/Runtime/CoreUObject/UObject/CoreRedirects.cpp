#include "Engine/Source/Runtime/CoreUObject/UObject/CoreRedirects.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include <algorithm>
#include <array>
#include <utility>

namespace ue5parse::runtime {

namespace {

//! The engine reads these from the ClassRedirects of BaseEngine.ini.
constexpr std::array<std::pair<FName, FName>, 3> ClassRedirects{{
    {FName("LevelStreamingKismet"), FName("LevelStreamingDynamic")},
    {FName("AssetObjectProperty"), FName("SoftObjectProperty")},
    {FName("AssetClassProperty"), FName("SoftClassProperty")},
}};

//! The engine reads these from the StructRedirects of BaseEngine.ini; they map the struct names of older versions to the ported structs.
constexpr std::array<std::pair<FName, FName>, 4> StructRedirects{{
    {FName("MaterialLayersFunctions"), FName("MaterialLayersFunctionsRuntimeData")},
    {FName("ClothingAssetData"), FName("ClothingAssetData_Legacy")},
    {FName("ClothPhysicsProperties"), FName("ClothPhysicsProperties_Legacy")},
    {FName("AttenuationSettings"), FName("SoundAttenuationSettings")},
}};

struct FPropertyRedirect {
    FName ClassName;
    FName OldPropertyName;
    FName NewPropertyName;
};

//! The engine reads these from the TaggedPropertyRedirects of BaseEngine.ini.
constexpr std::array<FPropertyRedirect, 12> PropertyRedirects{{
    {FName("ActorComponent"), FName("bWantsBeginPlayComponent"), FName("bWantsInitializeComponent")},
    {FName("Actor"), FName("LifeSpan"), FName("InitialLifeSpan")},
    {FName("MeshComponent"), FName("Materials"), FName("OverrideMaterials")},
    {FName("PrimitiveComponent"), FName("bReceiveCSMFromDynamicObjects"), FName("bReceiveCombinedCSMAndStaticShadowsFromStationaryLights")},
    {FName("PrimitiveComponent"), FName("CanBeCharacterBase"), FName("CanCharacterStepUpOn")},
    {FName("SceneComponent"), FName("ModifyFrequency"), FName("Mobility")},
    {FName("SkeletalMeshComponent"), FName("bPropagateCurvesToSlaves"), FName("bPropagateCurvesToFollowers")},
    {FName("SkinnedMeshComponent"), FName("bIgnoreMasterPoseComponentLOD"), FName("bIgnoreLeaderPoseComponentLOD")},
    {FName("SkinnedMeshComponent"), FName("bUseBoundsFromMasterPoseComponent"), FName("bUseBoundsFromLeaderPoseComponent")},
    {FName("SkinnedMeshComponent"), FName("MasterPoseComponent"), FName("LeaderPoseComponent")},
    {FName("SkinnedMeshComponent"), FName("MeshComponentUpdateFlag"), FName("VisibilityBasedAnimTickOption")},
    {FName("WorldSettings"), FName("DefaultGameType"), FName("DefaultGameMode")},
}};

struct FEnumValueRedirect {
    FName EnumName;
    FName OldValueName;
    FName NewValueName;
};

//! The engine reads these from the ValueChanges of the EnumRedirects in BaseEngine.ini.
constexpr std::array<FEnumValueRedirect, 7> EnumValueRedirects{{
    {FName("EAttenuationDistanceModel"), FName("ATTENUATION_Linear"), FName("EAttenuationDistanceModel::Linear")},
    {FName("EAttenuationDistanceModel"), FName("ATTENUATION_Logarithmic"), FName("EAttenuationDistanceModel::Logarithmic")},
    {FName("EAttenuationDistanceModel"), FName("ATTENUATION_Inverse"), FName("EAttenuationDistanceModel::Inverse")},
    {FName("EAttenuationDistanceModel"), FName("ATTENUATION_LogReverse"), FName("EAttenuationDistanceModel::LogReverse")},
    {FName("EAttenuationDistanceModel"), FName("ATTENUATION_NaturalSound"), FName("EAttenuationDistanceModel::NaturalSound")},
    {FName("EAttenuationDistanceModel"), FName("ATTENUATION_Custom"), FName("EAttenuationDistanceModel::Custom")},
    {FName("TextureCompressionSettings"), FName("TC_ReflectionCapture"), FName("TC_EncodedReflectionCapture")},
}};

}

FName FCoreRedirects::GetRedirectedPropertyName(const UStruct* Struct, FName OldName) {
    for (; Struct != nullptr; Struct = Struct->GetSuperStruct()) {
        const FName ClassName = Struct->GetFName();
        const auto Found = std::ranges::find_if(PropertyRedirects, [&](const FPropertyRedirect& Redirect) {
            return Redirect.ClassName == ClassName && Redirect.OldPropertyName == OldName;
        });
        if (Found != PropertyRedirects.end())
            return Found->NewPropertyName;
    }
    return OldName;
}

FName FCoreRedirects::GetRedirectedClassName(FName OldName) {
    const auto Found = std::ranges::find(ClassRedirects, OldName, &std::pair<FName, FName>::first);
    return Found != ClassRedirects.end() ? Found->second : OldName;
}

FName FCoreRedirects::GetRedirectedStructName(FName OldName) {
    const auto Found = std::ranges::find(StructRedirects, OldName, &std::pair<FName, FName>::first);
    return Found != StructRedirects.end() ? Found->second : OldName;
}

FName FCoreRedirects::GetRedirectedEnumValueName(FName EnumName, FName OldValueName) {
    const auto Found = std::ranges::find_if(EnumValueRedirects, [&](const FEnumValueRedirect& Redirect) {
        return Redirect.EnumName == EnumName && Redirect.OldValueName == OldValueName;
    });
    return Found != EnumValueRedirects.end() ? Found->NewValueName : OldValueName;
}

}
