#include "Engine/Source/Runtime/Engine/GameFramework/PainCausingVolume.h"

#include "Engine/Source/Runtime/Engine/EngineNativeMissing.h"

namespace ue5parse::runtime {

APainCausingVolume::APainCausingVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DamageType = UDamageType::StaticClass();
}

}
