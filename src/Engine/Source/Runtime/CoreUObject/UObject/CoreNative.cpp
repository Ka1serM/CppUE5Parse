#include <algorithm>
#include <atomic>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/Source/Runtime/Core/Math/Axis.h"
#include "Engine/Source/Runtime/Core/Math/InterpCurve.h"
#include "Engine/Source/Runtime/Core/PixelFormat.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/CoreUObjectPrivate.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStructContainer.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealTypePrivate.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFrameMigration.h"

//! A struct that cooked data serializes natively must have its C++ operations here, else its holder's stream cannot be read.

namespace ue5parse::runtime {

void InitializeScriptCore();

namespace {

template <class T>
void OpConstruct(void* Dest) {
    new (Dest) T();
}
template <class T>
void OpDestruct(void* Dest) {
    static_cast<T*>(Dest)->~T();
}
template <class T>
void OpCopy(void* Dest, const void* Src) {
    *static_cast<T*>(Dest) = *static_cast<const T*>(Src);
}
template <class T>
bool OpIdenticalBytes(const void* A, const void* B) {
    return std::memcmp(A, B, sizeof(T)) == 0;
}
template <class T, class TElement>
bool OpIdenticalElements(const void* A, const void* B) {
    const TElement* ElementsA = static_cast<const TElement*>(A);
    const TElement* ElementsB = static_cast<const TElement*>(B);
    for (std::size_t i = 0; i < sizeof(T) / sizeof(TElement); i++)
        if (!(ElementsA[i] == ElementsB[i]))
            return false;
    return true;
}
template <class T>
uint32 OpHashBytes(const void* Src) {
    uint32 Hash = 2166136261u;
    const uint8* Bytes = static_cast<const uint8*>(Src);
    for (std::size_t i = 0; i < sizeof(T); i++)
        Hash = (Hash ^ Bytes[i]) * 16777619u;
    return Hash;
}
template <class T>
bool OpSerialize(FArchive& Ar, void* Data) {
    Ar << *static_cast<T*>(Data);
    return true;
}

template <class T, class TElement>
constexpr FCppStructOps PlainStructOps() {
    FCppStructOps Ops;
    Ops.Size = sizeof(T);
    Ops.Alignment = alignof(T);
    Ops.Copy = &OpCopy<T>;
    Ops.Identical = &OpIdenticalElements<T, TElement>;
    Ops.GetTypeHash = &OpHashBytes<T>;
    Ops.Serialize = &OpSerialize<T>;
    Ops.bIsPlainOldData = true;
    Ops.bHasZeroConstructor = true;
    return Ops;
}

constexpr FName NAME_StructProperty("StructProperty");
constexpr FName NAME_Vector4("Vector4");
constexpr FName NAME_Vector4d("Vector4d");

//! UE_SERIALIZE_VARIANT_FROM_MISMATCHED_TAG: a float math struct loads a tag of its double variant and narrows the value.
template <class T, class TDouble, const FName* AliasName, const FName* DoubleName>
bool VariantSerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar, void* Data) {
    const FName StructName = Tag.TypeName.GetParameterName(0);
    if (Tag.Type != NAME_StructProperty || (StructName != *AliasName && StructName != *DoubleName))
        return false;
    TDouble Value;
    Ar << Value;
    *static_cast<T*>(Data) = T(Value);
    return true;
}

template <class T, class TDouble, const FName* AliasName, const FName* DoubleName>
constexpr FCppStructOps FloatVariantStructOps() {
    FCppStructOps Ops = PlainStructOps<T, float>();
    Ops.SerializeFromMismatchedTag = &VariantSerializeFromMismatchedTag<T, TDouble, AliasName, DoubleName>;
    return Ops;
}

template <int32 N>
struct FOpaqueStruct {
    uint8 Bytes[N > 0 ? N : 1];
};
template <int32 N>
bool OpaqueSerialize(FArchive& Ar, void* Data) {
    if constexpr (N > 0)
        Ar.Serialize(Data, N);
    return true;
}
template <int32 N>
constexpr FCppStructOps OpaqueStructOps() {
    FCppStructOps Ops;
    Ops.Size = sizeof(FOpaqueStruct<N>);
    Ops.Alignment = 1;
    Ops.Copy = &OpCopy<FOpaqueStruct<N>>;
    Ops.Identical = &OpIdenticalBytes<FOpaqueStruct<N>>;
    Ops.GetTypeHash = &OpHashBytes<FOpaqueStruct<N>>;
    Ops.Serialize = &OpaqueSerialize<N>;
    Ops.bIsPlainOldData = true;
    Ops.bHasZeroConstructor = true;
    return Ops;
}

template <class T>
constexpr FCppStructOps ObjectStructOps(bool (*Serialize)(FArchive&, void*), bool (*Identical)(const void*, const void*)) {
    FCppStructOps Ops;
    Ops.Size = sizeof(T);
    Ops.Alignment = alignof(T);
    Ops.Construct = &OpConstruct<T>;
    Ops.Destruct = &OpDestruct<T>;
    Ops.Copy = &OpCopy<T>;
    Ops.Identical = Identical;
    Ops.Serialize = Serialize;
    return Ops;
}

const FCppStructOps VectorOps = PlainStructOps<FVector, double>();
const FCppStructOps Vector2DOps = PlainStructOps<FVector2D, double>();
const FCppStructOps Vector4Ops = PlainStructOps<FVector4, double>();
const FCppStructOps Vector3fOps = PlainStructOps<FVector3f, float>();
const FCppStructOps Vector2fOps = PlainStructOps<FVector2f, float>();
const FCppStructOps Vector4fOps = FloatVariantStructOps<FVector4f, FVector4, &NAME_Vector4, &NAME_Vector4d>();
const FCppStructOps RotatorOps = PlainStructOps<FRotator, double>();
const FCppStructOps Rotator3fOps = PlainStructOps<FRotator3f, float>();
const FCppStructOps QuatOps = PlainStructOps<FQuat, double>();
const FCppStructOps Quat4fOps = PlainStructOps<FQuat4f, float>();
const FCppStructOps PlaneOps = PlainStructOps<FPlane, double>();
const FCppStructOps Plane4fOps = PlainStructOps<FPlane4f, float>();
const FCppStructOps SphereOps = PlainStructOps<FSphere, double>();
const FCppStructOps Sphere3fOps = PlainStructOps<FSphere3f, float>();
const FCppStructOps MatrixOps = PlainStructOps<FMatrix, double>();
const FCppStructOps Matrix44fOps = PlainStructOps<FMatrix44f, float>();
const FCppStructOps LinearColorOps = PlainStructOps<FLinearColor, float>();
const FCppStructOps ColorOps = PlainStructOps<FColor, uint8>();
const FCppStructOps GuidOps = PlainStructOps<FGuid, uint32>();
const FCppStructOps IntPointOps = PlainStructOps<FIntPoint, int32>();
const FCppStructOps UintPointOps = PlainStructOps<FUintPoint, uint32>();
const FCppStructOps Int64PointOps = PlainStructOps<FInt64Point, int64>();
const FCppStructOps IntVectorOps = PlainStructOps<FIntVector, int32>();
const FCppStructOps UintVectorOps = PlainStructOps<FUintVector, uint32>();
const FCppStructOps Int64VectorOps = PlainStructOps<FInt64Vector, int64>();
const FCppStructOps IntVector4Ops = PlainStructOps<FIntVector4, int32>();
const FCppStructOps UintVector4Ops = PlainStructOps<FUintVector4, uint32>();
const FCppStructOps IntVector2Ops = PlainStructOps<FIntVector2, int32>();
const FCppStructOps UintVector2Ops = PlainStructOps<FUintVector2, uint32>();
const FCppStructOps DateTimeOps = PlainStructOps<FDateTime, int64>();
const FCppStructOps TimespanOps = PlainStructOps<FTimespan, int64>();
const FCppStructOps FrameNumberOps = PlainStructOps<FFrameNumber, int32>();
constexpr FCppStructOps PropertyStructOps(FCppStructOps Ops) {
    Ops.Serialize = nullptr;
    return Ops;
}
FCppStructOps MakeFrameRateOps() {
    FCppStructOps Ops = PropertyStructOps(PlainStructOps<FFrameRate, int32>());
    Ops.Construct = &OpConstruct<FFrameRate>;
    Ops.bHasZeroConstructor = false;
    return Ops;
}
const FCppStructOps FrameRateOps = MakeFrameRateOps();

FCppStructOps MakeInputDeviceIdOps() {
    FCppStructOps Ops = PropertyStructOps(PlainStructOps<FInputDeviceId, int32>());
    Ops.Construct = &OpConstruct<FInputDeviceId>;
    Ops.bHasZeroConstructor = false;
    return Ops;
}
const FCppStructOps InputDeviceIdOps = MakeInputDeviceIdOps();

bool BoxIdentical(const void* A, const void* B) {
    return *static_cast<const FBox*>(A) == *static_cast<const FBox*>(B);
}
FCppStructOps MakeBoxOps() {
    FCppStructOps Ops = PlainStructOps<FBox, double>();
    Ops.Identical = &BoxIdentical;
    return Ops;
}
const FCppStructOps BoxOps = MakeBoxOps();

template <class T>
bool Box2Identical(const void* A, const void* B) {
    return *static_cast<const TBox2<T>*>(A) == *static_cast<const TBox2<T>*>(B);
}
template <class T>
FCppStructOps MakeBox2Ops() {
    FCppStructOps Ops = PlainStructOps<TBox2<T>, T>();
    Ops.Identical = &Box2Identical<T>;
    return Ops;
}
const FCppStructOps Box2DOps = MakeBox2Ops<double>();
const FCppStructOps Box2fOps = MakeBox2Ops<float>();

bool Box3fIdentical(const void* A, const void* B) {
    return *static_cast<const FBox3f*>(A) == *static_cast<const FBox3f*>(B);
}
FCppStructOps MakeBox3fOps() {
    FCppStructOps Ops = PlainStructOps<FBox3f, float>();
    Ops.Identical = &Box3fIdentical;
    return Ops;
}
const FCppStructOps Box3fOps = MakeBox3fOps();

bool TwoVectorsSerialize(FArchive& Ar, void* Data) {
    FTwoVectors& Value = *static_cast<FTwoVectors*>(Data);
    Ar << Value.v1 << Value.v2;
    return true;
}
FCppStructOps MakeTwoVectorsOps() {
    FCppStructOps Ops = PlainStructOps<FTwoVectors, double>();
    Ops.Serialize = &TwoVectorsSerialize;
    return Ops;
}
const FCppStructOps TwoVectorsOps = MakeTwoVectorsOps();

template <class T, class TElement>
FCppStructOps MakeTransformOps() {
    FCppStructOps Ops = PlainStructOps<T, TElement>();
    Ops.Construct = &OpConstruct<T>;
    Ops.bHasZeroConstructor = false;
    Ops.Serialize = nullptr;
    return Ops;
}
const FCppStructOps TransformOps = MakeTransformOps<FTransform, double>();
const FCppStructOps Transform3fOps = MakeTransformOps<FTransform3f, float>();

FCppStructOps MakeRandomStreamOps() {
    FCppStructOps Ops;
    Ops.Size = sizeof(FRandomStream);
    Ops.Alignment = alignof(FRandomStream);
    Ops.Copy = &OpCopy<FRandomStream>;
    Ops.Identical = &OpIdenticalElements<FRandomStream, int32>;
    Ops.GetTypeHash = &OpHashBytes<FRandomStream>;
    Ops.bIsPlainOldData = true;
    Ops.bHasZeroConstructor = true;
    return Ops;
}
const FCppStructOps RandomStreamOps = MakeRandomStreamOps();

FCppStructOps MakeBoxSphereBoundsOps() {
    FCppStructOps Ops = PlainStructOps<FBoxSphereBounds, double>();
    Ops.Serialize = nullptr;
    return Ops;
}
const FCppStructOps BoxSphereBoundsOps = MakeBoxSphereBoundsOps();

bool SoftObjectPathIdentical(const void* A, const void* B) {
    return *static_cast<const FSoftObjectPath*>(A) == *static_cast<const FSoftObjectPath*>(B);
}
uint32 SoftObjectPathHash(const void* Src) {
    return GetTypeHash(*static_cast<const FSoftObjectPath*>(Src));
}
bool SoftObjectPathSerialize(FArchive& Ar, void* Data) {
    Ar << *static_cast<FSoftObjectPath*>(Data);
    return true;
}
FCppStructOps MakeSoftObjectPathOps() {
    FCppStructOps Ops = ObjectStructOps<FSoftObjectPath>(&SoftObjectPathSerialize, &SoftObjectPathIdentical);
    Ops.GetTypeHash = &SoftObjectPathHash;
    return Ops;
}
const FCppStructOps SoftObjectPathOps = MakeSoftObjectPathOps();

bool TopLevelAssetPathIdentical(const void* A, const void* B) {
    return *static_cast<const FTopLevelAssetPath*>(A) == *static_cast<const FTopLevelAssetPath*>(B);
}
bool TopLevelAssetPathSerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar, void* Data) {
    FString Path;
    if (Tag.Type == FName("NameProperty")) {
        FName Name;
        Ar << Name;
        Path = NameToString(Name);
    } else if (Tag.Type == FName("StrProperty")) {
        Ar << Path;
    } else {
        return false;
    }
    *static_cast<FTopLevelAssetPath*>(Data) = FSoftObjectPath(Path).AssetPath;
    return true;
}
FCppStructOps MakeTopLevelAssetPathOps() {
    FCppStructOps Ops = ObjectStructOps<FTopLevelAssetPath>(nullptr, &TopLevelAssetPathIdentical);
    Ops.SerializeFromMismatchedTag = &TopLevelAssetPathSerializeFromMismatchedTag;
    return Ops;
}
const FCppStructOps TopLevelAssetPathOps = MakeTopLevelAssetPathOps();

template <class T, class TValue>
bool PerPlatformSerialize(FArchive& Ar, void* Data) {
    T& Value = *static_cast<T*>(Data);
    bool bCooked = false;
    Ar << bCooked << Value.Default;
    if (!bCooked && Ar.GetGame() >= EGame::GAME_UE5_8) {
        int32 Count = Ar.ReadCount(8);
        for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
            FName Platform;
            TValue PlatformValue{};
            Ar << Platform << PlatformValue;
        }
    }
    return true;
}

template <class T>
bool PerPlatformIdentical(const void* A, const void* B) {
    return static_cast<const T*>(A)->Default == static_cast<const T*>(B)->Default;
}
constexpr FName NAME_IntProperty("IntProperty");
constexpr FName NAME_FloatProperty("FloatProperty");

template <class T, const FName* BasePropertyName>
bool PerPlatformSerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar, void* Data) {
    if (Tag.Type != *BasePropertyName)
        return false;
    Ar << static_cast<T*>(Data)->Default;
    return true;
}

template <class T, class TValue, const FName* BasePropertyName>
FCppStructOps MakeConvertiblePerPlatformOps() {
    FCppStructOps Ops = ObjectStructOps<T>(&PerPlatformSerialize<T, TValue>, &PerPlatformIdentical<T>);
    Ops.SerializeFromMismatchedTag = &PerPlatformSerializeFromMismatchedTag<T, BasePropertyName>;
    return Ops;
}
const FCppStructOps PerPlatformIntOps = MakeConvertiblePerPlatformOps<FPerPlatformInt, int32, &NAME_IntProperty>();
const FCppStructOps PerPlatformFloatOps = MakeConvertiblePerPlatformOps<FPerPlatformFloat, float, &NAME_FloatProperty>();
const FCppStructOps PerPlatformBoolOps =
    ObjectStructOps<FPerPlatformBool>(&PerPlatformSerialize<FPerPlatformBool, bool>, &PerPlatformIdentical<FPerPlatformBool>);
const FCppStructOps PerPlatformFrameRateOps =
    ObjectStructOps<FPerPlatformFrameRate>(&PerPlatformSerialize<FPerPlatformFrameRate, FFrameRate>, &PerPlatformIdentical<FPerPlatformFrameRate>);
bool PerPlatformFStringIdentical(const void* A, const void* B) {
    return static_cast<const FPerPlatformFString*>(A)->Default.Equals(static_cast<const FPerPlatformFString*>(B)->Default);
}
const FCppStructOps PerPlatformFStringOps =
    ObjectStructOps<FPerPlatformFString>(&PerPlatformSerialize<FPerPlatformFString, FString>, &PerPlatformFStringIdentical);

template <class T, class TValue>
bool PerQualitySerialize(FArchive& Ar, void* Data) {
    T& Value = *static_cast<T*>(Data);
    bool bCooked = false;
    Ar << bCooked << Value.Default;
    int32 Count = Ar.ReadCount(8);
    Value.PerQuality.Empty(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        int32 Level = 0;
        TValue LevelValue{};
        Ar << Level << LevelValue;
        Value.PerQuality.Add({Level, LevelValue});
    }
    return true;
}
const FCppStructOps PerQualityLevelIntOps =
    ObjectStructOps<FPerQualityLevelInt>(&PerQualitySerialize<FPerQualityLevelInt, int32>, &PerPlatformIdentical<FPerQualityLevelInt>);
const FCppStructOps PerQualityLevelFloatOps =
    ObjectStructOps<FPerQualityLevelFloat>(&PerQualitySerialize<FPerQualityLevelFloat, float>, &PerPlatformIdentical<FPerQualityLevelFloat>);

const FCppStructOps InstancedStructOps = TCppStructOps<FInstancedStruct>();
const FCppStructOps InstancedStructContainerOps = TCppStructOps<FInstancedStructContainer>();
const FCppStructOps InstancedPropertyBagOps = TCppStructOps<FInstancedPropertyBag>();

const FCppStructOps Empty0Ops = OpaqueStructOps<0>();

bool ClothTetherDataSerialize(FArchive& Ar, void*) {
    for (uint16 Fragment = 0; (Fragment & 0x0100) == 0 && !Ar.IsError();) {
        Ar << Fragment;
        if ((Fragment >> 9) != 0)
            Ar.SetError(ErrorCode::InvalidData, "ClothTetherData.Properties");
    }
    const int32 BatchCount = Ar.ReadCount(4);
    for (int32 i = 0; i < BatchCount && !Ar.IsError(); i++)
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(12)) * 12);
    return true;
}
FCppStructOps MakeClothTetherDataOps() {
    FCppStructOps Ops = OpaqueStructOps<0>();
    Ops.Serialize = &ClothTetherDataSerialize;
    return Ops;
}
const FCppStructOps ClothTetherDataOps = MakeClothTetherDataOps();

bool ClothLODDataCommonSerialize(FArchive& Ar, void* Data) {
    static constinit std::atomic<UScriptStruct*> Cache = nullptr;
    UScriptStruct* Struct = Cache.load(std::memory_order_acquire);
    if (Struct == nullptr)
        Struct = FindNativeStruct(Cache, "ClothLODDataCommon");
    Struct->SerializeTaggedProperties(Ar, static_cast<uint8*>(Data));
    for (int32 Transition = 0; Transition < 2 && !Ar.IsError(); Transition++)
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(64)) * 64);
    return true;
}
FCppStructOps MakeClothLODDataCommonOps() {
    FCppStructOps Ops = OpaqueStructOps<0>();
    Ops.Serialize = &ClothLODDataCommonSerialize;
    return Ops;
}
const FCppStructOps ClothLODDataCommonOps = MakeClothLODDataCommonOps();
bool MovieSceneFrameRangeSerialize(FArchive& Ar, void* Data) {
    auto& Range = *static_cast<FMovieSceneFrameRange*>(Data);
    Ar << Range.LowerBound << Range.UpperBound;
    return true;
}
bool MovieSceneFrameRangeIdentical(const void* A, const void* B) {
    return *static_cast<const FMovieSceneFrameRange*>(A) == *static_cast<const FMovieSceneFrameRange*>(B);
}
bool MovieSceneFrameRangeSerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar, void* Data) {
    if (Tag.Type != FName("StructProperty") || Tag.TypeName.GetParameterName(0) != FName("FloatRange"))
        return false;
    static constinit std::atomic<UScriptStruct*> Cache = nullptr;
    UScriptStruct* FloatRangeStruct = Cache.load(std::memory_order_acquire);
    if (FloatRangeStruct == nullptr)
        FloatRangeStruct = FindNativeStruct(Cache, "FloatRange");
    FFloatRange FloatRange;
    FloatRangeStruct->SerializeItem(Ar, &FloatRange);
    auto& Range = *static_cast<FMovieSceneFrameRange*>(Data);
    Range.LowerBound = UpgradeLegacyRangeBound(FloatRange.LowerBound);
    Range.UpperBound = UpgradeLegacyRangeBound(FloatRange.UpperBound);
    return true;
}
FCppStructOps MakeMovieSceneFrameRangeOps() {
    FCppStructOps Ops = ObjectStructOps<FMovieSceneFrameRange>(&MovieSceneFrameRangeSerialize, &MovieSceneFrameRangeIdentical);
    Ops.SerializeFromMismatchedTag = &MovieSceneFrameRangeSerializeFromMismatchedTag;
    return Ops;
}
const FCppStructOps MovieSceneFrameRangeOps = MakeMovieSceneFrameRangeOps();

const FCppStructOps Opaque4Ops = OpaqueStructOps<4>();
const FCppStructOps Opaque8Ops = OpaqueStructOps<8>();
const FCppStructOps Opaque32Ops = OpaqueStructOps<32>();
const FCppStructOps Opaque80Ops = OpaqueStructOps<80>();

#define MEMBER(TType, TName, TScriptType, TMember)                                                                                                             \
    FNativeMember {                                                                                                                                            \
        TName, TScriptType, static_cast<int32>(offsetof(TType, TMember)), static_cast<int32>(sizeof(TType::TMember))                                           \
    }

const FNativeMember VectorMembers[] = {MEMBER(FVector, "X", "double", X), MEMBER(FVector, "Y", "double", Y), MEMBER(FVector, "Z", "double", Z)};
const FNativeMember Vector2DMembers[] = {MEMBER(FVector2D, "X", "double", X), MEMBER(FVector2D, "Y", "double", Y)};
const FNativeMember Vector4Members[] = {MEMBER(FVector4, "X", "double", X), MEMBER(FVector4, "Y", "double", Y), MEMBER(FVector4, "Z", "double", Z),
                                        MEMBER(FVector4, "W", "double", W)};
const FNativeMember Vector3fMembers[] = {MEMBER(FVector3f, "X", "float", X), MEMBER(FVector3f, "Y", "float", Y), MEMBER(FVector3f, "Z", "float", Z)};
const FNativeMember Vector2fMembers[] = {MEMBER(FVector2f, "X", "float", X), MEMBER(FVector2f, "Y", "float", Y)};
const FNativeMember Vector4fMembers[] = {MEMBER(FVector4f, "X", "float", X), MEMBER(FVector4f, "Y", "float", Y), MEMBER(FVector4f, "Z", "float", Z),
                                         MEMBER(FVector4f, "W", "float", W)};
const FNativeMember RotatorMembers[] = {MEMBER(FRotator, "Pitch", "double", Pitch), MEMBER(FRotator, "Yaw", "double", Yaw),
                                        MEMBER(FRotator, "Roll", "double", Roll)};
const FNativeMember Rotator3fMembers[] = {MEMBER(FRotator3f, "Pitch", "float", Pitch), MEMBER(FRotator3f, "Yaw", "float", Yaw),
                                          MEMBER(FRotator3f, "Roll", "float", Roll)};
const FNativeMember QuatMembers[] = {MEMBER(FQuat, "X", "double", X), MEMBER(FQuat, "Y", "double", Y), MEMBER(FQuat, "Z", "double", Z),
                                     MEMBER(FQuat, "W", "double", W)};
const FNativeMember Quat4fMembers[] = {MEMBER(FQuat4f, "X", "float", X), MEMBER(FQuat4f, "Y", "float", Y), MEMBER(FQuat4f, "Z", "float", Z),
                                       MEMBER(FQuat4f, "W", "float", W)};
const FNativeMember PlaneMembers[] = {MEMBER(FPlane, "W", "double", W)};
const FNativeMember MatrixMembers[] = {MEMBER(FMatrix, "XPlane", "Struct:Plane", M[0]), MEMBER(FMatrix, "YPlane", "Struct:Plane", M[1]),
                                       MEMBER(FMatrix, "ZPlane", "Struct:Plane", M[2]), MEMBER(FMatrix, "WPlane", "Struct:Plane", M[3])};
const FNativeMember TransformMembers[] = {MEMBER(FTransform, "Rotation", "Struct:Quat", Rotation),
                                          MEMBER(FTransform, "Translation", "Struct:Vector", Translation),
                                          MEMBER(FTransform, "Scale3D", "Struct:Vector", Scale3D)};
const FNativeMember Transform3fMembers[] = {MEMBER(FTransform3f, "Rotation", "Struct:Quat4f", Rotation),
                                            MEMBER(FTransform3f, "Translation", "Struct:Vector3f", Translation),
                                            MEMBER(FTransform3f, "Scale3D", "Struct:Vector3f", Scale3D)};
const FNativeMember BoxSphereBoundsMembers[] = {MEMBER(FBoxSphereBounds, "Origin", "Struct:Vector", Origin),
                                                MEMBER(FBoxSphereBounds, "BoxExtent", "Struct:Vector", BoxExtent),
                                                MEMBER(FBoxSphereBounds, "SphereRadius", "double", SphereRadius)};
const FNativeMember LinearColorMembers[] = {MEMBER(FLinearColor, "R", "float", R), MEMBER(FLinearColor, "G", "float", G), MEMBER(FLinearColor, "B", "float", B),
                                            MEMBER(FLinearColor, "A", "float", A)};
const FNativeMember ColorMembers[] = {MEMBER(FColor, "B", "uint8", B), MEMBER(FColor, "G", "uint8", G), MEMBER(FColor, "R", "uint8", R),
                                      MEMBER(FColor, "A", "uint8", A)};
const FNativeMember GuidMembers[] = {MEMBER(FGuid, "A", "uint32", a), MEMBER(FGuid, "B", "uint32", b), MEMBER(FGuid, "C", "uint32", c),
                                     MEMBER(FGuid, "D", "uint32", d)};
const FNativeMember IntPointMembers[] = {MEMBER(FIntPoint, "X", "int32", X), MEMBER(FIntPoint, "Y", "int32", Y)};
const FNativeMember IntVectorMembers[] = {MEMBER(FIntVector, "X", "int32", X), MEMBER(FIntVector, "Y", "int32", Y), MEMBER(FIntVector, "Z", "int32", Z)};
const FNativeMember BoxMembers[] = {MEMBER(FBox, "Min", "Struct:Vector", Min), MEMBER(FBox, "Max", "Struct:Vector", Max),
                                    MEMBER(FBox, "IsValid", "uint8", IsValid)};
const FNativeMember Box2DMembers[] = {MEMBER(FBox2D, "Min", "Struct:Vector2D", Min), MEMBER(FBox2D, "Max", "Struct:Vector2D", Max),
                                      MEMBER(FBox2D, "bIsValid", "bool", bIsValid)};
const FNativeMember DateTimeMembers[] = {MEMBER(FDateTime, "Ticks", "int64", Ticks)};
const FNativeMember TimespanMembers[] = {MEMBER(FTimespan, "Ticks", "int64", Ticks)};
const FNativeMember RandomStreamMembers[] = {MEMBER(FRandomStream, "InitialSeed", "int32", InitialSeed), MEMBER(FRandomStream, "Seed", "int32", Seed)};
const FNativeMember FrameNumberMembers[] = {MEMBER(FFrameNumber, "Value", "int32", Value)};
const FNativeMember InputDeviceIdMembers[] = {MEMBER(FInputDeviceId, "InternalId", "int32", InternalId)};
const FNativeMember FrameRateMembers[] = {MEMBER(FFrameRate, "Numerator", "int32", Numerator), MEMBER(FFrameRate, "Denominator", "int32", Denominator)};
const FNativeMember PerPlatformFrameRateMembers[] = {MEMBER(FPerPlatformFrameRate, "Default", "Struct:FrameRate", Default)};
const FNativeMember TopLevelAssetPathMembers[] = {MEMBER(FTopLevelAssetPath, "PackageName", "Name", PackageName),
                                                  MEMBER(FTopLevelAssetPath, "AssetName", "Name", AssetName)};
const FNativeMember SoftObjectPathMembers[] = {MEMBER(FSoftObjectPath, "AssetPath", "Struct:TopLevelAssetPath", AssetPath),
                                               MEMBER(FSoftObjectPath, "SubPathString", "Str", SubPathString)};
const FNativeMember FloatIntervalMembers[] = {MEMBER(FFloatInterval, "Min", "float", Min), MEMBER(FFloatInterval, "Max", "float", Max)};
const FNativeMember Int32IntervalMembers[] = {MEMBER(FInt32Interval, "Min", "int32", Min), MEMBER(FInt32Interval, "Max", "int32", Max)};
const FNativeMember DirectoryPathMembers[] = {MEMBER(FDirectoryPath, "Path", "Str", Path)};
const FNativeMember PerPlatformIntMembers[] = {MEMBER(FPerPlatformInt, "Default", "int32", Default)};
const FNativeMember PerPlatformFloatMembers[] = {MEMBER(FPerPlatformFloat, "Default", "float", Default)};
const FNativeMember PerPlatformBoolMembers[] = {MEMBER(FPerPlatformBool, "Default", "bool", Default)};
const FNativeMember PerQualityLevelIntMembers[] = {MEMBER(FPerQualityLevelInt, "Default", "int32", Default)};
const FNativeMember PerQualityLevelFloatMembers[] = {MEMBER(FPerQualityLevelFloat, "Default", "float", Default)};

const FNativeMember InterpCurvePointFloatMembers[] = {
    MEMBER(FInterpCurvePointFloat, "InVal", "float", InVal), MEMBER(FInterpCurvePointFloat, "OutVal", "float", OutVal),
    MEMBER(FInterpCurvePointFloat, "ArriveTangent", "float", ArriveTangent), MEMBER(FInterpCurvePointFloat, "LeaveTangent", "float", LeaveTangent),
    MEMBER(FInterpCurvePointFloat, "InterpMode", "Byte:EInterpCurveMode", InterpMode)};
const FNativeMember InterpCurveFloatMembers[] = {MEMBER(FInterpCurveFloat, "Points", "Array<Struct:InterpCurvePointFloat>", Points),
                                                 MEMBER(FInterpCurveFloat, "bIsLooped", "bool", bIsLooped),
                                                 MEMBER(FInterpCurveFloat, "LoopKeyOffset", "float", LoopKeyOffset)};
const FNativeMember InterpCurvePointVectorMembers[] = {MEMBER(FInterpCurvePointVector, "InVal", "float", InVal),
                                                       MEMBER(FInterpCurvePointVector, "OutVal", "Struct:Vector", OutVal),
                                                       MEMBER(FInterpCurvePointVector, "ArriveTangent", "Struct:Vector", ArriveTangent),
                                                       MEMBER(FInterpCurvePointVector, "LeaveTangent", "Struct:Vector", LeaveTangent),
                                                       MEMBER(FInterpCurvePointVector, "InterpMode", "Byte:EInterpCurveMode", InterpMode)};
const FNativeMember InterpCurveVectorMembers[] = {MEMBER(FInterpCurveVector, "Points", "Array<Struct:InterpCurvePointVector>", Points),
                                                  MEMBER(FInterpCurveVector, "bIsLooped", "bool", bIsLooped),
                                                  MEMBER(FInterpCurveVector, "LoopKeyOffset", "float", LoopKeyOffset)};
const FNativeMember InterpCurvePointQuatMembers[] = {MEMBER(FInterpCurvePointQuat, "InVal", "float", InVal),
                                                     MEMBER(FInterpCurvePointQuat, "OutVal", "Struct:Quat", OutVal),
                                                     MEMBER(FInterpCurvePointQuat, "ArriveTangent", "Struct:Quat", ArriveTangent),
                                                     MEMBER(FInterpCurvePointQuat, "LeaveTangent", "Struct:Quat", LeaveTangent),
                                                     MEMBER(FInterpCurvePointQuat, "InterpMode", "Byte:EInterpCurveMode", InterpMode)};
const FNativeMember InterpCurveQuatMembers[] = {MEMBER(FInterpCurveQuat, "Points", "Array<Struct:InterpCurvePointQuat>", Points),
                                                MEMBER(FInterpCurveQuat, "bIsLooped", "bool", bIsLooped),
                                                MEMBER(FInterpCurveQuat, "LoopKeyOffset", "float", LoopKeyOffset)};
const FNativeMember Int32RangeBoundMembers[] = {MEMBER(FInt32RangeBound, "Type", "Byte:ERangeBoundTypes", Type),
                                                MEMBER(FInt32RangeBound, "Value", "int32", Value)};
const FNativeMember Int32RangeMembers[] = {MEMBER(FInt32Range, "LowerBound", "Struct:Int32RangeBound", LowerBound),
                                           MEMBER(FInt32Range, "UpperBound", "Struct:Int32RangeBound", UpperBound)};
const FCppStructOps FloatIntervalOps = TCppStructOps<FFloatInterval>();
const FCppStructOps Int32IntervalOps = TCppStructOps<FInt32Interval>();
const FCppStructOps DirectoryPathOps = TCppStructOps<FDirectoryPath>();
const FCppStructOps Int32RangeBoundOps = TCppStructOps<FInt32RangeBound>();
const FCppStructOps Int32RangeOps = TCppStructOps<FInt32Range>();
const FNativeMember FloatRangeBoundMembers[] = {MEMBER(FFloatRangeBound, "Type", "Byte:ERangeBoundTypes", Type),
                                                MEMBER(FFloatRangeBound, "Value", "float", Value)};
const FNativeMember FloatRangeMembers[] = {MEMBER(FFloatRange, "LowerBound", "Struct:FloatRangeBound", LowerBound),
                                           MEMBER(FFloatRange, "UpperBound", "Struct:FloatRangeBound", UpperBound)};
const FNativeMember FrameNumberRangeBoundMembers[] = {MEMBER(FFrameNumberRangeBound, "Type", "Byte:ERangeBoundTypes", Type),
                                                      MEMBER(FFrameNumberRangeBound, "Value", "Struct:FrameNumber", Value)};
const FNativeMember FrameNumberRangeMembers[] = {MEMBER(FFrameNumberRange, "LowerBound", "Struct:FrameNumberRangeBound", LowerBound),
                                                 MEMBER(FFrameNumberRange, "UpperBound", "Struct:FrameNumberRangeBound", UpperBound)};
const FNativeMember FrameTimeMembers[] = {MEMBER(FFrameTime, "FrameNumber", "Struct:FrameNumber", FrameNumber), MEMBER(FFrameTime, "SubFrame", "float", SubFrame)};
const FCppStructOps FrameNumberRangeBoundOps = TCppStructOps<FFrameNumberRangeBound>();
const FCppStructOps FrameNumberRangeOps = TCppStructOps<FFrameNumberRange>();
const FCppStructOps FrameTimeOps = TCppStructOps<FFrameTime>();
const FCppStructOps FloatRangeBoundOps = TCppStructOps<FFloatRangeBound>();
const FCppStructOps FloatRangeOps = TCppStructOps<FFloatRange>();
const FNativeMember TimecodeMembers[] = {MEMBER(FTimecode, "Hours", "int32", Hours),       MEMBER(FTimecode, "Minutes", "int32", Minutes),
                                         MEMBER(FTimecode, "Seconds", "int32", Seconds),   MEMBER(FTimecode, "Frames", "int32", Frames),
                                         MEMBER(FTimecode, "Subframe", "float", Subframe), MEMBER(FTimecode, "bDropFrameFormat", "bool", bDropFrameFormat)};
const FCppStructOps TimecodeOps = TCppStructOps<FTimecode>();
const FCppStructOps InterpCurvePointFloatOps = TCppStructOps<FInterpCurvePointFloat>();
const FCppStructOps InterpCurveFloatOps = TCppStructOps<FInterpCurveFloat>();
const FCppStructOps InterpCurvePointVectorOps = TCppStructOps<FInterpCurvePointVector>();
const FCppStructOps InterpCurveVectorOps = TCppStructOps<FInterpCurveVector>();
const FCppStructOps InterpCurvePointQuatOps = TCppStructOps<FInterpCurvePointQuat>();
const FCppStructOps InterpCurveQuatOps = TCppStructOps<FInterpCurveQuat>();

constexpr const char* CoreUObjectPackage = "/Script/CoreUObject";

const FNativeEnumName InterpCurveModeNames[] = {{"CIM_Linear", CIM_Linear},         {"CIM_CurveAuto", CIM_CurveAuto},
                                                {"CIM_Constant", CIM_Constant},     {"CIM_CurveUser", CIM_CurveUser},
                                                {"CIM_CurveBreak", CIM_CurveBreak}, {"CIM_CurveAutoClamped", CIM_CurveAutoClamped},
                                                {"CIM_Unknown", CIM_Unknown}};
const FNativeEnumName RangeBoundTypeNames[] = {{"ERangeBoundTypes::Exclusive", ERangeBoundTypes::Exclusive},
                                               {"ERangeBoundTypes::Inclusive", ERangeBoundTypes::Inclusive},
                                               {"ERangeBoundTypes::Open", ERangeBoundTypes::Open}};
const FNativeEnumName AxisNames[] = {{"EAxis::None", EAxis::None}, {"EAxis::X", EAxis::X}, {"EAxis::Y", EAxis::Y}, {"EAxis::Z", EAxis::Z}};

const FNativeEnumName PixelFormatNames[] = {{"PF_Unknown", int64(EPixelFormat::PF_Unknown)},
                                            {"PF_A32B32G32R32F", int64(EPixelFormat::PF_A32B32G32R32F)},
                                            {"PF_B8G8R8A8", int64(EPixelFormat::PF_B8G8R8A8)},
                                            {"PF_G8", int64(EPixelFormat::PF_G8)},
                                            {"PF_G16", int64(EPixelFormat::PF_G16)},
                                            {"PF_DXT1", int64(EPixelFormat::PF_DXT1)},
                                            {"PF_DXT3", int64(EPixelFormat::PF_DXT3)},
                                            {"PF_DXT5", int64(EPixelFormat::PF_DXT5)},
                                            {"PF_UYVY", int64(EPixelFormat::PF_UYVY)},
                                            {"PF_FloatRGB", int64(EPixelFormat::PF_FloatRGB)},
                                            {"PF_FloatRGBA", int64(EPixelFormat::PF_FloatRGBA)},
                                            {"PF_DepthStencil", int64(EPixelFormat::PF_DepthStencil)},
                                            {"PF_ShadowDepth", int64(EPixelFormat::PF_ShadowDepth)},
                                            {"PF_R32_FLOAT", int64(EPixelFormat::PF_R32_FLOAT)},
                                            {"PF_G16R16", int64(EPixelFormat::PF_G16R16)},
                                            {"PF_G16R16F", int64(EPixelFormat::PF_G16R16F)},
                                            {"PF_G16R16F_FILTER", int64(EPixelFormat::PF_G16R16F_FILTER)},
                                            {"PF_G32R32F", int64(EPixelFormat::PF_G32R32F)},
                                            {"PF_A2B10G10R10", int64(EPixelFormat::PF_A2B10G10R10)},
                                            {"PF_A16B16G16R16", int64(EPixelFormat::PF_A16B16G16R16)},
                                            {"PF_D24", int64(EPixelFormat::PF_D24)},
                                            {"PF_R16F", int64(EPixelFormat::PF_R16F)},
                                            {"PF_R16F_FILTER", int64(EPixelFormat::PF_R16F_FILTER)},
                                            {"PF_BC5", int64(EPixelFormat::PF_BC5)},
                                            {"PF_V8U8", int64(EPixelFormat::PF_V8U8)},
                                            {"PF_A1", int64(EPixelFormat::PF_A1)},
                                            {"PF_FloatR11G11B10", int64(EPixelFormat::PF_FloatR11G11B10)},
                                            {"PF_A8", int64(EPixelFormat::PF_A8)},
                                            {"PF_R32_UINT", int64(EPixelFormat::PF_R32_UINT)},
                                            {"PF_R32_SINT", int64(EPixelFormat::PF_R32_SINT)},
                                            {"PF_PVRTC2", int64(EPixelFormat::PF_PVRTC2)},
                                            {"PF_PVRTC4", int64(EPixelFormat::PF_PVRTC4)},
                                            {"PF_R16_UINT", int64(EPixelFormat::PF_R16_UINT)},
                                            {"PF_R16_SINT", int64(EPixelFormat::PF_R16_SINT)},
                                            {"PF_R16G16B16A16_UINT", int64(EPixelFormat::PF_R16G16B16A16_UINT)},
                                            {"PF_R16G16B16A16_SINT", int64(EPixelFormat::PF_R16G16B16A16_SINT)},
                                            {"PF_R5G6B5_UNORM", int64(EPixelFormat::PF_R5G6B5_UNORM)},
                                            {"PF_R8G8B8A8", int64(EPixelFormat::PF_R8G8B8A8)},
                                            {"PF_A8R8G8B8", int64(EPixelFormat::PF_A8R8G8B8)},
                                            {"PF_BC4", int64(EPixelFormat::PF_BC4)},
                                            {"PF_R8G8", int64(EPixelFormat::PF_R8G8)},
                                            {"PF_ATC_RGB", int64(EPixelFormat::PF_ATC_RGB)},
                                            {"PF_ATC_RGBA_E", int64(EPixelFormat::PF_ATC_RGBA_E)},
                                            {"PF_ATC_RGBA_I", int64(EPixelFormat::PF_ATC_RGBA_I)},
                                            {"PF_X24_G8", int64(EPixelFormat::PF_X24_G8)},
                                            {"PF_ETC1", int64(EPixelFormat::PF_ETC1)},
                                            {"PF_ETC2_RGB", int64(EPixelFormat::PF_ETC2_RGB)},
                                            {"PF_ETC2_RGBA", int64(EPixelFormat::PF_ETC2_RGBA)},
                                            {"PF_R32G32B32A32_UINT", int64(EPixelFormat::PF_R32G32B32A32_UINT)},
                                            {"PF_R16G16_UINT", int64(EPixelFormat::PF_R16G16_UINT)},
                                            {"PF_ASTC_4x4", int64(EPixelFormat::PF_ASTC_4x4)},
                                            {"PF_ASTC_6x6", int64(EPixelFormat::PF_ASTC_6x6)},
                                            {"PF_ASTC_8x8", int64(EPixelFormat::PF_ASTC_8x8)},
                                            {"PF_ASTC_10x10", int64(EPixelFormat::PF_ASTC_10x10)},
                                            {"PF_ASTC_12x12", int64(EPixelFormat::PF_ASTC_12x12)},
                                            {"PF_BC6H", int64(EPixelFormat::PF_BC6H)},
                                            {"PF_BC7", int64(EPixelFormat::PF_BC7)},
                                            {"PF_R8_UINT", int64(EPixelFormat::PF_R8_UINT)},
                                            {"PF_L8", int64(EPixelFormat::PF_L8)},
                                            {"PF_XGXR8", int64(EPixelFormat::PF_XGXR8)},
                                            {"PF_R8G8B8A8_UINT", int64(EPixelFormat::PF_R8G8B8A8_UINT)},
                                            {"PF_R8G8B8A8_SNORM", int64(EPixelFormat::PF_R8G8B8A8_SNORM)},
                                            {"PF_R16G16B16A16_UNORM", int64(EPixelFormat::PF_R16G16B16A16_UNORM)},
                                            {"PF_R16G16B16A16_SNORM", int64(EPixelFormat::PF_R16G16B16A16_SNORM)},
                                            {"PF_PLATFORM_HDR_0", int64(EPixelFormat::PF_PLATFORM_HDR_0)},
                                            {"PF_PLATFORM_HDR_1", int64(EPixelFormat::PF_PLATFORM_HDR_1)},
                                            {"PF_PLATFORM_HDR_2", int64(EPixelFormat::PF_PLATFORM_HDR_2)},
                                            {"PF_NV12", int64(EPixelFormat::PF_NV12)},
                                            {"PF_R32G32_UINT", int64(EPixelFormat::PF_R32G32_UINT)},
                                            {"PF_ETC2_R11_EAC", int64(EPixelFormat::PF_ETC2_R11_EAC)},
                                            {"PF_ETC2_RG11_EAC", int64(EPixelFormat::PF_ETC2_RG11_EAC)},
                                            {"PF_R8", int64(EPixelFormat::PF_R8)},
                                            {"PF_B5G5R5A1_UNORM", int64(EPixelFormat::PF_B5G5R5A1_UNORM)},
                                            {"PF_ASTC_4x4_HDR", int64(EPixelFormat::PF_ASTC_4x4_HDR)},
                                            {"PF_ASTC_6x6_HDR", int64(EPixelFormat::PF_ASTC_6x6_HDR)},
                                            {"PF_ASTC_8x8_HDR", int64(EPixelFormat::PF_ASTC_8x8_HDR)},
                                            {"PF_ASTC_10x10_HDR", int64(EPixelFormat::PF_ASTC_10x10_HDR)},
                                            {"PF_ASTC_12x12_HDR", int64(EPixelFormat::PF_ASTC_12x12_HDR)},
                                            {"PF_G16R16_SNORM", int64(EPixelFormat::PF_G16R16_SNORM)},
                                            {"PF_R8G8_UINT", int64(EPixelFormat::PF_R8G8_UINT)},
                                            {"PF_R32G32B32_UINT", int64(EPixelFormat::PF_R32G32B32_UINT)},
                                            {"PF_R32G32B32_SINT", int64(EPixelFormat::PF_R32G32B32_SINT)},
                                            {"PF_R32G32B32F", int64(EPixelFormat::PF_R32G32B32F)},
                                            {"PF_R8_SINT", int64(EPixelFormat::PF_R8_SINT)},
                                            {"PF_R64_UINT", int64(EPixelFormat::PF_R64_UINT)},
                                            {"PF_R9G9B9EXP5", int64(EPixelFormat::PF_R9G9B9EXP5)},
                                            {"PF_P010", int64(EPixelFormat::PF_P010)},
                                            {"PF_ASTC_4x4_NORM_RG", int64(EPixelFormat::PF_ASTC_4x4_NORM_RG)},
                                            {"PF_ASTC_6x6_NORM_RG", int64(EPixelFormat::PF_ASTC_6x6_NORM_RG)},
                                            {"PF_ASTC_8x8_NORM_RG", int64(EPixelFormat::PF_ASTC_8x8_NORM_RG)},
                                            {"PF_ASTC_10x10_NORM_RG", int64(EPixelFormat::PF_ASTC_10x10_NORM_RG)},
                                            {"PF_ASTC_12x12_NORM_RG", int64(EPixelFormat::PF_ASTC_12x12_NORM_RG)},
                                            {"PF_R16G16_SINT", int64(EPixelFormat::PF_R16G16_SINT)},
                                            {"PF_R8G8B8", int64(EPixelFormat::PF_R8G8B8)},
                                            {"PF_MAX", int64(EPixelFormat::PF_MAX)},
                                            {"PF_B4G4R4A4", int64(EPixelFormat::PF_B4G4R4A4)},
                                            {"PF_BC6H_Signed", int64(EPixelFormat::PF_BC6H_Signed)},
                                            {"PF_ETC2_R11", int64(EPixelFormat::PF_ETC2_R11)},
                                            {"PF_ASTC_8x5", int64(EPixelFormat::PF_ASTC_8x5)},
                                            {"PF_ASTC_8x6", int64(EPixelFormat::PF_ASTC_8x6)},
                                            {"PF_ASTC_10x8", int64(EPixelFormat::PF_ASTC_10x8)}};

const FNativeEnumInfo CoreEnums[] = {
    {CoreUObjectPackage, "EInterpCurveMode", InterpCurveModeNames},
    {CoreUObjectPackage, "EAxis", AxisNames},
    {CoreUObjectPackage, "ERangeBoundTypes", RangeBoundTypeNames},
    {CoreUObjectPackage, "EPixelFormat", PixelFormatNames},
};

const FNativeStructInfo CoreStructs[] = {
    {CoreUObjectPackage, "Vector", &VectorOps, VectorMembers},
    {CoreUObjectPackage, "Vector3d", &VectorOps, VectorMembers},
    {CoreUObjectPackage, "Vector2D", &Vector2DOps, Vector2DMembers},
    {CoreUObjectPackage, "Vector4", &Vector4Ops, Vector4Members},
    {CoreUObjectPackage, "Vector4d", &Vector4Ops, Vector4Members},
    {CoreUObjectPackage, "Vector3f", &Vector3fOps, Vector3fMembers},
    {CoreUObjectPackage, "Vector2f", &Vector2fOps, Vector2fMembers},
    {CoreUObjectPackage, "Vector4f", &Vector4fOps, Vector4fMembers},
    {CoreUObjectPackage, "Rotator", &RotatorOps, RotatorMembers},
    {CoreUObjectPackage, "Rotator3d", &RotatorOps, RotatorMembers},
    {CoreUObjectPackage, "Rotator3f", &Rotator3fOps, Rotator3fMembers},
    {CoreUObjectPackage, "Quat", &QuatOps, QuatMembers},
    {CoreUObjectPackage, "Quat4d", &QuatOps, QuatMembers},
    {CoreUObjectPackage, "Quat4f", &Quat4fOps, Quat4fMembers},
    {CoreUObjectPackage, "Plane", &PlaneOps, PlaneMembers},
    {CoreUObjectPackage, "Plane4d", &PlaneOps, PlaneMembers},
    {CoreUObjectPackage, "Plane4f", &Plane4fOps},
    {CoreUObjectPackage, "Sphere", &SphereOps},
    {CoreUObjectPackage, "Sphere3d", &SphereOps},
    {CoreUObjectPackage, "Sphere3f", &Sphere3fOps},
    {CoreUObjectPackage, "Matrix", &MatrixOps, MatrixMembers},
    {CoreUObjectPackage, "Matrix44d", &MatrixOps},
    {CoreUObjectPackage, "Matrix44f", &Matrix44fOps},
    {CoreUObjectPackage, "Box", &BoxOps, BoxMembers},
    {CoreUObjectPackage, "Box3d", &BoxOps, BoxMembers},
    {CoreUObjectPackage, "Box3f", &Box3fOps},
    {CoreUObjectPackage, "Box2D", &Box2DOps, Box2DMembers},
    {CoreUObjectPackage, "Box2f", &Box2fOps},
    {CoreUObjectPackage, "TwoVectors", &TwoVectorsOps},
    {CoreUObjectPackage, "Transform", &TransformOps, TransformMembers},
    {CoreUObjectPackage, "Transform3d", &TransformOps, TransformMembers},
    {CoreUObjectPackage, "BoxSphereBounds", &BoxSphereBoundsOps, BoxSphereBoundsMembers},
    {CoreUObjectPackage, "BoxSphereBounds3d", &BoxSphereBoundsOps, BoxSphereBoundsMembers},
    {CoreUObjectPackage, "Transform3f", &Transform3fOps, Transform3fMembers},
    {CoreUObjectPackage, "LinearColor", &LinearColorOps, LinearColorMembers},
    {CoreUObjectPackage, "Color", &ColorOps, ColorMembers},
    {CoreUObjectPackage, "Guid", &GuidOps, GuidMembers},
    {CoreUObjectPackage, "IntPoint", &IntPointOps, IntPointMembers},
    {CoreUObjectPackage, "Int32Point", &IntPointOps, IntPointMembers},
    {CoreUObjectPackage, "IntVector2", &IntVector2Ops},
    {CoreUObjectPackage, "Int32Vector2", &IntVector2Ops},
    {CoreUObjectPackage, "UintVector2", &UintVector2Ops},
    {CoreUObjectPackage, "Uint32Point", &UintPointOps},
    {CoreUObjectPackage, "IntVector", &IntVectorOps, IntVectorMembers},
    {CoreUObjectPackage, "UintVector", &UintVectorOps},
    {CoreUObjectPackage, "IntVector4", &IntVector4Ops},
    {CoreUObjectPackage, "UintVector4", &UintVector4Ops},
    {CoreUObjectPackage, "Uint32Vector4", &UintVector4Ops},
    {CoreUObjectPackage, "Int64Point", &Int64PointOps},
    {CoreUObjectPackage, "Int64Vector2", &Int64PointOps},
    {CoreUObjectPackage, "UInt64Point", &Int64PointOps},
    {CoreUObjectPackage, "UInt64Vector2", &Int64PointOps},
    {CoreUObjectPackage, "Int64Vector", &Int64VectorOps},
    {CoreUObjectPackage, "UInt64Vector", &Int64VectorOps},
    {CoreUObjectPackage, "Int64Vector4", &Opaque32Ops},
    {CoreUObjectPackage, "UInt64Vector4", &Opaque32Ops},
    {CoreUObjectPackage, "DateTime", &DateTimeOps, DateTimeMembers},
    {CoreUObjectPackage, "Timespan", &TimespanOps, TimespanMembers},
    {CoreUObjectPackage, "RandomStream", &RandomStreamOps, RandomStreamMembers},
    {CoreUObjectPackage, "FrameNumber", &FrameNumberOps, FrameNumberMembers},
    {CoreUObjectPackage, "FrameRate", &FrameRateOps, FrameRateMembers},
    {CoreUObjectPackage, "InputDeviceId", &InputDeviceIdOps, InputDeviceIdMembers},
    {CoreUObjectPackage, "TopLevelAssetPath", &TopLevelAssetPathOps, TopLevelAssetPathMembers},
    {CoreUObjectPackage, "SoftObjectPath", &SoftObjectPathOps, SoftObjectPathMembers},
    {CoreUObjectPackage, "SoftClassPath", &SoftObjectPathOps, SoftObjectPathMembers},
    {CoreUObjectPackage, "StringAssetReference", &SoftObjectPathOps},
    {CoreUObjectPackage, "FloatInterval", &FloatIntervalOps, FloatIntervalMembers},
    {CoreUObjectPackage, "Int32Interval", &Int32IntervalOps, Int32IntervalMembers},
    {CoreUObjectPackage, "DirectoryPath", &DirectoryPathOps, DirectoryPathMembers},
    {CoreUObjectPackage, "StringClassReference", &SoftObjectPathOps},
    {CoreUObjectPackage, "PerPlatformInt", &PerPlatformIntOps, PerPlatformIntMembers},
    {CoreUObjectPackage, "PerPlatformFloat", &PerPlatformFloatOps, PerPlatformFloatMembers},
    {CoreUObjectPackage, "PerPlatformBool", &PerPlatformBoolOps, PerPlatformBoolMembers},
    {CoreUObjectPackage, "PerPlatformFrameRate", &PerPlatformFrameRateOps, PerPlatformFrameRateMembers},
    {CoreUObjectPackage, "PerPlatformFString", &PerPlatformFStringOps},
    {CoreUObjectPackage, "PerQualityLevelInt", &PerQualityLevelIntOps, PerQualityLevelIntMembers},
    {CoreUObjectPackage, "PerQualityLevelFloat", &PerQualityLevelFloatOps, PerQualityLevelFloatMembers},
    {CoreUObjectPackage, "InstancedStruct", &InstancedStructOps},
    {CoreUObjectPackage, "InstancedStructContainer", &InstancedStructContainerOps},
    {CoreUObjectPackage, "InstancedPropertyBag", &InstancedPropertyBagOps},
    {CoreUObjectPackage, "Int32RangeBound", &Int32RangeBoundOps, Int32RangeBoundMembers},
    {CoreUObjectPackage, "Int32Range", &Int32RangeOps, Int32RangeMembers},
    {CoreUObjectPackage, "FrameNumberRangeBound", &FrameNumberRangeBoundOps, FrameNumberRangeBoundMembers},
    {CoreUObjectPackage, "FrameNumberRange", &FrameNumberRangeOps, FrameNumberRangeMembers},
    {CoreUObjectPackage, "FrameTime", &FrameTimeOps, FrameTimeMembers},
    {CoreUObjectPackage, "FloatRangeBound", &FloatRangeBoundOps, FloatRangeBoundMembers},
    {CoreUObjectPackage, "FloatRange", &FloatRangeOps, FloatRangeMembers},
    {CoreUObjectPackage, "Timecode", &TimecodeOps, TimecodeMembers},
    {CoreUObjectPackage, "InterpCurvePointFloat", &InterpCurvePointFloatOps, InterpCurvePointFloatMembers},
    {CoreUObjectPackage, "InterpCurveFloat", &InterpCurveFloatOps, InterpCurveFloatMembers},
    {CoreUObjectPackage, "InterpCurvePointVector", &InterpCurvePointVectorOps, InterpCurvePointVectorMembers},
    {CoreUObjectPackage, "InterpCurveVector", &InterpCurveVectorOps, InterpCurveVectorMembers},
    {CoreUObjectPackage, "InterpCurvePointQuat", &InterpCurvePointQuatOps, InterpCurvePointQuatMembers},
    {CoreUObjectPackage, "InterpCurveQuat", &InterpCurveQuatOps, InterpCurveQuatMembers},
    {"/Script/SlateCore", "DeprecateSlateVector2D", &Opaque8Ops},
    {"/Script/MovieScene", "MovieSceneFrameRange", &MovieSceneFrameRangeOps},
    {CoreUObjectPackage, "UInt32Property", &Opaque4Ops},
    {"/Script/ClothingSystemRuntimeCommon", "ClothTetherData", &ClothTetherDataOps},
    {"/Script/ClothingSystemRuntimeCommon", "ClothLODDataCommon", &ClothLODDataCommonOps},
    {"/Script/Engine", "KeyHandleMap", &Empty0Ops},
    {"/Script/Engine", "KeyHandleLookupTable", &Empty0Ops},
    {CoreUObjectPackage, "ZeroSizeProperty", &Empty0Ops},
};

const FNativeClassInfo CoreClasses[] = {
    NATIVE_CLASS_INFO(UObject, CoreUObjectPackage, "Object", nullptr),
    NATIVE_CLASS_INFO(UField, CoreUObjectPackage, "Field", "Object", 0, CASTCLASS_UField),
    NATIVE_CLASS_INFO(UStruct, CoreUObjectPackage, "Struct", "Field", 0, CASTCLASS_UStruct),
    NATIVE_CLASS_INFO(UScriptStruct, CoreUObjectPackage, "ScriptStruct", "Struct", 0, CASTCLASS_UScriptStruct),
    NATIVE_CLASS_INFO(UFunction, CoreUObjectPackage, "Function", "Struct", 0, CASTCLASS_UFunction),
    NATIVE_CLASS_INFO(UDelegateFunction, CoreUObjectPackage, "DelegateFunction", "Function", 0, CASTCLASS_UDelegateFunction),
    NATIVE_CLASS_INFO(USparseDelegateFunction, CoreUObjectPackage, "SparseDelegateFunction", "DelegateFunction", 0, CASTCLASS_USparseDelegateFunction),
    NATIVE_CLASS_INFO(UClass, CoreUObjectPackage, "Class", "Struct", 0, CASTCLASS_UClass),
    NATIVE_CLASS_INFO(UEnum, CoreUObjectPackage, "Enum", "Field", 0, CASTCLASS_UEnum),
    NATIVE_CLASS_INFO(UPackage, CoreUObjectPackage, "Package", "Object", 0, CASTCLASS_UPackage),
    NATIVE_CLASS_INFO(UInterface, CoreUObjectPackage, "Interface", "Object", CLASS_Interface | CLASS_Abstract),
    NATIVE_CLASS_INFO(UProperty, CoreUObjectPackage, "Property", "Field"),
    NATIVE_CLASS_INFO(UNumericProperty, CoreUObjectPackage, "NumericProperty", "Property"),
    NATIVE_CLASS_INFO(UByteProperty, CoreUObjectPackage, "ByteProperty", "NumericProperty"),
    NATIVE_CLASS_INFO(UInt8Property, CoreUObjectPackage, "Int8Property", "NumericProperty"),
    NATIVE_CLASS_INFO(UInt16Property, CoreUObjectPackage, "Int16Property", "NumericProperty"),
    NATIVE_CLASS_INFO(UIntProperty, CoreUObjectPackage, "IntProperty", "NumericProperty"),
    NATIVE_CLASS_INFO(UInt64Property, CoreUObjectPackage, "Int64Property", "NumericProperty"),
    NATIVE_CLASS_INFO(UUInt16Property, CoreUObjectPackage, "UInt16Property", "NumericProperty"),
    NATIVE_CLASS_INFO(UUInt32Property, CoreUObjectPackage, "UInt32Property", "NumericProperty"),
    NATIVE_CLASS_INFO(UUInt64Property, CoreUObjectPackage, "UInt64Property", "NumericProperty"),
    NATIVE_CLASS_INFO(UFloatProperty, CoreUObjectPackage, "FloatProperty", "NumericProperty"),
    NATIVE_CLASS_INFO(UDoubleProperty, CoreUObjectPackage, "DoubleProperty", "NumericProperty"),
    NATIVE_CLASS_INFO(UBoolProperty, CoreUObjectPackage, "BoolProperty", "Property"),
    NATIVE_CLASS_INFO(UObjectPropertyBase, CoreUObjectPackage, "ObjectPropertyBase", "Property"),
    NATIVE_CLASS_INFO(UObjectProperty, CoreUObjectPackage, "ObjectProperty", "ObjectPropertyBase"),
    NATIVE_CLASS_INFO(UWeakObjectProperty, CoreUObjectPackage, "WeakObjectProperty", "ObjectPropertyBase"),
    NATIVE_CLASS_INFO(ULazyObjectProperty, CoreUObjectPackage, "LazyObjectProperty", "ObjectPropertyBase"),
    NATIVE_CLASS_INFO(USoftObjectProperty, CoreUObjectPackage, "SoftObjectProperty", "ObjectPropertyBase"),
    NATIVE_CLASS_INFO(UClassProperty, CoreUObjectPackage, "ClassProperty", "ObjectProperty"),
    NATIVE_CLASS_INFO(USoftClassProperty, CoreUObjectPackage, "SoftClassProperty", "SoftObjectProperty"),
    NATIVE_CLASS_INFO(UInterfaceProperty, CoreUObjectPackage, "InterfaceProperty", "Property"),
    NATIVE_CLASS_INFO(UNameProperty, CoreUObjectPackage, "NameProperty", "Property"),
    NATIVE_CLASS_INFO(UStrProperty, CoreUObjectPackage, "StrProperty", "Property"),
    NATIVE_CLASS_INFO(UTextProperty, CoreUObjectPackage, "TextProperty", "Property"),
    NATIVE_CLASS_INFO(UArrayProperty, CoreUObjectPackage, "ArrayProperty", "Property"),
    NATIVE_CLASS_INFO(UMapProperty, CoreUObjectPackage, "MapProperty", "Property"),
    NATIVE_CLASS_INFO(USetProperty, CoreUObjectPackage, "SetProperty", "Property"),
    NATIVE_CLASS_INFO(UStructProperty, CoreUObjectPackage, "StructProperty", "Property"),
    NATIVE_CLASS_INFO(UDelegateProperty, CoreUObjectPackage, "DelegateProperty", "Property"),
    NATIVE_CLASS_INFO(UMulticastDelegateProperty, CoreUObjectPackage, "MulticastDelegateProperty", "Property"),
    NATIVE_CLASS_INFO(UMulticastInlineDelegateProperty, CoreUObjectPackage, "MulticastInlineDelegateProperty", "MulticastDelegateProperty"),
    NATIVE_CLASS_INFO(UMulticastSparseDelegateProperty, CoreUObjectPackage, "MulticastSparseDelegateProperty", "MulticastDelegateProperty"),
    NATIVE_CLASS_INFO(UEnumProperty, CoreUObjectPackage, "EnumProperty", "Property"),
};

}

void RegisterCoreUObject() {
    InitializeScriptCore();
    RegisterNativeClasses(CoreClasses);
    RegisterNativeStructs(CoreStructs);
    RegisterNativeEnums(CoreEnums);
}

}
