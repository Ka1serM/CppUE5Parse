#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModule.h"
#include "Engine/Source/Runtime/Engine/Particles/ParticleModuleLocation.h"

#include "ParticleModuleLocationBoneSocket.generated.h"

namespace ue5parse::runtime {

class AActor;
class UParticleModuleTypeDataBase;
class UParticleSystemComponent;
class USkeletalMeshComponent;
class USkeletalMeshSocket;
struct FModuleLocationBoneSocketInstancePayload;

UENUM()
enum ELocationBoneSocketSource : int32 { BONESOCKETSOURCE_Bones, BONESOCKETSOURCE_Sockets, BONESOCKETSOURCE_MAX };
UENUM()
enum ELocationBoneSocketSelectionMethod : int32 { BONESOCKETSEL_Sequential, BONESOCKETSEL_Random, BONESOCKETSEL_MAX };
USTRUCT()
struct FLocationBoneSocketInfo {
    GENERATED_BODY()
    UPROPERTY()
    FName BoneSocketName;
    UPROPERTY()
    FVector Offset;
};

enum class EBoneSocketSourceIndexMode : uint8 {
    SourceLocations,
    PreSelectedIndices,
    Direct,
};

UCLASS()
class UParticleModuleLocationBoneSocket : public UParticleModuleLocationBase {
    GENERATED_BODY()
public:
    explicit UParticleModuleLocationBoneSocket(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TEnumAsByte<ELocationBoneSocketSource> SourceType = BONESOCKETSOURCE_Sockets;
    UPROPERTY()
    FVector UniversalOffset;
    UPROPERTY()
    TArray<FLocationBoneSocketInfo> SourceLocations;
    UPROPERTY()
    TEnumAsByte<ELocationBoneSocketSelectionMethod> SelectionMethod = BONESOCKETSEL_Sequential;
    UPROPERTY()
    bool bUpdatePositionEachFrame = false;
    UPROPERTY()
    bool bOrientMeshEmitters = true;
    UPROPERTY()
    bool bInheritBoneVelocity = false;
    UPROPERTY()
    float InheritVelocityScale = 1.0f;
    UPROPERTY()
    FName SkelMeshActorParamName = FName("BoneSocketActor");
    UPROPERTY()
    int32 NumPreSelectedIndices = 10;
    EBoneSocketSourceIndexMode SourceIndexMode = EBoneSocketSourceIndexMode::Direct;

    void PostLoad() override;
    void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    void FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    uint32 RequiredBytes(UParticleModuleTypeDataBase* TypeData) override;
    uint32 RequiredBytesPerInstance() override;
    uint32 PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData) override;
    bool TouchesMeshRotation() const override { return true; }
    void GetSkeletalMeshComponentSource(FParticleEmitterInstance* Owner, FModuleLocationBoneSocketInstancePayload* InstancePayload);
    bool GetParticleLocation(FModuleLocationBoneSocketInstancePayload* InstancePayload, FParticleEmitterInstance* Owner,
                             USkeletalMeshComponent* InSkelMeshComponent, int32 InBoneSocketIndex, FVector& OutPosition, FQuat* OutRotation);
    int32 GetMaxSourceIndex(FModuleLocationBoneSocketInstancePayload* Payload, USkeletalMeshComponent* SourceComponent) const;
    bool GetSocketInfoForSourceIndex(FModuleLocationBoneSocketInstancePayload* InstancePayload, USkeletalMeshComponent* SourceComponent, int32 SourceIndex,
                                     USkeletalMeshSocket*& OutSocket, FVector& OutOffset) const;
    bool GetBoneInfoForSourceIndex(FModuleLocationBoneSocketInstancePayload* InstancePayload, USkeletalMeshComponent* SourceComponent, int32 SourceIndex,
                                   FMatrix& OutBoneMatrix, FVector& OutOffset) const;
    int32 SelectNextSpawnIndex(FModuleLocationBoneSocketInstancePayload* InstancePayload, USkeletalMeshComponent* SourceComponent,
                               FRandomStream& InRandomStream);
    void RegeneratePreSelectedIndices(FModuleLocationBoneSocketInstancePayload* InstancePayload, USkeletalMeshComponent* SourceComponent,
                                      FRandomStream& InRandomStream);
    void UpdatePrevBoneLocationsAndVelocities(FModuleLocationBoneSocketInstancePayload* InstancePayload, USkeletalMeshComponent* SourceComponent,
                                              float DeltaTime);
    bool InheritingBoneVelocity() const { return bInheritBoneVelocity && !bUpdatePositionEachFrame; }
    void SetSourceIndexMode();
};

struct FModuleLocationBoneSocketInstancePayload {
    TWeakObjectPtr<USkeletalMeshComponent> SourceComponent;
    TWeakObjectPtr<AActor> CachedActor;
    int32 LastSelectedIndex;
    TPreallocatedArrayProxy<FVector3f> PrevFrameBoneSocketPositions;
    TPreallocatedArrayProxy<FVector3f> BoneSocketVelocities;
    TPreallocatedArrayProxy<int32> PreSelectedBoneSocketIndices;
    void InitArrayProxies(int32 FixedArraySize) {
        const uint32 StructSize = sizeof(FModuleLocationBoneSocketInstancePayload);
        PrevFrameBoneSocketPositions = TPreallocatedArrayProxy<FVector3f>((uint8*)this + StructSize, FixedArraySize);
        uint32 StructOffset = StructSize + (FixedArraySize * sizeof(FVector3f));
        BoneSocketVelocities = TPreallocatedArrayProxy<FVector3f>((uint8*)this + StructOffset, FixedArraySize);
        StructOffset += (FixedArraySize * sizeof(FVector3f));
        PreSelectedBoneSocketIndices = TPreallocatedArrayProxy<int32>((uint8*)this + StructOffset, FixedArraySize);
        StructOffset += (FixedArraySize * sizeof(int32));
    }
};

}
