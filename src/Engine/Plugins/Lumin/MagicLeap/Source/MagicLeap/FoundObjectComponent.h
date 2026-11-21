#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "FoundObjectComponent.generated.h"

namespace ue5parse::runtime {

class UBoxComponent;

UENUM()
enum class EFoundObjectType : uint8 { None, PersistentPoint, Plane, Generic };

USTRUCT()
struct FFoundObjectProperty {
    GENERATED_BODY()

    UPROPERTY()
    FString Key;
    UPROPERTY()
    FString Value;
};

UCLASS()
class UFoundObjectComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UFoundObjectComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FString QueryObjectID;
    UPROPERTY()
    TArray<FString> QueryLabels;
    UPROPERTY()
    TArray<EFoundObjectType> QueryTypes;
    UPROPERTY()
    TArray<FFoundObjectProperty> QueryProperties;
    UPROPERTY()
    int32 MaxResults = 1;
    UPROPERTY()
    TObjectPtr<UBoxComponent> SearchVolume = nullptr;
};

}
