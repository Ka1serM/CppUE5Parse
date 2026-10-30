#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"

#include "BlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

class AActor;
class UActorComponent;
class UDynamicBlueprintBinding;
class UTimelineTemplate;
class UInheritableComponentHandler;
class USimpleConstructionScript;

// FBlueprintComponentChangedPropertyInfo: a property of a component template that differs from its archetype.
USTRUCT()
struct FBlueprintComponentChangedPropertyInfo {
    GENERATED_BODY()
    UPROPERTY()
    FName PropertyName;
    UPROPERTY()
    int32 ArrayIndex = 0;
    UPROPERTY()
    UStruct* PropertyScope = nullptr;
};

// FBlueprintCookedComponentInstancingData: what the cook prepared to instance a component template fast.
USTRUCT()
struct FBlueprintCookedComponentInstancingData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FBlueprintComponentChangedPropertyInfo> ChangedPropertyList;
    UPROPERTY()
    bool bHasValidCookedData = false;
};

// UBlueprintGeneratedClass: the class a Blueprint compiles to, with the templates of the components it adds (UE's
// Engine/BlueprintGeneratedClass.h).
UCLASS()
class UBlueprintGeneratedClass : public UClass {
    GENERATED_BODY()
public:
    // The component events and the like a Blueprint binds when it is constructed (UComponentDelegateBinding...).
    UPROPERTY()
    TArray<UDynamicBlueprintBinding*> DynamicBindingObjects;
    // The templates that AddComponent nodes create components from.
    UPROPERTY()
    TArray<UActorComponent*> ComponentTemplates;
    // The templates of the class's own timelines.
    UPROPERTY()
    TArray<UTimelineTemplate*> Timelines;
    UPROPERTY()
    USimpleConstructionScript* SimpleConstructionScript = nullptr;
    UPROPERTY()
    UInheritableComponentHandler* InheritableComponentHandler = nullptr;
    UPROPERTY()
    UFunction* UberGraphFunction = nullptr;

    void Serialize(FArchive& Ar) override;
    // The Blueprint classes of a class's super chain, from the class to the first native one (UE's
    // GetGeneratedClassesHierarchy). True when every super is a Blueprint class up to a native one.
    static bool GetGeneratedClassesHierarchy(const UClass* InClass, TArray<const UBlueprintGeneratedClass*>& OutBPGClasses);
    USimpleConstructionScript* GetSimpleConstructionScript() const { return SimpleConstructionScript; }
    UActorComponent* FindComponentTemplateByName(FName TemplateName) const;
    // The timeline components of the class's timelines (UE's CreateComponentsForActor).
    static void CreateComponentsForActor(const UClass* ThisClass, AActor* Actor);
    // Binds the dynamic delegates of each Blueprint class of the chain on the instance.
    static void BindDynamicDelegates(const UClass* ThisClass, UObject* InInstance);
    UInheritableComponentHandler* GetInheritableComponentHandler() const { return InheritableComponentHandler; }
};

}
