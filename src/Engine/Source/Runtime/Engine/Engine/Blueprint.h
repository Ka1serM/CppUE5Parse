#pragma once

#include "Engine/Source/Runtime/Engine/Engine/BlueprintCore.h"

#include "Blueprint.generated.h"

namespace ue5parse::runtime {

class UActorComponent;
class UInheritableComponentHandler;
class USimpleConstructionScript;
class UTimelineTemplate;

UENUM()
enum EBlueprintType : int32 { BPTYPE_Normal, BPTYPE_Const, BPTYPE_MacroLibrary, BPTYPE_Interface, BPTYPE_LevelScript, BPTYPE_FunctionLibrary, BPTYPE_MAX };

//! Cooks of UE 4.14 and older keep the Blueprint with only its runtime properties.
UCLASS()
class UBlueprint : public UBlueprintCore {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bRecompileOnLoad = false;
    UPROPERTY()
    TSubclassOf<UObject> ParentClass;
    UPROPERTY()
    TObjectPtr<USimpleConstructionScript> SimpleConstructionScript = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UActorComponent>> ComponentTemplates;
    UPROPERTY()
    TArray<TObjectPtr<UTimelineTemplate>> Timelines;
    UPROPERTY()
    TObjectPtr<UInheritableComponentHandler> InheritableComponentHandler = nullptr;
    UPROPERTY()
    TEnumAsByte<EBlueprintType> BlueprintType = BPTYPE_Normal;
    UPROPERTY()
    int32 BlueprintSystemVersion = 0;
};

}
