#include "RefGenReferenceLibrary.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, RefGenReference)

namespace
{
	bool SetVariables(UAnimInstance* Instance, const TMap<FName, FString>& Variables)
	{
		for (const TPair<FName, FString>& Variable : Variables)
		{
			FProperty* Property = Instance->GetClass()->FindPropertyByName(Variable.Key);
			if (Property == nullptr || Property->ImportText_InContainer(*Variable.Value, Instance, Instance, PPF_None) == nullptr)
				return false;
		}
		return true;
	}
}

TArray<FTransform> URefGenReferenceLibrary::EvaluateAnimBlueprint(USkeletalMesh* Mesh, UClass* AnimClass, const TMap<FName, FString>& Variables, int32 Ticks, float DeltaSeconds)
{
	if (Mesh == nullptr || AnimClass == nullptr)
		return {};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(World);
	Component->SetSkeletalMeshAsset(Mesh);
	Component->SetAnimInstanceClass(AnimClass);
	Component->RegisterComponentWithWorld(World);
	TArray<FTransform> Pose;
	UAnimInstance* Instance = Component->GetAnimInstance();
	if (Instance != nullptr && SetVariables(Instance, Variables))
	{
		for (int32 Tick = 0; Tick < Ticks; Tick++)
		{
			Component->TickAnimation(DeltaSeconds, false);
			Component->RefreshBoneTransforms();
		}
		Pose = Component->GetBoneSpaceTransforms();
	}
	Component->UnregisterComponent();
	World->DestroyWorld(false);
	return Pose;
}
