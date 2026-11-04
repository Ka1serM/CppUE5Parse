#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"

#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/PlayerController.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitter.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleHelper.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleEvent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"

namespace ue5parse::runtime {

UParticleSystemComponent::UParticleSystemComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_DuringPhysics;
    PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
    bAutoActivate = true;
    //! The export must be deterministic: always the fixed seed, as UE with FApp::bUseFixedSeed.
    RandomStream.Initialize(int32(GetTypeHash(NameToString(GetFName()))));
}

void UParticleSystemComponent::PostLoad() {
    Super::PostLoad();
    if (Template)
        Template->ConditionalPostLoad();
}

void UParticleSystemComponent::BeginDestroy() {
    Super::BeginDestroy();
    ResetParticles(true);
}

void UParticleSystemComponent::OnRegister() {
    bIsDuringRegister = true;
    UWorld* World = GetWorld();
    if (bAutoManageAttachment && !IsActive()) {
        if (GetAttachParent()) {
            if (!AutoAttachParent.IsValid())
                AutoAttachParent = GetAttachParent();
            if (AutoAttachSocketName == NAME_None)
                AutoAttachSocketName = GetAttachSocketName();
            if (World->IsGameWorld()) {
                if (GetAttachParent()->AttachChildren.Contains(this)) {
                    if (!bAutoActivate ||
                        (AutoAttachLocationRule != EAttachmentRule::KeepRelative && AutoAttachRotationRule != EAttachmentRule::KeepRelative &&
                            AutoAttachScaleRule != EAttachmentRule::KeepRelative) ||
                        AutoAttachSocketName != GetAttachSocketName() || AutoAttachParent != GetAttachParent())
                        DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepRelative, false));
                } else
                    SetupAttachment(nullptr, NAME_None);
            }
        }
        SavedAutoAttachRelativeLocation = GetRelativeLocation();
        SavedAutoAttachRelativeRotation = GetRelativeRotation();
        SavedAutoAttachRelativeScale3D = GetRelativeScale3D();
    }
    Super::OnRegister();
    if (bWasActive && !IsActive())
        Activate(true);
    if (LODLevel == -1)
        LODLevel = 0;
    bIsDuringRegister = false;
}

void UParticleSystemComponent::OnUnregister() {
    bWasActive = IsActive() && !bWasDeactivated;
    SetComponentTickEnabled(false);
    ResetParticles(!bAllowRecycling);
    Super::OnUnregister();
}

void UParticleSystemComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    if (Template == nullptr || Template->Emitters.Num() == 0) {
        SetComponentTickEnabled(false);
        return;
    }
    if (TimeSinceLastTick + static_cast<uint32>(DeltaTime * 1000.0f) < Template->MinTimeBetweenTicks) {
        TimeSinceLastTick += static_cast<uint32>(DeltaTime * 1000.0f);
        return;
    }
    DeltaTime += TimeSinceLastTick / 1000.0f;
    TimeSinceLastTick = 0;
    if (bDeactivateTriggered) {
        DeactivateSystem();
        if (bWasDeactivated)
            BroadcastDeactivated();
    }
    if (!IsActive() && !bAutoActivate) {
        SetComponentTickEnabled(false);
        return;
    }
    DeltaTimeTick = DeltaTime;
    if (!bUpdateOnDedicatedServer && GetNetMode() == NM_DedicatedServer) {
        if (bAutoDestroy)
            DestroyComponent();
        else
            SetComponentTickEnabled(false);
        return;
    }
    bool bRequiresReset = bResetTriggered;
    bResetTriggered = false;
    if (DetailMode > GetCurrentDetailMode()) {
        if (IsActive())
            DeactivateSystem();
        return;
    }
    if (bRequiresReset) {
        const bool bOldActive = IsActive();
        ResetParticles(true);
        if (bOldActive)
            ActivateSystem();
        else
            InitializeSystem();
    }
    if (!bWarmingUp) {
        AccumLODDistanceCheckTime += DeltaTime;
        if (AccumLODDistanceCheckTime > Template->LODDistanceCheckTime) {
            AccumLODDistanceCheckTime = 0.0f;
            const bool bCalculateLODLevel =
                bOverrideLODMethod ? LODMethod == PARTICLESYSTEMLODMETHOD_Automatic : Template->LODMethod == PARTICLESYSTEMLODMETHOD_Automatic;
            if (bCalculateLODLevel)
                SetLODLevel(DetermineLODLevelForLocation(GetComponentLocation()));
        }
    }
    DeltaTime *= CustomTimeDilation;
    DeltaTimeTick = DeltaTime;
    AccumTickTime += DeltaTime;
    UpdatePlayerLocations();
    if (Template->bOrientZAxisTowardCamera)
        OrientZAxisTowardCamera();
    if (Template->SystemUpdateMode == EPSUM_FixedTime)
        DeltaTime = Template->UpdateTime_Delta;
    SpawnEvents.Reset();
    DeathEvents.Reset();
    CollisionEvents.Reset();
    BurstEvents.Reset();
    TotalActiveParticles = 0;
    ComputeTickComponent();
    FinalizeTickComponent();
}

void UParticleSystemComponent::UpdatePlayerLocations() {
    PlayerLocations.Reset();
    PlayerLODDistanceFactor.Reset();
    UWorld* World = GetWorld();
    if (!World->IsGameWorld())
        return;
    for (AController* Controller : World->GetControllers()) {
        APlayerController* PlayerController = Cast<APlayerController>(Controller);
        if (PlayerController && PlayerController->IsLocalPlayerController()) {
            FVector POVLoc;
            FRotator POVRotation;
            PlayerController->GetPlayerViewPoint(POVLoc, POVRotation);
            PlayerLocations.Add(POVLoc);
            PlayerLODDistanceFactor.Add(1.0f);
        }
    }
}

void UParticleSystemComponent::OrientZAxisTowardCamera() {
    if (PlayerLocations.Num() == 0)
        return;
    FVector DirToCamera = PlayerLocations[0] - GetComponentLocation();
    DirToCamera.Normalize();
    DirToCamera = GetComponentTransform().InverseTransformVectorNoScale(DirToCamera);
    const FQuat PointTo = FQuat::FindBetweenNormals(FVector(0, 0, 1), DirToCamera);
    const FRotator AdjustmentAngle(PointTo);
    if (!AdjustmentAngle.IsZero())
        SetRelativeRotation(GetRelativeRotation() + AdjustmentAngle);
}

int32 UParticleSystemComponent::GetCurrentDetailMode() const {
    return DM_Epic;
}

void UParticleSystemComponent::ComputeTickComponent() {
    NumSignificantEmitters = 0;
    for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++) {
        FParticleEmitterInstance* Instance = EmitterInstances[EmitterIndex];
        if (Instance && Instance->SpriteTemplate) {
            UParticleLODLevel* SpriteLODLevel = Instance->SpriteTemplate->GetCurrentLODLevel(Instance);
            if (SpriteLODLevel && SpriteLODLevel->bEnabled) {
                ++NumSignificantEmitters;
                Instance->Tick(DeltaTimeTick, bSuppressSpawning);
                Instance->Tick_MaterialOverrides(EmitterIndex);
                TotalActiveParticles += Instance->ActiveParticles;
            }
        }
    }
}

void UParticleSystemComponent::FinalizeTickComponent() {
    for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++) {
        FParticleEmitterInstance* Instance = EmitterInstances[EmitterIndex];
        if (Instance && Instance->bEnabled && Instance->SpriteTemplate) {
            UParticleLODLevel* SpriteLODLevel = Instance->SpriteTemplate->GetCurrentLODLevel(Instance);
            if (SpriteLODLevel && SpriteLODLevel->bEnabled)
                Instance->ProcessParticleEvents(DeltaTimeTick, bSuppressSpawning);
        }
    }
    KismetEvents.Empty();
    bJustRegistered = false;
    LastSignificantTime = GetWorld()->GetTimeSeconds();
    const bool bIsCompleted = HasCompleted();
    if (bIsCompleted && !bWasCompleted)
        Complete();
    bWasCompleted = bIsCompleted;
    UpdateBounds();
    if (bOldPositionValid) {
        const float InvDeltaTime = DeltaTimeTick > 0.0f ? 1.0f / DeltaTimeTick : 0.0f;
        PartSysVelocity = (GetComponentLocation() - OldPosition) * InvDeltaTime;
    } else
        PartSysVelocity = FVector::ZeroVector;
    bOldPositionValid = true;
    OldPosition = GetComponentLocation();
}

void UParticleSystemComponent::UpdateBounds() {
    if (Template && Template->bUseFixedRelativeBoundingBox) {
        Bounds = Template->FixedRelativeBoundingBox.TransformBy(GetComponentTransform());
        return;
    }
    FBox BoundingBox;
    for (FParticleEmitterInstance* EmitterInstance : EmitterInstances)
        if (EmitterInstance && EmitterInstance->HasActiveParticles())
            BoundingBox += EmitterInstance->GetBoundingBox();
    if (!BoundingBox.IsValid) {
        Bounds = FBox(GetComponentLocation(), GetComponentLocation());
        return;
    }
    const FVector ExpandAmount = BoundingBox.GetExtent() * 0.1f;
    Bounds = FBox(BoundingBox.Min - ExpandAmount, BoundingBox.Max + ExpandAmount);
}

double UParticleSystemComponent::GetLastRenderTime() const {
    return GetWorld()->GetTimeSeconds();
}

bool UParticleSystemComponent::ParticleLineCheck(FHitResult& Hit, AActor* SourceActor, const FVector& End, const FVector& Start,
    const FVector& HalfExtent, const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes) {
    return false;
}

void UParticleSystemComponent::InitParticles() {
    if (IsTemplate() || Template == nullptr)
        return;
    WarmupTime = Template->WarmupTime;
    WarmupTickRate = Template->WarmupTickRate;
    const int32 GlobalDetailMode = GetCurrentDetailMode();
    const int32 NumInstances = EmitterInstances.Num();
    const int32 NumEmitters = Template->Emitters.Num();
    const bool bIsFirstCreate = NumInstances == 0;
    EmitterInstances.SetNum(NumEmitters);
    bWasCompleted = bIsFirstCreate ? false : bWasCompleted;
    int32 PreferredLODLevel = LODLevel;
    bool bSetLodLevels = LODLevel > 0;
    for (int32 Idx = 0; Idx < NumEmitters; Idx++) {
        UParticleEmitter* Emitter = Template->Emitters[Idx];
        if (Emitter == nullptr)
            continue;
        FParticleEmitterInstance* Instance = NumInstances == 0 ? nullptr : EmitterInstances[Idx];
        const bool bDetailModeAllowsRendering = DetailMode <= GlobalDetailMode && (Emitter->DetailModeBitmask & (1 << GlobalDetailMode));
        if (bDetailModeAllowsRendering && Emitter->HasAnyEnabledLODs()) {
            if (Instance) {
                Instance->SetHaltSpawning(false);
                Instance->SetHaltSpawningExternal(false);
            } else {
                Instance = Emitter->CreateInstance(this);
                EmitterInstances[Idx] = Instance;
            }
            if (Instance) {
                Instance->bEnabled = true;
                Instance->InitParameters(Emitter, this);
                Instance->Init();
                PreferredLODLevel = FMath::Min(PreferredLODLevel, Emitter->LODLevels.Num());
                bSetLodLevels |= !bIsFirstCreate;
            }
        } else if (Instance) {
            delete Instance;
            EmitterInstances[Idx] = nullptr;
        }
    }
    if (bSetLodLevels) {
        LODLevel = PreferredLODLevel;
        for (FParticleEmitterInstance* Instance : EmitterInstances) {
            if (Instance == nullptr)
                continue;
            Instance->CurrentLODLevelIndex = FMath::Min(LODLevel, Instance->SpriteTemplate->LODLevels.Num() - 1);
            Instance->CurrentLODLevel = Instance->SpriteTemplate->LODLevels[Instance->CurrentLODLevelIndex];
        }
    }
    for (FParticleEmitterInstance* Instance : EmitterInstances)
        if (Instance)
            Instance->InitCustomColor();
}

void UParticleSystemComponent::ResetParticles(bool bEmptyInstances) {
    UWorld* OwningWorld = GetWorld();
    const bool bIsGameWorld = OwningWorld && OwningWorld->IsGameWorld();
    SetActiveFlag(false);
    if (!bIsGameWorld || bEmptyInstances) {
        for (FParticleEmitterInstance* EmitInst : EmitterInstances)
            delete EmitInst;
        EmitterInstances.Empty();
        return;
    }
    for (FParticleEmitterInstance* EmitInst : EmitterInstances)
        if (EmitInst)
            EmitInst->Rewind();
}

void UParticleSystemComponent::SetTemplate(UParticleSystem* NewTemplate) {
    const bool bIsTemplate = IsTemplate();
    bWasCompleted = false;
    bWasActive = IsActive() && !bWasDeactivated;
    const bool bResetInstances = NewTemplate != Template;
    if (!bIsTemplate)
        ResetParticles(bResetInstances);
    Template = NewTemplate;
    WarmupTime = Template ? Template->WarmupTime : 0.0f;
    LODLevel = 0;
    SetComponentTickEnabled(false);
    if (NewTemplate && IsRegistered()) {
        if ((bAutoActivate || bWasActive) && !bIsTemplate)
            ActivateSystem();
        else
            InitializeSystem();
    }
    EmitterMaterials.Empty();
    for (FParticleEmitterInstance* Instance : EmitterInstances)
        if (Instance)
            Instance->CurrentLODLevelIndex = 0;
}

void UParticleSystemComponent::ActivateSystem(bool bFlagAsJustAttached) {
    if (IsTemplate() || !IsRegistered())
        return;
    bOldPositionValid = false;
    OldPosition = FVector::ZeroVector;
    PartSysVelocity = FVector::ZeroVector;
    UWorld* World = GetWorld();
    const bool bIsGameWorld = World->IsGameWorld();
    if (DetailMode <= GetCurrentDetailMode() && Template) {
        const bool bWasAutoAttached = bDidAutoAttach;
        bDidAutoAttach = false;
        if (bAutoManageAttachment && bIsGameWorld) {
            if (USceneComponent* NewParent = AutoAttachParent.Get()) {
                const bool bAlreadyAttached = GetAttachParent() && GetAttachParent() == NewParent && GetAttachSocketName() == AutoAttachSocketName &&
                                              GetAttachParent()->AttachChildren.Contains(this);
                if (!bAlreadyAttached) {
                    bDidAutoAttach = bWasAutoAttached;
                    CancelAutoAttachment(true);
                    SavedAutoAttachRelativeLocation = GetRelativeLocation();
                    SavedAutoAttachRelativeRotation = GetRelativeRotation();
                    SavedAutoAttachRelativeScale3D = GetRelativeScale3D();
                    AttachToComponent(NewParent,
                        FAttachmentTransformRules(
                            AutoAttachLocationRule, AutoAttachRotationRule, AutoAttachScaleRule, bAutoAttachWeldSimulatedBodies),
                        AutoAttachSocketName);
                }
                bDidAutoAttach = true;
                bFlagAsJustAttached = true;
            } else
                CancelAutoAttachment(true);
        }
        AccumTickTime = 0.0;
        if (!IsActive()) {
            LastSignificantTime = World->GetTimeSeconds();
            RequiredSignificance = EParticleSignificanceLevel::Low;
        }
        if (bFlagAsJustAttached)
            bJustRegistered = true;
        bSuppressSpawning = false;
        bWasCompleted = false;
        bWasDeactivated = false;
        SetActiveFlag(true);
        bWasActive = false;
        SetComponentTickEnabled(true);
        if (bIsGameWorld)
            SetLODLevel(DetermineLODLevelForLocation(GetComponentLocation()));
        if (EmitterInstances.Num() == 0 || (bIsGameWorld && (!bAutoActivate || bHasBeenActivated)))
            InitializeSystem();
        else if (EmitterInstances.Num() > 0 && !bIsGameWorld) {
            for (FParticleEmitterInstance* Instance : EmitterInstances) {
                if (Instance == nullptr)
                    continue;
                Instance->Rewind();
                Instance->SetHaltSpawning(false);
                Instance->SetHaltSpawningExternal(false);
            }
        }
        bHasBeenActivated = true;
        TimeSinceLastTick = 0;
        const bool bCalculateLODLevel =
            bOverrideLODMethod ? LODMethod != PARTICLESYSTEMLODMETHOD_DirectSet : Template->LODMethod != PARTICLESYSTEMLODMETHOD_DirectSet;
        if (bCalculateLODLevel) {
            const int32 DesiredLODLevel = DetermineLODLevelForLocation(GetComponentLocation());
            if (DesiredLODLevel != LODLevel) {
                SetActiveFlag(true);
                SetComponentTickEnabled(true);
            }
            SetLODLevel(DesiredLODLevel);
        }
        if (WarmupTime != 0.0f) {
            const bool bSaveSkipUpdate = bSkipUpdateDynamicDataDuringTick;
            bSkipUpdateDynamicDataDuringTick = true;
            bWarmingUp = true;
            ResetBurstLists();
            float WarmupElapsed = 0.f;
            float WarmupTimestep = 0.032f;
            if (WarmupTickRate > 0)
                WarmupTimestep = WarmupTickRate <= WarmupTime ? WarmupTickRate : WarmupTime;
            while (WarmupElapsed < WarmupTime) {
                TickComponent(WarmupTimestep, LEVELTICK_All, nullptr);
                WarmupElapsed += WarmupTimestep;
            }
            bWarmingUp = false;
            WarmupTime = 0.0f;
            bSkipUpdateDynamicDataDuringTick = bSaveSkipUpdate;
        }
    }
}

void UParticleSystemComponent::Complete() {
    UParticleSystemComponent* Parms = this;
    OnSystemFinished.ProcessMulticastDelegate(&Parms);
    ResetParticles();
    SetActiveFlag(false);
    SetComponentTickEnabled(false);
    if (bAutoDestroy)
        DestroyComponent();
    else if (bAutoManageAttachment)
        CancelAutoAttachment(true);
}

void UParticleSystemComponent::DeactivateSystem() {
    if (IsTemplate())
        return;
    bDeactivateTriggered = false;
    bSuppressSpawning = true;
    bWasDeactivated = true;
    if (GetWorld() == nullptr) {
        ResetParticles(true);
        return;
    }
    for (int32 i = 0; i < EmitterInstances.Num(); i++) {
        FParticleEmitterInstance* Instance = EmitterInstances[i];
        if (Instance == nullptr)
            continue;
        if (Instance->bKillOnDeactivate) {
            for (int32 InnerIndex = 0; InnerIndex < EmitterInstances.Num(); InnerIndex++)
                if (InnerIndex != i && EmitterInstances[InnerIndex] != nullptr)
                    EmitterInstances[InnerIndex]->OnEmitterInstanceKilled(Instance);
            delete Instance;
            EmitterInstances[i] = nullptr;
        } else
            Instance->OnDeactivateSystem();
    }
    SetComponentTickEnabled(true);
}

void UParticleSystemComponent::CancelAutoAttachment(bool bDetachFromParent) {
    UWorld* MyWorld = GetWorld();
    if (!bAutoManageAttachment || MyWorld == nullptr || !MyWorld->IsGameWorld())
        return;
    if (bDidAutoAttach) {
        SetRelativeLocation(SavedAutoAttachRelativeLocation);
        SetRelativeRotation(SavedAutoAttachRelativeRotation);
        SetRelativeScale3D(SavedAutoAttachRelativeScale3D);
        bDidAutoAttach = false;
    }
    if (bDetachFromParent)
        DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
}

void UParticleSystemComponent::Activate(bool bReset) {
    if (Template == nullptr)
        return;
    bDeactivateTriggered = false;
    if (!bReset && !ShouldActivate())
        return;
    ActivateSystem(bReset);
    if (!IsActive())
        return;
    struct {
        UActorComponent* Component;
        bool bReset;
    } Parms{this, bReset};
    OnComponentActivated.ProcessMulticastDelegate(&Parms);
}

void UParticleSystemComponent::Deactivate() {
    if (ShouldActivate())
        return;
    DeactivateSystem();
    if (bWasDeactivated)
        BroadcastDeactivated();
}

void UParticleSystemComponent::BroadcastDeactivated() {
    UActorComponent* Parms = this;
    OnComponentDeactivated.ProcessMulticastDelegate(&Parms);
}

bool UParticleSystemComponent::HasCompleted() {
    bool bHasCompleted = true;
    bool bCanBeDeactivated = true;
    for (int32 i = 0; i < EmitterInstances.Num(); i++) {
        FParticleEmitterInstance* Instance = EmitterInstances[i];
        if (Instance == nullptr || Instance->CurrentLODLevel == nullptr || !Instance->bEnabled)
            continue;
        if (!Instance->bEmitterIsDone)
            bCanBeDeactivated = false;
        if (!Instance->CurrentLODLevel->bEnabled) {
            UParticleEmitter* Em = CastChecked<UParticleEmitter>(Instance->CurrentLODLevel->GetOuter());
            if (Em && Em->bDisabledLODsKeepEmitterAlive)
                bHasCompleted = false;
            continue;
        }
        if (Instance->CurrentLODLevel->RequiredModule->EmitterLoops > 0 || Instance->IsTrailEmitter()) {
            if (bWasDeactivated && bSuppressSpawning) {
                if (Instance->ActiveParticles != 0)
                    bHasCompleted = false;
            } else if (Instance->HasCompleted()) {
                if (Instance->bKillOnCompleted) {
                    for (int32 InnerIndex = 0; InnerIndex < EmitterInstances.Num(); InnerIndex++)
                        if (InnerIndex != i && EmitterInstances[InnerIndex] != nullptr)
                            EmitterInstances[InnerIndex]->OnEmitterInstanceKilled(Instance);
                    delete Instance;
                    EmitterInstances[i] = nullptr;
                }
            } else
                bHasCompleted = false;
        } else if (!bWasDeactivated || Instance->ActiveParticles != 0)
            bHasCompleted = false;
    }
    if (bCanBeDeactivated && Template && Template->bAutoDeactivate)
        DeactivateSystem();
    return bHasCompleted;
}

void UParticleSystemComponent::InitializeSystem() {
    if (!IsRegistered() || IsTemplate() || DetailMode > GetCurrentDetailMode())
        return;
    if (Template != nullptr) {
        EmitterDelay = Template->Delay;
        if (Template->bUseDelayRange)
            EmitterDelay = Template->DelayLow + (Template->Delay - Template->DelayLow) * RandomStream.FRand();
    }
    InitParticles();
    AccumTickTime = 0.0;
    if (!IsActive() && bAutoActivate && !bWasDeactivated)
        SetActive(true, false);
}

int32 UParticleSystemComponent::DetermineLODLevelForLocation(const FVector& EffectLocation) {
    if (Template == nullptr || Template->LODDistances.Num() <= 1 || Template->LODMethod == PARTICLESYSTEMLODMETHOD_DirectSet)
        return 0;
    UpdatePlayerLocations();
    double LODDistanceSqr = PlayerLocations.Num() ? FMath::Square(UE_OLD_WORLD_MAX) : 0.0;
    for (const FVector& ViewLocation : PlayerLocations)
        LODDistanceSqr = FMath::Min(LODDistanceSqr, (ViewLocation - EffectLocation).SizeSquared());
    for (int32 LODIdx = 1; LODIdx < Template->LODDistances.Num(); ++LODIdx)
        if (LODDistanceSqr < FMath::Square(Template->LODDistances[LODIdx]))
            return LODIdx - 1;
    return Template->LODDistances.Num() - 1;
}

void UParticleSystemComponent::SetAutoAttachParams(USceneComponent* Parent, FName SocketName, EAttachLocation::Type LocationType) {
    AutoAttachParent = Parent;
    AutoAttachSocketName = SocketName;
    USceneComponent::ConvertAttachLocation(LocationType, AutoAttachLocationRule, AutoAttachRotationRule, AutoAttachScaleRule);
}

int32 UParticleSystemComponent::GetNumMaterials() const {
    if (Template)
        return Template->Emitters.Num();
    return 0;
}

UMaterialInterface* UParticleSystemComponent::GetMaterial(int32 ElementIndex) const {
    if (EmitterMaterials.IsValidIndex(ElementIndex) && EmitterMaterials[ElementIndex] != nullptr)
        return EmitterMaterials[ElementIndex];
    if (Template && Template->Emitters.IsValidIndex(ElementIndex)) {
        UParticleEmitter* Emitter = Template->Emitters[ElementIndex];
        if (Emitter && Emitter->LODLevels.Num() > 0) {
            UParticleLODLevel* EmitterLODLevel = Emitter->LODLevels[0];
            if (EmitterLODLevel && EmitterLODLevel->RequiredModule)
                return EmitterLODLevel->RequiredModule->Material;
        }
    }
    return nullptr;
}

void UParticleSystemComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* Material) {
    if (Template && Template->Emitters.IsValidIndex(ElementIndex)) {
        if (!EmitterMaterials.IsValidIndex(ElementIndex))
            EmitterMaterials.AddZeroed(ElementIndex + 1 - EmitterMaterials.Num());
        EmitterMaterials[ElementIndex] = Material;
    }
    for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); ++EmitterIndex)
        if (FParticleEmitterInstance* Inst = EmitterInstances[EmitterIndex])
            Inst->Tick_MaterialOverrides(EmitterIndex);
}

void UParticleSystemComponent::ResetBurstLists() {
    for (int32 i = 0; i < EmitterInstances.Num(); i++)
        if (EmitterInstances[i])
            EmitterInstances[i]->ResetBurstList();
}

bool UParticleSystemComponent::ShouldActivate() const {
    return (Super::ShouldActivate() || (bWasDeactivated || bWasCompleted));
}

int32 UParticleSystemComponent::GetNumActiveParticles() const {
    int32 NumParticles = 0;
    for (int32 i = 0; i < EmitterInstances.Num(); i++) {
        FParticleEmitterInstance* Instance = EmitterInstances[i];
        if (Instance != nullptr)
            NumParticles += Instance->ActiveParticles;
    }
    return NumParticles;
}

void UParticleSystemComponent::SetEmitterEnable(FName EmitterName, bool bNewEnableState) {
    for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); ++EmitterIndex) {
        FParticleEmitterInstance* EmitterInst = EmitterInstances[EmitterIndex];
        if (EmitterInst && EmitterInst->SpriteTemplate) {
            if (EmitterInst->SpriteTemplate->EmitterName == EmitterName)
                EmitterInst->SetHaltSpawningExternal(!bNewEnableState);
        }
    }
}

void UParticleSystemComponent::SetLODLevel(int32 InLODLevel) {
    if (Template == nullptr)
        return;
    if (Template->LODDistances.Num() == 0)
        return;
    int32 NewLODLevel = FMath::Clamp(InLODLevel, 0, Template->GetLODLevelCount() - 1);
    if (LODLevel != NewLODLevel) {
        LODLevel = NewLODLevel;
        for (int32 i = 0; i < EmitterInstances.Num(); i++) {
            FParticleEmitterInstance* Instance = EmitterInstances[i];
            if (Instance)
                Instance->SetCurrentLODIndex(LODLevel, true);
        }
    }
}

void UParticleSystemComponent::SetFloatParameter(FName Name, float Param) {
    if (Name == NAME_None)
        return;
    for (int32 i = 0; i < InstanceParameters.Num(); i++) {
        FParticleSysParam& P = InstanceParameters[i];
        if (P.Name == Name && P.ParamType == PSPT_Scalar) {
            P.Scalar = Param;
            return;
        }
    }
    int32 NewParamIndex = InstanceParameters.AddZeroed();
    InstanceParameters[NewParamIndex].Name = Name;
    InstanceParameters[NewParamIndex].ParamType = PSPT_Scalar;
    InstanceParameters[NewParamIndex].Scalar = Param;
}

void UParticleSystemComponent::SetFloatRandParameter(FName ParameterName, float Param, float ParamLow) {
    if (ParameterName == NAME_None)
        return;
    for (int32 i = 0; i < InstanceParameters.Num(); i++) {
        FParticleSysParam& P = InstanceParameters[i];
        if (P.Name == ParameterName && P.ParamType == PSPT_ScalarRand) {
            P.Scalar = Param;
            P.Scalar_Low = ParamLow;
            return;
        }
    }
    int32 NewParamIndex = InstanceParameters.AddZeroed();
    InstanceParameters[NewParamIndex].Name = ParameterName;
    InstanceParameters[NewParamIndex].ParamType = PSPT_ScalarRand;
    InstanceParameters[NewParamIndex].Scalar = Param;
    InstanceParameters[NewParamIndex].Scalar_Low = ParamLow;
}

void UParticleSystemComponent::SetVectorParameter(FName Name, FVector Param) {
    if (Name == NAME_None)
        return;
    for (int32 i = 0; i < InstanceParameters.Num(); i++) {
        FParticleSysParam& P = InstanceParameters[i];
        if (P.Name == Name && P.ParamType == PSPT_Vector) {
            P.Vector = Param;
            return;
        }
    }
    int32 NewParamIndex = InstanceParameters.AddZeroed();
    InstanceParameters[NewParamIndex].Name = Name;
    InstanceParameters[NewParamIndex].ParamType = PSPT_Vector;
    InstanceParameters[NewParamIndex].Vector = Param;
}

void UParticleSystemComponent::SetVectorRandParameter(FName ParameterName, const FVector& Param, const FVector& ParamLow) {
    if (ParameterName == NAME_None)
        return;
    for (int32 i = 0; i < InstanceParameters.Num(); i++) {
        FParticleSysParam& P = InstanceParameters[i];
        if (P.Name == ParameterName && P.ParamType == PSPT_VectorRand) {
            P.Vector = Param;
            P.Vector_Low = ParamLow;
            return;
        }
    }
    int32 NewParamIndex = InstanceParameters.AddZeroed();
    InstanceParameters[NewParamIndex].Name = ParameterName;
    InstanceParameters[NewParamIndex].ParamType = PSPT_VectorRand;
    InstanceParameters[NewParamIndex].Vector = Param;
    InstanceParameters[NewParamIndex].Vector_Low = ParamLow;
}

void UParticleSystemComponent::SetColorParameter(FName Name, FLinearColor Param) {
    if (Name == NAME_None)
        return;
    FColor NewColor(Param.ToFColor(true));
    for (int32 i = 0; i < InstanceParameters.Num(); i++) {
        FParticleSysParam& P = InstanceParameters[i];
        if (P.Name == Name && P.ParamType == PSPT_Color) {
            P.Color = NewColor;
            return;
        }
    }
    int32 NewParamIndex = InstanceParameters.AddZeroed();
    InstanceParameters[NewParamIndex].Name = Name;
    InstanceParameters[NewParamIndex].ParamType = PSPT_Color;
    InstanceParameters[NewParamIndex].Color = NewColor;
}

void UParticleSystemComponent::SetActorParameter(FName Name, AActor* Param) {
    if (Name == NAME_None)
        return;
    for (int32 i = 0; i < InstanceParameters.Num(); i++) {
        FParticleSysParam& P = InstanceParameters[i];
        if (P.Name == Name && P.ParamType == PSPT_Actor) {
            P.Actor = Param;
            return;
        }
    }
    int32 NewParamIndex = InstanceParameters.AddZeroed();
    InstanceParameters[NewParamIndex].Name = Name;
    InstanceParameters[NewParamIndex].ParamType = PSPT_Actor;
    InstanceParameters[NewParamIndex].Actor = Param;
}

void UParticleSystemComponent::SetMaterialParameter(FName Name, UMaterialInterface* Param) {
    if (Name == NAME_None)
        return;
    for (int32 i = 0; i < InstanceParameters.Num(); i++) {
        FParticleSysParam& P = InstanceParameters[i];
        if (P.Name == Name && P.ParamType == PSPT_Material) {
            P.Material = Param;
            return;
        }
    }
    int32 NewParamIndex = InstanceParameters.AddZeroed();
    InstanceParameters[NewParamIndex].Name = Name;
    InstanceParameters[NewParamIndex].ParamType = PSPT_Material;
    InstanceParameters[NewParamIndex].Material = Param;
}

bool UParticleSystemComponent::GetFloatParameter(const FName InName, float& OutFloat) {
    if (InName == NAME_None)
        return false;
    const TArray<struct FParticleSysParam>& UseInstanceParameters = InstanceParameters;
    for (int32 i = 0; i < UseInstanceParameters.Num(); i++) {
        const FParticleSysParam& Param = UseInstanceParameters[i];
        if (Param.Name == InName) {
            if (Param.ParamType == PSPT_Scalar) {
                OutFloat = Param.Scalar;
                return true;
            } else if (Param.ParamType == PSPT_ScalarRand) {
                OutFloat = Param.Scalar + (Param.Scalar_Low - Param.Scalar) * RandomStream.FRand();
                return true;
            }
        }
    }
    return false;
}

bool UParticleSystemComponent::GetAnyVectorParameter(const FName InName, FVector& OutVector) {
    if (InName == NAME_None)
        return false;
    const TArray<struct FParticleSysParam>& UseInstanceParameters = InstanceParameters;
    for (int32 i = 0; i < UseInstanceParameters.Num(); i++) {
        const FParticleSysParam& Param = UseInstanceParameters[i];
        if (Param.Name == InName) {
            if (Param.ParamType == PSPT_Vector) {
                OutVector = Param.Vector;
                return true;
            }
            if (Param.ParamType == PSPT_VectorRand) {
                FVector RandValue(RandomStream.FRand(), RandomStream.FRand(), RandomStream.FRand());
                OutVector = Param.Vector + (Param.Vector_Low - Param.Vector) * RandValue;
                return true;
            } else if (Param.ParamType == PSPT_VectorUnitRand) {
                return true;
            }
            if (Param.ParamType == PSPT_Scalar) {
                float OutFloat = Param.Scalar;
                OutVector = FVector(OutFloat, OutFloat, OutFloat);
                return true;
            }
            if (Param.ParamType == PSPT_ScalarRand) {
                float OutFloat = Param.Scalar + (Param.Scalar_Low - Param.Scalar) * RandomStream.FRand();
                OutVector = FVector(OutFloat, OutFloat, OutFloat);
                return true;
            }
            if (Param.ParamType == PSPT_Color) {
                const FLinearColor Color(Param.Color);
                OutVector = FVector(Color.R, Color.G, Color.B);
                return true;
            }
        }
    }
    return false;
}

bool UParticleSystemComponent::GetActorParameter(const FName InName, class AActor*& OutActor) {
    if (InName == NAME_None)
        return false;
    const TArray<struct FParticleSysParam>& UseInstanceParameters = InstanceParameters;
    for (int32 i = 0; i < UseInstanceParameters.Num(); i++) {
        const FParticleSysParam& Param = UseInstanceParameters[i];
        if (Param.Name == InName && Param.ParamType == PSPT_Actor) {
            OutActor = Param.Actor;
            return true;
        }
    }
    return false;
}

void UParticleSystemComponent::ClearParameter(FName ParameterName, TEnumAsByte<EParticleSysParamType> ParameterType) {
    for (int32 i = 0; i < InstanceParameters.Num(); i++)
        if (InstanceParameters[i].Name == ParameterName && (ParameterType == PSPT_None || InstanceParameters[i].ParamType == ParameterType))
            InstanceParameters.RemoveAt(i--);
}

void UParticleSystemComponent::ReportEventSpawn(const FName InEventName, const float InEmitterTime, const FVector InLocation,
    const FVector InVelocity, const TArray<TObjectPtr<UParticleModuleEventSendToGame>>& InEventData) {
    FParticleEventSpawnData* SpawnData = &SpawnEvents.Emplace_GetRef();
    SpawnData->Type = EPET_Spawn;
    SpawnData->EventName = InEventName;
    SpawnData->EmitterTime = InEmitterTime;
    SpawnData->Location = InLocation;
    SpawnData->Velocity = InVelocity;
    SpawnData->EventData = InEventData;
}

void UParticleSystemComponent::ReportEventDeath(const FName InEventName, const float InEmitterTime, const FVector InLocation,
    const FVector InVelocity, const TArray<TObjectPtr<UParticleModuleEventSendToGame>>& InEventData, const float InParticleTime) {
    FParticleEventDeathData* DeathData = &DeathEvents.Emplace_GetRef();
    DeathData->Type = EPET_Death;
    DeathData->EventName = InEventName;
    DeathData->EmitterTime = InEmitterTime;
    DeathData->Location = InLocation;
    DeathData->Velocity = InVelocity;
    DeathData->EventData = InEventData;
    DeathData->ParticleTime = InParticleTime;
}

void UParticleSystemComponent::ReportEventCollision(const FName InEventName, const float InEmitterTime, const FVector InLocation,
    const FVector InDirection, const FVector InVelocity, const TArray<TObjectPtr<UParticleModuleEventSendToGame>>& InEventData,
    const float InParticleTime, const FVector InNormal, const float InTime, const int32 InItem, const FName InBoneName, UPhysicalMaterial* PhysMat) {
    FParticleEventCollideData* CollideData = &CollisionEvents.Emplace_GetRef();
    CollideData->Type = EPET_Collision;
    CollideData->EventName = InEventName;
    CollideData->EmitterTime = InEmitterTime;
    CollideData->Location = InLocation;
    CollideData->Direction = InDirection;
    CollideData->Velocity = InVelocity;
    CollideData->EventData = InEventData;
    CollideData->ParticleTime = InParticleTime;
    CollideData->Normal = InNormal;
    CollideData->Time = InTime;
    CollideData->Item = InItem;
    CollideData->BoneName = InBoneName;
    CollideData->PhysMat = PhysMat;
}

void UParticleSystemComponent::ReportEventBurst(const FName InEventName, const float InEmitterTime, const int32 InParticleCount,
    const FVector InLocation, const TArray<TObjectPtr<UParticleModuleEventSendToGame>>& InEventData) {
    FParticleEventBurstData* BurstData = &BurstEvents.Emplace_GetRef();
    BurstData->Type = EPET_Burst;
    BurstData->EventName = InEventName;
    BurstData->EmitterTime = InEmitterTime;
    BurstData->ParticleCount = InParticleCount;
    BurstData->Location = InLocation;
    BurstData->EventData = InEventData;
}

void UParticleSystemComponent::KillParticlesForced() {
    for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
        if (EmitterInstances[EmitterIndex])
            EmitterInstances[EmitterIndex]->KillParticlesForced();
}

void UParticleSystemComponent::SetMaterialByName(FName MaterialSlotName, class UMaterialInterface* SourceMaterial) {
    int32 Index = GetNamedMaterialIndex(MaterialSlotName);
    if (INDEX_NONE == Index)
        return;
    if (SourceMaterial)
        SetMaterial(Index, SourceMaterial);
}

UMaterialInterface* UParticleSystemComponent::GetNamedMaterial(FName Name) const {
    int32 Index = GetNamedMaterialIndex(Name);
    if (INDEX_NONE != Index) {
        if (EmitterMaterials.IsValidIndex(Index) && nullptr != EmitterMaterials[Index])
            return EmitterMaterials[Index];
        else
            return Template ? Template->NamedMaterialSlots[Index].Material : nullptr;
    }
    return nullptr;
}

int32 UParticleSystemComponent::GetNamedMaterialIndex(FName Name) const {
    if (Template != nullptr) {
        TArray<FNamedEmitterMaterial>& Slots = Template->NamedMaterialSlots;
        for (int32 SlotIdx = 0; SlotIdx < Slots.Num(); ++SlotIdx)
            if (Name == Slots[SlotIdx].Name)
                return SlotIdx;
    }
    return INDEX_NONE;
}

void UParticleSystemComponent::SetAutoAttachmentParameters(
    USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule, EAttachmentRule ScaleRule) {
    AutoAttachParent = Parent;
    AutoAttachSocketName = SocketName;
    AutoAttachLocationRule = LocationRule;
    AutoAttachRotationRule = RotationRule;
    AutoAttachScaleRule = ScaleRule;
}

}
