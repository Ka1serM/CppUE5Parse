#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/RigidBodyIndexPair.h"

#include "PhysicsAsset.generated.h"

namespace ue5parse::runtime {

class UAssetUserData;
class UBodySetup;
class UPhysicsConstraintTemplate;
class USkeletalBodySetup;
class USkeletalMesh;

USTRUCT()
struct FSolverIterations {
    GENERATED_BODY()
    UPROPERTY()
    float FixedTimeStep = 0.0f;
    UPROPERTY()
    int32 SolverIterations = 3;
    UPROPERTY()
    int32 JointIterations = 2;
    UPROPERTY()
    int32 CollisionIterations = 2;
    UPROPERTY()
    int32 SolverPushOutIterations = 1;
    UPROPERTY()
    int32 JointPushOutIterations = 1;
    UPROPERTY()
    int32 CollisionPushOutIterations = 0;
};

USTRUCT()
struct FPhysicsAssetSolverSettings {
    GENERATED_BODY()
    UPROPERTY()
    int32 PositionIterations = 6;
    UPROPERTY()
    int32 VelocityIterations = 1;
    UPROPERTY()
    int32 ProjectionIterations = 1;
    UPROPERTY()
    float CullDistance = 3.0f;
    UPROPERTY()
    float MaxDepenetrationVelocity = 0.0f;
    UPROPERTY()
    float FixedTimeStep = 0.0f;
    UPROPERTY()
    bool bUseLinearJointSolver = true;
    UPROPERTY()
    bool bUseManifolds = true;
};

UENUM()
enum class EPhysicsAssetSolverType : uint8 { RBAN, World };

UCLASS()
class UPhysicsAsset : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSoftObjectPtr<USkeletalMesh> PreviewSkeletalMesh;
    UPROPERTY()
    TArray<int32> BoundsBodies;
    UPROPERTY()
    TArray<TObjectPtr<UPhysicsConstraintTemplate>> ConstraintSetup;
    UPROPERTY()
    TArray<FName> PhysicalAnimationProfiles;
    UPROPERTY()
    TArray<FName> ConstraintProfiles;
    UPROPERTY()
    FName CurrentPhysicalAnimationProfileName;
    UPROPERTY()
    FName CurrentConstraintProfileName;
    UPROPERTY()
    TArray<TObjectPtr<USkeletalBodySetup>> SkeletalBodySetups;
    //! UE 4.3 to 4.15 store the bodies as BodySetup.
    UPROPERTY()
    TArray<TObjectPtr<UBodySetup>> BodySetup;
    UPROPERTY()
    bool bUseAsyncScene = false;
    UPROPERTY()
    FSolverIterations SolverIterations;
    UPROPERTY()
    bool bNotForDedicatedServer = false;
    UPROPERTY()
    TObjectPtr<UObject> ThumbnailInfo = nullptr;
    UPROPERTY()
    TArray<int32> EditorBodyFlags;
    UPROPERTY()
    FPhysicsAssetSolverSettings SolverSettings;
    UPROPERTY()
    EPhysicsAssetSolverType SolverType = EPhysicsAssetSolverType::RBAN;
    UPROPERTY()
    TArray<TObjectPtr<UAssetUserData>> AssetUserData;

    TMap<FRigidBodyIndexPair, bool> CollisionDisableTable;

    void Serialize(FArchive& Ar) override;
};

}
