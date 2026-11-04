#pragma once

#include <functional>

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"

#include "EngineBaseTypes.generated.h"

namespace ue5parse::runtime {

class ULevel;
class AActor;
class UActorComponent;

enum ETickingGroup : int32 {
    TG_PrePhysics,
    TG_StartPhysics,
    TG_DuringPhysics,
    TG_EndPhysics,
    TG_PostPhysics,
    TG_PostUpdateWork,
    TG_LastDemotable,
    TG_NewlySpawned,
    TG_MAX,
};

enum ELevelTick : int32 {
    LEVELTICK_TimeOnly = 0,
    LEVELTICK_ViewportsOnly = 1,
    LEVELTICK_All = 2,
    LEVELTICK_PauseTick = 3,
};

enum ENetRole : int32 {
    ROLE_None,
    ROLE_SimulatedProxy,
    ROLE_AutonomousProxy,
    ROLE_Authority,
    ROLE_MAX,
};

enum ENetMode : int32 {
    NM_Standalone,
    NM_DedicatedServer,
    NM_ListenServer,
    NM_Client,
    NM_MAX,
};

struct FURL {
    FString Protocol;
    FString Host;
    int32 Port = 0;
    bool Valid = false;
    FString Map;
    TArray<FString> Op;
    FString Portal;

    friend FArchive& operator<<(FArchive& Ar, FURL& U) {
        Ar << U.Protocol << U.Host << U.Map << U.Portal << U.Op << U.Port;
        int32 Valid = 0;
        Ar << Valid;
        U.Valid = Valid != 0;
        return Ar;
    }
};

struct FTickFunction;

struct FTickPrerequisite {
    TWeakObjectPtr<UObject> PrerequisiteObject;
    FTickFunction* PrerequisiteTickFunction = nullptr;

    FTickFunction* Get() const { return PrerequisiteObject.IsValid() ? PrerequisiteTickFunction : nullptr; }
    bool operator==(const FTickPrerequisite& Other) const {
        return PrerequisiteObject == Other.PrerequisiteObject && PrerequisiteTickFunction == Other.PrerequisiteTickFunction;
    }
};

USTRUCT()
struct FTickFunction {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<ETickingGroup> TickGroup = TG_PrePhysics;
    UPROPERTY()
    TEnumAsByte<ETickingGroup> EndTickGroup = TG_PrePhysics;
    UPROPERTY()
    bool bTickEvenWhenPaused = false;
    UPROPERTY()
    bool bCanEverTick = false;
    UPROPERTY()
    bool bStartWithTickEnabled = true;
    UPROPERTY()
    bool bAllowTickOnDedicatedServer = true;
    bool bHighPriority = false;
    bool bRunOnAnyThread = false;
    UPROPERTY()
    float TickInterval = 0;

    FTickFunction() = default;
    FTickFunction(const FTickFunction&) = delete;
    FTickFunction& operator=(const FTickFunction&) = delete;
    virtual ~FTickFunction();

    void RegisterTickFunction(ULevel* Level);
    void UnRegisterTickFunction();
    bool IsTickFunctionRegistered() const { return bRegistered; }
    void SetTickFunctionEnable(bool bInEnabled);
    bool IsTickFunctionEnabled() const { return TickState != ETickState::Disabled; }
    void UpdateTickIntervalAndCoolDown(float NewTickInterval);
    void AddPrerequisite(UObject* TargetObject, FTickFunction& TargetTickFunction);
    void RemovePrerequisite(UObject* TargetObject, FTickFunction& TargetTickFunction);
    TArray<FTickPrerequisite>& GetPrerequisites() { return Prerequisites; }
    const TArray<FTickPrerequisite>& GetPrerequisites() const { return Prerequisites; }

    void DontCompleteUntil(std::function<void()> InCompletionTask);

    virtual void ExecuteTick(float DeltaTime, ELevelTick TickType) {}
    virtual FString DiagnosticMessage() { return FString("TickFunction"); }

private:
    friend class FTickTaskManager;
    enum class ETickState : uint8 { Disabled, Enabled, CoolingDown };

    ETickState TickState = ETickState::Enabled;
    bool bRegistered = false;
    bool bWasInterval = false;
    TEnumAsByte<ETickingGroup> ActualStartTickGroup = TG_PrePhysics;
    TEnumAsByte<ETickingGroup> ActualEndTickGroup = TG_PrePhysics;
    int32 TickVisitedGFrameCounter = 0;
    float RelativeTickCooldown = 0;
    float LastTickGameTimeSeconds = -1;
    ULevel* TickLevel = nullptr;
    TArray<FTickPrerequisite> Prerequisites;
    std::function<void()> CompletionTask;
};

template <>
struct TStructOpsTypeTraits<FTickFunction> : TStructOpsTypeTraitsBase2<FTickFunction> {
    enum { WithCopy = false };
};

USTRUCT()
struct FActorTickFunction : public FTickFunction {
    GENERATED_BODY()
    AActor* Target = nullptr;

    void ExecuteTick(float DeltaTime, ELevelTick TickType) override;
    FString DiagnosticMessage() override;
};

template <>
struct TStructOpsTypeTraits<FActorTickFunction> : TStructOpsTypeTraitsBase2<FActorTickFunction> {
    enum { WithCopy = false };
};

USTRUCT()
struct FActorComponentTickFunction : public FTickFunction {
    GENERATED_BODY()
    UActorComponent* Target = nullptr;

    void ExecuteTick(float DeltaTime, ELevelTick TickType) override;
    FString DiagnosticMessage() override;
};

template <>
struct TStructOpsTypeTraits<FActorComponentTickFunction> : TStructOpsTypeTraitsBase2<FActorComponentTickFunction> {
    enum { WithCopy = false };
};

}
