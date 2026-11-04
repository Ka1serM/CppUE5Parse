#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLocationBoneSocket.h"

#include <cstring>

#include "Engine/Source/Runtime/Engine/Particles/ParticleEmitterInstances.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleLODLevel.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleRequired.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleSystemComponent.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleTypeData.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"
#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMeshSocket.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMesh.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Engine/Animation/SkeletalMeshActor.h"

namespace ue5parse::runtime {

UParticleModuleLocationBoneSocket::UParticleModuleLocationBoneSocket(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bSpawnModule = true;
    bUpdateModule = true;
    bFinalUpdateModule = true;
    bUpdateForGPUEmitter = true;
    bSupported3DDrawMode = true;
    SourceType = BONESOCKETSOURCE_Sockets;
    SkelMeshActorParamName = FName("BoneSocketActor");
    bOrientMeshEmitters = true;
    SourceIndexMode = EBoneSocketSourceIndexMode::Direct;
    NumPreSelectedIndices = 10;
    InheritVelocityScale = 1.0f;
}

int32 UParticleModuleLocationBoneSocket::SelectNextSpawnIndex(
    FModuleLocationBoneSocketInstancePayload* InstancePayload, USkeletalMeshComponent* SourceComponent, FRandomStream& InRandomStream) {
    int32 SourceIndex = -1;
    int32 MaxIndex = GetMaxSourceIndex(InstancePayload, SourceComponent);
    if (SelectionMethod == BONESOCKETSEL_Sequential || SourceIndexMode == EBoneSocketSourceIndexMode::PreSelectedIndices) {
        SourceIndex = InstancePayload->LastSelectedIndex++;
        if (InstancePayload->LastSelectedIndex >= MaxIndex)
            InstancePayload->LastSelectedIndex = 0;
    } else if (SelectionMethod == BONESOCKETSEL_Random) {
        SourceIndex = FMath::TruncToInt(InRandomStream.FRand() * ((float)MaxIndex - 0.5f));
        InstancePayload->LastSelectedIndex = SourceIndex;
    }
    if (SourceIndex == -1)
        return INDEX_NONE;
    if (SourceIndex >= MaxIndex)
        return INDEX_NONE;
    return SourceIndex;
}

void UParticleModuleLocationBoneSocket::RegeneratePreSelectedIndices(
    FModuleLocationBoneSocketInstancePayload* InstancePayload, USkeletalMeshComponent* SourceComponent, FRandomStream& InRandomStream) {
    if (SourceIndexMode == EBoneSocketSourceIndexMode::PreSelectedIndices) {
        int32 MaxIndex =
            SourceType == BONESOCKETSOURCE_Sockets ? SourceComponent->GetSkeletalMeshAsset()->NumSockets() : SourceComponent->GetNumBones();
        for (int32 i = 0; i < NumPreSelectedIndices; ++i)
            InstancePayload->PreSelectedBoneSocketIndices[i] = FMath::TruncToInt(InRandomStream.FRand() * ((float)MaxIndex - 0.5f));
        if (InheritingBoneVelocity())
            UpdatePrevBoneLocationsAndVelocities(InstancePayload, SourceComponent, 0.0f);
    }
}

void UParticleModuleLocationBoneSocket::SetSourceIndexMode() {
    if (SourceLocations.Num() > 0)
        SourceIndexMode = EBoneSocketSourceIndexMode::SourceLocations;
    else if (InheritingBoneVelocity())
        SourceIndexMode = EBoneSocketSourceIndexMode::PreSelectedIndices;
    else
        SourceIndexMode = EBoneSocketSourceIndexMode::Direct;
}

void UParticleModuleLocationBoneSocket::PostLoad() {
    Super::PostLoad();
    SetSourceIndexMode();
}

void UParticleModuleLocationBoneSocket::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) {
    FModuleLocationBoneSocketInstancePayload* InstancePayload = (FModuleLocationBoneSocketInstancePayload*)(Owner->GetModuleInstanceData(this));
    if (InstancePayload == nullptr)
        return;
    FRandomStream& RandomStream = GetRandomStream(Owner);
    GetSkeletalMeshComponentSource(Owner, InstancePayload);
    if (!InstancePayload->SourceComponent.IsValid())
        return;
    USkeletalMeshComponent* SourceComponent = InstancePayload->SourceComponent.Get();
    int32 SourceIndex = SelectNextSpawnIndex(InstancePayload, SourceComponent, RandomStream);
    if (SourceIndex == INDEX_NONE)
        return;
    FVector SourceLocation;
    FQuat RotationQuat = FQuat::Identity;
    const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
    const bool bMeshRotationActive = MeshRotationOffset > 0 && Owner->IsMeshRotationActive();
    FQuat* SourceRotation = (bMeshRotationActive) ? nullptr : &RotationQuat;
    if (GetParticleLocation(InstancePayload, Owner, SourceComponent, SourceIndex, SourceLocation, SourceRotation)) {
        SPAWN_INIT {
            FModuleLocationBoneSocketParticlePayload* ParticlePayload = (FModuleLocationBoneSocketParticlePayload*)((uint8*)&Particle + Offset);
            ParticlePayload->SourceIndex = SourceIndex;
            Particle.Location = SourceLocation;
            if (InheritingBoneVelocity()) {
                Particle.BaseVelocity =
                    FMath::Lerp<FVector3f>(Particle.BaseVelocity, InstancePayload->BoneSocketVelocities[SourceIndex], InheritVelocityScale);
            }
            if (bMeshRotationActive) {
                FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
                PayloadData->Rotation = FVector3f(RotationQuat.Euler());
                if (Owner->CurrentLODLevel->RequiredModule->bUseLocalSpace) {
                    PayloadData->Rotation =
                        FVector3f(Owner->Component->GetComponentTransform().InverseTransformVectorNoScale((FVector)PayloadData->Rotation));
                }
            }
        }
    }
}

void UParticleModuleLocationBoneSocket::UpdatePrevBoneLocationsAndVelocities(
    FModuleLocationBoneSocketInstancePayload* InstancePayload, USkeletalMeshComponent* SourceComponent, float DeltaTime) {
    const float InvDeltaTime = (DeltaTime > 0.0f) ? 1.0f / DeltaTime : 0.0f;
    int32 MaxIndex = GetMaxSourceIndex(InstancePayload, SourceComponent);
    FMatrix WorldBoneTM;
    FVector Offset;
    for (int32 SourceIndex = 0; SourceIndex < MaxIndex; ++SourceIndex) {
        if (GetBoneInfoForSourceIndex(InstancePayload, SourceComponent, SourceIndex, WorldBoneTM, Offset) &&
            SourceIndex < InstancePayload->BoneSocketVelocities.Num()) {
            const FVector3f CurrLocation = FVector3f(WorldBoneTM.GetOrigin());
            const FVector3f Diff = CurrLocation - InstancePayload->PrevFrameBoneSocketPositions[SourceIndex];
            InstancePayload->BoneSocketVelocities[SourceIndex] = Diff * InvDeltaTime;
            InstancePayload->PrevFrameBoneSocketPositions[SourceIndex] = CurrLocation;
        } else {
            InstancePayload->BoneSocketVelocities[SourceIndex] = FVector3f::ZeroVector;
            InstancePayload->PrevFrameBoneSocketPositions[SourceIndex] = FVector3f(SourceComponent->GetComponentLocation());
        }
    }
}

void UParticleModuleLocationBoneSocket::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    FModuleLocationBoneSocketInstancePayload* InstancePayload = (FModuleLocationBoneSocketInstancePayload*)(Owner->GetModuleInstanceData(this));
    if (!InstancePayload->SourceComponent.IsValid())
        return;
    USkeletalMeshComponent* SourceComponent = InstancePayload->SourceComponent.Get();
    if (InheritingBoneVelocity())
        UpdatePrevBoneLocationsAndVelocities(InstancePayload, SourceComponent, DeltaTime);
    if (bUpdatePositionEachFrame == false)
        return;
    if (Owner->ParticleData == nullptr)
        return;
    FVector SourceLocation;
    FQuat RotationQuat = FQuat::Identity;
    const int32 MeshRotationOffset = Owner->GetMeshRotationOffset();
    const bool bMeshRotationActive = MeshRotationOffset > 0 && Owner->IsMeshRotationActive();
    FQuat* SourceRotation = (bMeshRotationActive) ? nullptr : &RotationQuat;
    const FTransform& OwnerTM = Owner->Component->GetComponentTransform();
    BEGIN_UPDATE_LOOP;
    {
        FModuleLocationBoneSocketParticlePayload* ParticlePayload = (FModuleLocationBoneSocketParticlePayload*)((uint8*)&Particle + Offset);
        if (GetParticleLocation(InstancePayload, Owner, SourceComponent, ParticlePayload->SourceIndex, SourceLocation, SourceRotation)) {
            Particle.Location = SourceLocation;
            if (bMeshRotationActive) {
                FMeshRotationPayloadData* PayloadData = (FMeshRotationPayloadData*)((uint8*)&Particle + MeshRotationOffset);
                PayloadData->Rotation = FVector3f(RotationQuat.Euler());
                if (Owner->CurrentLODLevel->RequiredModule->bUseLocalSpace)
                    PayloadData->Rotation = FVector3f(OwnerTM.InverseTransformVectorNoScale((FVector)PayloadData->Rotation));
            }
        }
    }
    END_UPDATE_LOOP;
}

void UParticleModuleLocationBoneSocket::FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {
    Super::FinalUpdate(Owner, Offset, DeltaTime);
    FModuleLocationBoneSocketInstancePayload* InstancePayload = (FModuleLocationBoneSocketInstancePayload*)(Owner->GetModuleInstanceData(this));
    if (!InstancePayload->SourceComponent.IsValid())
        return;
    USkeletalMeshComponent* SourceComponent = InstancePayload->SourceComponent.Get();
    if (Owner->ParticleData) {
        if (SourceType == BONESOCKETSOURCE_Sockets) {
            bool bHaveDeadParticles = false;
            BEGIN_UPDATE_LOOP;
            {
                FModuleLocationBoneSocketParticlePayload* ParticlePayload = (FModuleLocationBoneSocketParticlePayload*)((uint8*)&Particle + Offset);
                if (SourceComponent && SourceComponent->GetSkeletalMeshAsset()) {
                    USkeletalMeshSocket* Socket;
                    FVector SocketOffset;
                    if (GetSocketInfoForSourceIndex(InstancePayload, SourceComponent, ParticlePayload->SourceIndex, Socket, SocketOffset)) {
                        int32 BoneIndex = SourceComponent->GetBoneIndex(Socket->BoneName);
                        if (BoneIndex != INDEX_NONE) {
                            if ((SourceComponent->IsBoneHidden(BoneIndex)) ||
                                (SourceComponent->GetBoneTransform(BoneIndex).GetScale3D() == FVector::ZeroVector)) {
                                Particle.RelativeTime = 1.1f;
                                bHaveDeadParticles = true;
                            }
                        }
                    }
                }
            }
            END_UPDATE_LOOP;
            if (bHaveDeadParticles)
                Owner->KillParticles();
        }
    }
    RegeneratePreSelectedIndices(InstancePayload, SourceComponent, GetRandomStream(Owner));
}

uint32 UParticleModuleLocationBoneSocket::RequiredBytes(UParticleModuleTypeDataBase* TypeData) {
    return sizeof(FModuleLocationBoneSocketParticlePayload);
}

uint32 UParticleModuleLocationBoneSocket::RequiredBytesPerInstance() {
    SetSourceIndexMode();
    int32 ArraySize = FMath::Max(SourceLocations.Num(), NumPreSelectedIndices);
    int32 ElemSize = (sizeof(FVector3f) * 2) + sizeof(int32);
    int32 BoneArraySize = ArraySize * ElemSize;
    return sizeof(FModuleLocationBoneSocketInstancePayload) + BoneArraySize;
}

uint32 UParticleModuleLocationBoneSocket::PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) {
    FModuleLocationBoneSocketInstancePayload* Payload = (FModuleLocationBoneSocketInstancePayload*)InstData;
    if (Payload) {
        std::memset(Payload, sizeof(FModuleLocationBoneSocketInstancePayload), 0);
        int32 ArraySize = FMath::Max(SourceLocations.Num(), NumPreSelectedIndices);
        if (ArraySize > 0)
            Payload->InitArrayProxies(ArraySize);
    }
    return 0xffffffff;
}

int32 UParticleModuleLocationBoneSocket::GetMaxSourceIndex(
    FModuleLocationBoneSocketInstancePayload* Payload, USkeletalMeshComponent* SourceComponent) const {
    switch (SourceIndexMode) {
    case EBoneSocketSourceIndexMode::SourceLocations: {
        return SourceLocations.Num();
    } break;
    case EBoneSocketSourceIndexMode::PreSelectedIndices: {
        return NumPreSelectedIndices;
    } break;
    case EBoneSocketSourceIndexMode::Direct: {
        if (SourceType == BONESOCKETSOURCE_Sockets) {
            if ((SourceComponent->GetSkeletalMeshAsset()))
                return SourceComponent->GetSkeletalMeshAsset()->NumSockets();
            return 0;
        } else {
            return SourceComponent->GetNumBones();
        }
    } break;
    }
    return 0;
}

void UParticleModuleLocationBoneSocket::GetSkeletalMeshComponentSource(
    FParticleEmitterInstance* Owner, FModuleLocationBoneSocketInstancePayload* InstancePayload) {
    if (Owner == nullptr) {
        InstancePayload->SourceComponent = nullptr;
        return;
    }
    UParticleSystemComponent* PSysComp = Owner->Component;
    if (PSysComp == nullptr) {
        InstancePayload->SourceComponent = nullptr;
        return;
    }
    USkeletalMeshComponent* NewSkelComp = nullptr;
    AActor* Actor = nullptr;
    PSysComp->GetActorParameter(SkelMeshActorParamName, Actor);
    USkeletalMeshComponent* AttachParentMesh = Cast<USkeletalMeshComponent>(PSysComp->GetAttachParent());
    bool bActorChanged = Actor != InstancePayload->CachedActor.Get();
    bool bAttachParentChanged = (AttachParentMesh && AttachParentMesh != InstancePayload->SourceComponent.Get());
    if (!InstancePayload->SourceComponent.IsValid() || bActorChanged || bAttachParentChanged) {
        InstancePayload->SourceComponent = nullptr;
        InstancePayload->CachedActor = Actor;
        if (Actor) {
            ASkeletalMeshActor* SkelMeshActor = Cast<ASkeletalMeshActor>(Actor);
            if (SkelMeshActor != nullptr) {
                NewSkelComp = SkelMeshActor->GetSkeletalMeshComponent();
            } else if (Actor) {
                USkeletalMeshComponent* SkeletalMeshComponent = Actor->FindComponentByClass<USkeletalMeshComponent>();
                if (SkeletalMeshComponent)
                    NewSkelComp = SkeletalMeshComponent;
            }
        }
        if (AttachParentMesh)
            NewSkelComp = AttachParentMesh;
        if (NewSkelComp) {
            InstancePayload->SourceComponent = NewSkelComp;
            RegeneratePreSelectedIndices(InstancePayload, NewSkelComp, GetRandomStream(Owner));
        }
    }
}

bool UParticleModuleLocationBoneSocket::GetSocketInfoForSourceIndex(FModuleLocationBoneSocketInstancePayload* InstancePayload,
    USkeletalMeshComponent* SourceComponent, int32 SourceIndex, USkeletalMeshSocket*& OutSocket, FVector& OutOffset) const {
    if (!(SourceType == BONESOCKETSOURCE_Sockets) || !(SourceComponent) || !(SourceComponent->GetSkeletalMeshAsset()))
        return false;
    switch (SourceIndexMode) {
    case EBoneSocketSourceIndexMode::SourceLocations: {
        if ((SourceLocations.IsValidIndex(SourceIndex))) {
            OutSocket = SourceComponent->GetSkeletalMeshAsset()->FindSocket(SourceLocations[SourceIndex].BoneSocketName);
            OutOffset = SourceLocations[SourceIndex].Offset + UniversalOffset;
        } else {
            return false;
        }
    } break;
    case EBoneSocketSourceIndexMode::PreSelectedIndices: {
        if ((InstancePayload) && (SourceIndex >= 0 && SourceIndex < InstancePayload->PreSelectedBoneSocketIndices.Num())) {
            OutSocket = SourceComponent->GetSkeletalMeshAsset()->GetSocketByIndex(InstancePayload->PreSelectedBoneSocketIndices[SourceIndex]);
            OutOffset = UniversalOffset;
        } else {
            return false;
        }
    } break;
    case EBoneSocketSourceIndexMode::Direct: {
        OutSocket = SourceComponent->GetSkeletalMeshAsset()->GetSocketByIndex(SourceIndex);
        OutOffset = UniversalOffset;
    } break;
    }
    return OutSocket != nullptr;
}

bool UParticleModuleLocationBoneSocket::GetBoneInfoForSourceIndex(FModuleLocationBoneSocketInstancePayload* InstancePayload,
    USkeletalMeshComponent* SourceComponent, int32 SourceIndex, FMatrix& OutBoneMatrix, FVector& OutOffset) const {
    int32 BoneIndex = INDEX_NONE;
    FVector Offset;
    if (SourceType == BONESOCKETSOURCE_Sockets) {
        USkeletalMeshSocket* Socket = nullptr;
        if (GetSocketInfoForSourceIndex(InstancePayload, SourceComponent, SourceIndex, Socket, Offset)) {
            BoneIndex = SourceComponent->GetBoneIndex(Socket->BoneName);
            return false;
        }
    } else {
        switch (SourceIndexMode) {
        case EBoneSocketSourceIndexMode::SourceLocations: {
            BoneIndex = SourceComponent->GetBoneIndex(SourceLocations[SourceIndex].BoneSocketName);
            Offset = SourceLocations[SourceIndex].Offset + UniversalOffset;
        } break;
        case EBoneSocketSourceIndexMode::PreSelectedIndices: {
            BoneIndex = InstancePayload->PreSelectedBoneSocketIndices[SourceIndex];
            Offset = UniversalOffset;
        } break;
        case EBoneSocketSourceIndexMode::Direct: {
            BoneIndex = SourceIndex;
            Offset = UniversalOffset;
        } break;
        }
    }
    if (BoneIndex != INDEX_NONE) {
        OutBoneMatrix = SourceComponent->GetBoneMatrix(BoneIndex);
        OutOffset = Offset;
        return true;
    }
    return false;
}

bool UParticleModuleLocationBoneSocket::GetParticleLocation(FModuleLocationBoneSocketInstancePayload* InstancePayload,
    FParticleEmitterInstance* Owner, USkeletalMeshComponent* InSkelMeshComponent, int32 InBoneSocketIndex, FVector& OutPosition, FQuat* OutRotation) {
    if (SourceType == BONESOCKETSOURCE_Sockets) {
        if (InSkelMeshComponent->GetSkeletalMeshAsset()) {
            USkeletalMeshSocket* Socket;
            FVector SocketOffset;
            if (GetSocketInfoForSourceIndex(InstancePayload, InSkelMeshComponent, InBoneSocketIndex, Socket, SocketOffset)) {
                FRotator SocketRotator(0, 0, 0);
                FMatrix SocketMatrix;
                if (Socket->GetSocketMatrixWithOffset(SocketMatrix, InSkelMeshComponent, SocketOffset, SocketRotator) == false)
                    return false;
                OutPosition = SocketMatrix.GetOrigin();
                if (OutRotation != nullptr) {
                    SocketMatrix.RemoveScaling();
                    *OutRotation = SocketMatrix.ToQuat();
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else {
        FVector SocketOffset;
        FMatrix WorldBoneTM;
        if (GetBoneInfoForSourceIndex(InstancePayload, InSkelMeshComponent, InBoneSocketIndex, WorldBoneTM, SocketOffset)) {
            FTranslationMatrix OffsetMatrix(SocketOffset);
            FMatrix ResultMatrix = OffsetMatrix * WorldBoneTM;
            OutPosition = ResultMatrix.GetOrigin();
            if (OutRotation != nullptr) {
                ResultMatrix.RemoveScaling();
                *OutRotation = ResultMatrix.ToQuat();
            }
        } else {
            return false;
        }
    }
    if (Owner->CurrentLODLevel->RequiredModule->bUseLocalSpace)
        OutPosition = Owner->Component->GetComponentTransform().InverseTransformPosition(OutPosition);
    return true;
}

}
