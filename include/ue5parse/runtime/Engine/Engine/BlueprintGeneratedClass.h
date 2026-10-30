#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"

#include "BlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

class AActor;
class FStructProperty;
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

// FPointerToUberGraphFrame: the pointer to an object's persistent frame of its class's ubergraph, the hidden
// UberGraphFrame property of a Blueprint class.
USTRUCT()
struct FPointerToUberGraphFrame {
    GENERATED_BODY()
    uint8* RawPointer = nullptr;
};

template <>
struct TStructOpsTypeTraits<FPointerToUberGraphFrame> : TStructOpsTypeTraitsBase2<FPointerToUberGraphFrame> {
    enum { WithZeroConstructor = true, WithCopy = false };
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
    // The class's own UberGraphFrame property, found when the class links.
    FStructProperty* UberGraphFramePointerProperty = nullptr;

    void Serialize(FArchive& Ar) override;
    void Link() override;
    void SerializeDefaultObject(UObject* Object, FArchive& Ar) override;
    // The persistent frame of the ubergraph that a Blueprint event's parameters and the ubergraph's locals live in.
    uint8* GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const override;
    void CreatePersistentUberGraphFrame(UObject* Obj, bool bCreateOnlyIfEmpty = false, bool bSkipSuperClass = false,
                                        UClass* OldClass = nullptr) const override;
    void DestroyPersistentUberGraphFrame(UObject* Obj, bool bSkipSuperClass = false) const override;
    void PostInitInstance(UObject* Obj) const override;
    static FName GetUberGraphFrameName();
    // UE's slow path: every property that C++ owns. UE's fast path copies only the ones whose value differs from the
    // native default object, which the C++ constructor already set, to the same result.
    void InitPropertiesFromCustomList(uint8* DataPtr, const uint8* DefaultDataPtr) override;
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
