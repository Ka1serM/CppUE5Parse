#pragma once

#include <variant>

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/BVHParticles.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitFwd.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/Curve.h"
#include "Engine/Source/Runtime/Experimental/Chaos/GeometryCollection/GeometryCollectionBoneNode.h"
#include "Engine/Source/Runtime/Experimental/Chaos/GeometryCollection/GeometryCollectionSection.h"

namespace ue5parse::runtime {

class UObject;

class FManagedArrayCollection {
public:
    enum class EArrayType : int32 {
        FNoneType,
        FVectorType,
        FIntVectorType,
        FVector2DType,
        FLinearColorType,
        FInt32Type,
        FBoolType,
        FTransformType,
        FStringType,
        FFloatType,
        FQuatType,
        FBoneNodeType,
        FMeshSectionType,
        FBoxType,
        FIntArrayType,
        FGuidType,
        FUInt8Type,
        FVectorArrayPointerType,
        FVectorArrayUniquePointerType,
        FFImplicitObject3PointerType,
        FFImplicitObject3UniquePointerType,
        FFImplicitObject3SerializablePtrType,
        FFBVHParticlesFloat3PointerType,
        FFBVHParticlesFloat3UniquePointerType,
        FTPBDRigidParticleHandle3fPtrType,
        FTPBDGeometryCollectionParticleHandle3fPtrType,
        FTGeometryParticle3fUniquePtrType,
        FFImplicitObject3ThreadSafeSharedPointerType,
        FFImplicitObject3SharedPointerType,
        FTPBDRigidClusteredParticleHandle3fPtrType,
        FFConvexUniquePtrType,
        FVector2DArrayType,
        FDoubleType,
        FIntVector4Type,
        FVector3dType,
        FIntVector2Type,
        FIntVector2ArrayType,
        FInt32ArrayType,
        FFloatArrayType,
        FVector4fType,
        FFVectorArrayType,
        FTPBDRigidParticle3fUniquePtrType,
        FFImplicitObjectRefCountedPtrType,
        FFConvexRefCountedPtrType,
        FTransform3fType,
        FIntVector3ArrayType,
        FVector4fArrayType,
        FPMatrix33dType,
        FPMatrix33dArrayType,
        FFVector3fNestedArrayType,
        FUintVector2Type,
        FUObjectArrayType,
        FLinearCurveType,
        FNameType,
        FSoftObjectPathType,
    };

    using FManagedArray = std::variant<std::monostate, TArray<FVector3f>, TArray<FIntVector>, TArray<FVector2f>, TArray<FLinearColor>, TArray<int32>, TArray<bool>,
        TArray<FTransform>, TArray<FString>, TArray<float>, TArray<FQuat4f>, TArray<FGeometryCollectionBoneNode>, TArray<FGeometryCollectionSection>, TArray<FBox>,
        TArray<TSet<int32>>, TArray<FGuid>, TArray<uint8>, TArray<Chaos::FImplicitObjectPtr>, TArray<TArray<FVector2f>>, TArray<double>, TArray<FIntVector4>,
        TArray<FVector3d>, TArray<FIntVector2>, TArray<TArray<FIntVector2>>, TArray<TArray<int32>>, TArray<TArray<float>>, TArray<FVector4f>,
        TArray<TArray<FVector3f>>, TArray<FTransform3f>, TArray<TArray<FIntVector>>, TArray<TArray<FVector4f>>, TArray<TArray<TArray<FVector3f>>>,
        TArray<FUintVector2>, TArray<TObjectPtr<UObject>>, TArray<Chaos::FLinearCurve>, TArray<FName>, TArray<FSoftObjectPath>, TArray<std::shared_ptr<Chaos::FBVHParticles>>>;

    using FKeyType = TPair<FName, FName>;

    struct FGroupInfo {
        int32 Size = 0;

        friend FArchive& operator<<(FArchive& Ar, FGroupInfo& Info);
    };

    struct FValueType {
        EArrayType ArrayType = EArrayType::FNoneType;
        FName GroupIndexDependency;
        bool Saved = false;
        FManagedArray Value;

        void Serialize(Chaos::FChaosArchive& Ar);

    private:
        void SelectArray(Chaos::FChaosArchive& Ar);
    };

    void Serialize(Chaos::FChaosArchive& Ar);

    const TMap<FName, FGroupInfo>& GetGroupInfo() const { return GroupInfo; }
    const TMap<FKeyType, FValueType>& GetAttributes() const { return Map; }

private:
    int32 Version = 0;
    TMap<FName, FGroupInfo> GroupInfo;
    TMap<FKeyType, FValueType> Map;
};

}
