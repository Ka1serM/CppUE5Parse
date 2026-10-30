#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "ue5parse/runtime/Engine/Engine/EngineBaseTypes.h"
#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"

#include "ActorComponent.generated.h"

namespace ue5parse::runtime {

class AActor;
class UWorld;
class UActorComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActorComponentActivatedSignature, UActorComponent*, Component, bool, bReset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActorComponentDeactivateSignature, UActorComponent*, Component);

enum class EComponentCreationMethod : uint8 { Native, SimpleConstructionScript, UserConstructionScript, Instance };

// UActorComponent: a reusable part of an actor (UE's Components/ActorComponent.h). It ticks once registered with its
// owner's world, and begins play with its owner.
UCLASS(Abstract)
class UActorComponent : public UObject {
    GENERATED_BODY()
public:
    explicit UActorComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FActorComponentTickFunction PrimaryComponentTick;
    UPROPERTY()
    TArray<FName> ComponentTags;
    UPROPERTY(Replicated)
    bool bReplicates = false;
    UPROPERTY()
    bool bAutoActivate = false;
    UPROPERTY(ReplicatedUsing = OnRep_IsActive)
    bool bIsActive = false;
    UPROPERTY()
    bool bIsEditorOnly = false;
    UPROPERTY()
    EComponentCreationMethod CreationMethod = EComponentCreationMethod::Native;
    UPROPERTY()
    FMulticastScriptDelegate OnComponentActivated;
    UPROPERTY()
    FMulticastScriptDelegate OnComponentDeactivated;

    void PostInitProperties() override;
    void Serialize(FArchive& Ar) override;
    UWorld* GetWorld() const override;
    int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;

    UFUNCTION(BlueprintCallable)
    AActor* GetOwner() const { return OwnerPrivate; }
    template <class T>
    T* GetOwner() const {
        return Cast<T>(OwnerPrivate);
    }
    UFUNCTION(BlueprintCallable)
    bool ComponentHasTag(FName Tag) const { return Tag != FName() && ComponentTags.Contains(Tag); }
    ENetRole GetOwnerRole() const;
    bool IsNetSimulating() const;
    ENetMode GetNetMode() const;

    // -- Registration --
    // Registers with the owner's world (UE's RegisterComponent).
    void RegisterComponent();
    void RegisterComponentWithWorld(UWorld* InWorld);
    void UnregisterComponent();
    bool IsRegistered() const { return bRegistered; }
    virtual void OnRegister();
    virtual void OnUnregister();
    virtual void InitializeComponent();
    virtual void UninitializeComponent();
    bool HasBeenInitialized() const { return bHasBeenInitialized; }
    bool HasBegunPlay() const { return bHasBegunPlay; }
    // bWantsInitializeComponent: the owner calls InitializeComponent.
    bool bWantsInitializeComponent = false;
    virtual void BeginPlay();
    virtual void EndPlay(EEndPlayReason::Type EndPlayReason);
    void RegisterAllComponentTickFunctions(bool bRegister);
    virtual void RegisterComponentTickFunctions(bool bRegister);
    bool SetupActorComponentTickFunction(FTickFunction* TickFunction);
    // ReceiveTick when the class is a Blueprint (UE's TickComponent).
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
    // UE's DestroyComponent: unregistered, removed from the owner, marked as garbage.
    virtual void DestroyComponent(bool bPromoteChildren = false);
    virtual void OnComponentCreated() { bHasBeenCreated = true; }
    bool HasBeenCreated() const { return bHasBeenCreated; }
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) { bHasBeenCreated = false; }
    UFUNCTION(BlueprintCallable)
    bool IsBeingDestroyed() const { return bIsBeingDestroyed; }
    UFUNCTION(BlueprintCallable)
    void SetComponentTickEnabled(bool bEnabled);
    bool IsComponentTickEnabled() const { return PrimaryComponentTick.IsTickFunctionEnabled(); }
    void AddTickPrerequisiteActor(AActor* PrerequisiteActor);
    void AddTickPrerequisiteComponent(UActorComponent* PrerequisiteComponent);

    // -- Activation --
    virtual void Activate(bool bReset = false);
    virtual void Deactivate();
    void SetActiveFlag(bool bNewIsActive) { bIsActive = bNewIsActive; }
    virtual bool ShouldActivate() const { return !bIsActive; }
    UFUNCTION()
    void OnRep_IsActive() { SetComponentTickEnabled(IsActive()); }

    UFUNCTION(BlueprintCallable)
    bool IsActive() const { return bIsActive; }
    UFUNCTION(BlueprintCallable)
    void K2_Activate(bool bReset) { Activate(bReset); }
    UFUNCTION(BlueprintCallable)
    void K2_Deactivate() { Deactivate(); }
    UFUNCTION(BlueprintCallable)
    void SetActive(bool bNewActive, bool bReset);
    UFUNCTION(BlueprintCallable)
    void ToggleActive() { SetActive(!bIsActive, false); }
    UFUNCTION(BlueprintCallable)
    void SetAutoActivate(bool bNewAutoActivate) { bAutoActivate = bNewAutoActivate; }
    UFUNCTION(BlueprintCallable)
    void K2_DestroyComponent(UObject* Object);
    UFUNCTION(BlueprintCallable)
    void SetComponentTickInterval(float TickInterval) { PrimaryComponentTick.UpdateTickIntervalAndCoolDown(TickInterval); }
    UFUNCTION(BlueprintCallable)
    float GetComponentTickInterval() const { return PrimaryComponentTick.TickInterval; }
    UFUNCTION(BlueprintCallable)
    void SetIsReplicated(bool ShouldReplicate) { bReplicates = ShouldReplicate; }
    UFUNCTION(BlueprintCallable)
    void SetCanEverAffectNavigation(bool bRelevant) {}

    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveBeginPlay();
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveEndPlay(TEnumAsByte<EEndPlayReason::Type> EndPlayReason);
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveTick(float DeltaSeconds);

protected:
    AActor* OwnerPrivate = nullptr;
    UWorld* WorldPrivate = nullptr;
    bool bRegistered = false;
    bool bHasBeenCreated = false;
    bool bHasBeenInitialized = false;
    bool bHasBegunPlay = false;
    bool bIsBeingDestroyed = false;
    bool bTickFunctionsRegistered = false;
};

}
