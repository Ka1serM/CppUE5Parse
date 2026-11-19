#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

namespace ue5parse::runtime {

bool FActorInstanceHandle::DoesRepresentClass(const UClass* OtherClass) const {
    AActor* Resolved = Actor.Get();
    return OtherClass != nullptr && Resolved != nullptr && Resolved->IsA(OtherClass);
}


FArchive& operator<<(FArchive& Ar, FLightmassPrimitiveSettings& Settings) {
    bool bUseTwoSidedLighting = false;
    bool bShadowIndirectOnly = false;
    bool bUseEmissiveForStaticLighting = false;
    bool bUseVertexNormalForHemisphereGather = false;
    Ar << bUseTwoSidedLighting << bShadowIndirectOnly << Settings.FullyOccludedSamplesFraction << bUseEmissiveForStaticLighting;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::NEW_LIGHTMASS_PRIMITIVE_SETTING)
        Ar << bUseVertexNormalForHemisphereGather;
    Ar << Settings.EmissiveLightFalloffExponent << Settings.EmissiveLightExplicitInfluenceRadius << Settings.EmissiveBoost << Settings.DiffuseBoost;
    Settings.bUseTwoSidedLighting = bUseTwoSidedLighting;
    Settings.bShadowIndirectOnly = bShadowIndirectOnly;
    Settings.bUseEmissiveForStaticLighting = bUseEmissiveForStaticLighting;
    Settings.bUseVertexNormalForHemisphereGather = bUseVertexNormalForHemisphereGather;
    return Ar;
}

}
