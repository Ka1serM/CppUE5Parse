#include "Engine/Source/Runtime/Experimental/Chaos/GeometryCollection/ManagedArrayCollection.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Experimental/Chaos/Chaos/ImplicitObject.h"

namespace ue5parse::runtime {

namespace {

template <class T>
constexpr bool CanBulkSerialize = std::is_same_v<T, FVector3f> || std::is_same_v<T, FGuid> || std::is_same_v<T, FIntVector> || std::is_same_v<T, FVector2f>
    || std::is_same_v<T, float> || std::is_same_v<T, FQuat4f> || std::is_same_v<T, bool> || std::is_same_v<T, int32> || std::is_same_v<T, uint8>
    || std::is_same_v<T, FUintVector2> || std::is_same_v<T, FIntVector2> || std::is_same_v<T, FIntVector4>;

template <class T>
void SerializeManagedArray(Chaos::FChaosArchive& Ar, TArray<T>& Array) {
    int32 ArrayVersion = 1;
    Ar << ArrayVersion;
    if constexpr (CanBulkSerialize<T>) {
        if (Ar.CustomVerAtLeast(FDestructionObjectVersion::GUID, FDestructionObjectVersion::BulkSerializeArrays, EGame::GAME_UE4_23)) {
            BulkSerialize(Ar, Array);
            return;
        }
    }
    Ar << Array;
}

}

FArchive& operator<<(FArchive& Ar, FManagedArrayCollection::FGroupInfo& Info) {
    int32 InfoVersion = 4;
    return Ar << InfoVersion << Info.Size;
}

void FManagedArrayCollection::FValueType::Serialize(Chaos::FChaosArchive& Ar) {
    int32 EntryVersion = 4;
    Ar << EntryVersion;
    int32 ArrayTypeAsInt = 0;
    Ar << ArrayTypeAsInt;
    ArrayType = static_cast<EArrayType>(ArrayTypeAsInt);
    if (EntryVersion < 4) {
        int32 ArrayScope = 0;
        Ar << ArrayScope;
    }
    if (EntryVersion >= 2)
        Ar << GroupIndexDependency << Saved;
    if (Ar.IsError())
        return;
    SelectArray(Ar);
    if (Ar.IsError())
        return;
    const bool bAlwaysSerializeValue = Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID,
        FUE5MainStreamObjectVersion::ManagedArrayCollectionAlwaysSerializeValue, EGame::GAME_UE5_1);
    if (!bAlwaysSerializeValue && !Saved)
        return;
    std::visit(
        [&Ar](auto& Array) {
            if constexpr (!std::is_same_v<std::decay_t<decltype(Array)>, std::monostate>)
                SerializeManagedArray(Ar, Array);
        },
        Value);
}

void FManagedArrayCollection::FValueType::SelectArray(Chaos::FChaosArchive& Ar) {
    switch (ArrayType) {
    case EArrayType::FVectorType:
        Value.emplace<TArray<FVector3f>>();
        return;
    case EArrayType::FIntVectorType:
        Value.emplace<TArray<FIntVector>>();
        return;
    case EArrayType::FVector2DType:
        Value.emplace<TArray<FVector2f>>();
        return;
    case EArrayType::FLinearColorType:
        Value.emplace<TArray<FLinearColor>>();
        return;
    case EArrayType::FInt32Type:
        Value.emplace<TArray<int32>>();
        return;
    case EArrayType::FBoolType:
        Value.emplace<TArray<bool>>();
        return;
    case EArrayType::FTransformType:
        Value.emplace<TArray<FTransform>>();
        return;
    case EArrayType::FStringType:
        Value.emplace<TArray<FString>>();
        return;
    case EArrayType::FFloatType:
        Value.emplace<TArray<float>>();
        return;
    case EArrayType::FQuatType:
        Value.emplace<TArray<FQuat4f>>();
        return;
    case EArrayType::FBoneNodeType:
        Value.emplace<TArray<FGeometryCollectionBoneNode>>();
        return;
    case EArrayType::FMeshSectionType:
        Value.emplace<TArray<FGeometryCollectionSection>>();
        return;
    case EArrayType::FBoxType:
        Value.emplace<TArray<FBox>>();
        return;
    case EArrayType::FIntArrayType:
        Value.emplace<TArray<TSet<int32>>>();
        return;
    case EArrayType::FGuidType:
        Value.emplace<TArray<FGuid>>();
        return;
    case EArrayType::FUInt8Type:
        Value.emplace<TArray<uint8>>();
        return;
    case EArrayType::FFImplicitObject3UniquePointerType:
    case EArrayType::FFImplicitObject3SerializablePtrType:
    case EArrayType::FFImplicitObject3ThreadSafeSharedPointerType:
    case EArrayType::FFImplicitObject3SharedPointerType:
    case EArrayType::FFConvexUniquePtrType:
    case EArrayType::FFImplicitObjectRefCountedPtrType:
    case EArrayType::FFConvexRefCountedPtrType:
        Value.emplace<TArray<Chaos::FImplicitObjectPtr>>();
        return;
    case EArrayType::FFBVHParticlesFloat3UniquePointerType:
        Value.emplace<TArray<std::shared_ptr<Chaos::FBVHParticles>>>();
        return;
    case EArrayType::FVector2DArrayType:
        Value.emplace<TArray<TArray<FVector2f>>>();
        return;
    case EArrayType::FDoubleType:
        Value.emplace<TArray<double>>();
        return;
    case EArrayType::FIntVector4Type:
        Value.emplace<TArray<FIntVector4>>();
        return;
    case EArrayType::FVector3dType:
        Value.emplace<TArray<FVector3d>>();
        return;
    case EArrayType::FIntVector2Type:
        Value.emplace<TArray<FIntVector2>>();
        return;
    case EArrayType::FIntVector2ArrayType:
        Value.emplace<TArray<TArray<FIntVector2>>>();
        return;
    case EArrayType::FInt32ArrayType:
        Value.emplace<TArray<TArray<int32>>>();
        return;
    case EArrayType::FFloatArrayType:
        Value.emplace<TArray<TArray<float>>>();
        return;
    case EArrayType::FVector4fType:
        Value.emplace<TArray<FVector4f>>();
        return;
    case EArrayType::FFVectorArrayType:
        Value.emplace<TArray<TArray<FVector3f>>>();
        return;
    case EArrayType::FTransform3fType:
        Value.emplace<TArray<FTransform3f>>();
        return;
    case EArrayType::FIntVector3ArrayType:
        Value.emplace<TArray<TArray<FIntVector>>>();
        return;
    case EArrayType::FVector4fArrayType:
        Value.emplace<TArray<TArray<FVector4f>>>();
        return;
    case EArrayType::FFVector3fNestedArrayType:
        Value.emplace<TArray<TArray<TArray<FVector3f>>>>();
        return;
    case EArrayType::FUintVector2Type:
        Value.emplace<TArray<FUintVector2>>();
        return;
    case EArrayType::FUObjectArrayType:
        Value.emplace<TArray<TObjectPtr<UObject>>>();
        return;
    case EArrayType::FLinearCurveType:
        Value.emplace<TArray<Chaos::FLinearCurve>>();
        return;
    case EArrayType::FNameType:
        Value.emplace<TArray<FName>>();
        return;
    case EArrayType::FSoftObjectPathType:
        Value.emplace<TArray<FSoftObjectPath>>();
        return;
    default:
        Ar.SetError(ErrorCode::UnsupportedVersion, "FManagedArrayCollection.ArrayType");
    }
}

void FManagedArrayCollection::Serialize(Chaos::FChaosArchive& Ar) {
    Ar << Version << GroupInfo;
    const int32 Count = Ar.ReadCount(1);
    Map.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        FKeyType Key;
        Ar << Key;
        Map.Add(Key).Serialize(Ar);
    }
}

}
