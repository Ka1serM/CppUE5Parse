#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "BlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

class AActor;
class FStructProperty;
class UActorComponent;
class UDynamicBlueprintBinding;
class UTimelineTemplate;
class UInheritableComponentHandler;
class USimpleConstructionScript;

USTRUCT()
struct FBlueprintComponentChangedPropertyInfo {
    GENERATED_BODY()
    UPROPERTY()
    FName PropertyName;
    UPROPERTY()
    int32 ArrayIndex = 0;
    UPROPERTY()
    TObjectPtr<UStruct> PropertyScope = nullptr;
};

USTRUCT()
struct FBlueprintCookedComponentInstancingData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FBlueprintComponentChangedPropertyInfo> ChangedPropertyList;
    UPROPERTY()
    bool bHasValidCookedData = false;
};

USTRUCT()
struct FPointerToUberGraphFrame {
    GENERATED_BODY()
    uint8* RawPointer = nullptr;
};

template <>
struct TStructOpsTypeTraits<FPointerToUberGraphFrame> : TStructOpsTypeTraitsBase2<FPointerToUberGraphFrame> {
    enum { WithZeroConstructor = true, WithCopy = false };
};

UCLASS()
class UBlueprintGeneratedClass : public UClass {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UDynamicBlueprintBinding>> DynamicBindingObjects;
    UPROPERTY()
    TArray<TObjectPtr<UActorComponent>> ComponentTemplates;
    UPROPERTY()
    TArray<TObjectPtr<UTimelineTemplate>> Timelines;
    UPROPERTY()
    TObjectPtr<USimpleConstructionScript> SimpleConstructionScript = nullptr;
    UPROPERTY()
    TObjectPtr<UInheritableComponentHandler> InheritableComponentHandler = nullptr;
    UPROPERTY()
    TObjectPtr<UFunction> UberGraphFunction = nullptr;
    FStructProperty* UberGraphFramePointerProperty = nullptr;

    void Serialize(FArchive& Ar) override;
    void Link() override;
    void SerializeDefaultObject(UObject* Object, FArchive& Ar) override;
    uint8* GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const override;
    void CreatePersistentUberGraphFrame(UObject* Obj, bool bCreateOnlyIfEmpty = false, bool bSkipSuperClass = false, UClass* OldClass = nullptr) const override;
    void DestroyPersistentUberGraphFrame(UObject* Obj, bool bSkipSuperClass = false) const override;
    void PostInitInstance(UObject* Obj) const override;
    static FName GetUberGraphFrameName();
    void InitPropertiesFromCustomList(uint8* DataPtr, const uint8* DefaultDataPtr) override;
    static bool GetGeneratedClassesHierarchy(const UClass* InClass, TArray<const UBlueprintGeneratedClass*>& OutBPGClasses);
    USimpleConstructionScript* GetSimpleConstructionScript() const { return SimpleConstructionScript; }
    UActorComponent* FindComponentTemplateByName(FName TemplateName) const;
    static void CreateComponentsForActor(const UClass* ThisClass, AActor* Actor);
    static void BindDynamicDelegates(const UClass* ThisClass, UObject* InInstance);
    UInheritableComponentHandler* GetInheritableComponentHandler() const { return InheritableComponentHandler; }
};

}
