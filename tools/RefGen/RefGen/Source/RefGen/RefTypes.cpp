#include "RefTypes.h"

ARefNativeActor::ARefNativeActor()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
#if ENGINE_MAJOR_VERSION > 4 || ENGINE_MINOR_VERSION >= 8 // no GameplayAbilities before 4.8 (see RG_GAMEPLAY)
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
#endif
}

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 5
// 4.4 doesn't export FRuntimeFloatCurve's constructor (the struct isn't ENGINE_API); it only clears the external curve
FRuntimeFloatCurve::FRuntimeFloatCurve()
	: ExternalCurve(nullptr)
{
}
#endif
