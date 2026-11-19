#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RefGenReferenceLibrary.generated.h"

class USkeletalMesh;

UCLASS()
class REFGENREFERENCE_API URefGenReferenceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RefGen")
	static TArray<FTransform> EvaluateAnimBlueprint(USkeletalMesh* Mesh, UClass* AnimClass, const TMap<FName, FString>& Variables, int32 Ticks, float DeltaSeconds);
};
