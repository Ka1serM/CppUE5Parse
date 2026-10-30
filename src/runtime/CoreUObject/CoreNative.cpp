#include <algorithm>
#include <cstring>

#include "CoreUObjectPrivate.h"

#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"

// The CoreUObject module: the VM tables, the intrinsic classes and the core native structs (UE's CoreNative.cpp and
// the NoExportTypes structs with their TStructOpsTypeTraits). A struct that cooked data serializes natively must have
// C++ operations here (or in its module), else the stream of the object that holds it cannot be read.

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
// Equality of structs of number fields, field by field (UE's operator== of the math types: -0 equals 0).
template <class T, class TElement>
bool OpIdenticalElements(const void* A, const void* B) {
    const TElement* ElementsA = static_cast<const TElement*>(A);
    const TElement* ElementsB = static_cast<const TElement*>(B);
    for (std::size_t i = 0; i < sizeof(T) / sizeof(TElement); i++)
        if (!(ElementsA[i] == ElementsB[i])) return false;
    return true;
}
template <class T>
uint32 OpHashBytes(const void* Src) {
    uint32 Hash = 2166136261u;
    const uint8* Bytes = static_cast<const uint8*>(Src);
    for (std::size_t i = 0; i < sizeof(T); i++) Hash = (Hash ^ Bytes[i]) * 16777619u;
    return Hash;
}
template <class T>
bool OpSerialize(FArchive& Ar, void* Data) {
    Ar << *static_cast<T*>(Data);
    return true;
}

// A plain struct whose zeroed memory is its default value, serialized with its operator<<.
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

// A struct the runtime keeps as its serialized bytes: a native struct of another module that nothing reads, which
// only has to keep the stream in step.
template <int32 N>
struct FOpaqueStruct {
    uint8 Bytes[N > 0 ? N : 1];
};
template <int32 N>
bool OpaqueSerialize(FArchive& Ar, void* Data) {
    if constexpr (N > 0) Ar.Serialize(Data, N);
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

// A struct with a constructor or destructor, serialized with a function.
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
const FCppStructOps Vector4fOps = PlainStructOps<FVector4f, float>();
const FCppStructOps RotatorOps = PlainStructOps<FRotator, double>();
const FCppStructOps Rotator3fOps = PlainStructOps<FRotator3f, float>();
// FQuat() does not initialize: a Blueprint quat starts zeroed, not as identity.
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
// FFrameRate has no native serializer in UE: the cook writes its properties.
constexpr FCppStructOps PropertyStructOps(FCppStructOps Ops) {
    Ops.Serialize = nullptr;
    return Ops;
}
const FCppStructOps FrameRateOps = PropertyStructOps(PlainStructOps<FFrameRate, int32>());

bool BoxIdentical(const void* A, const void* B) { return *static_cast<const FBox*>(A) == *static_cast<const FBox*>(B); }
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

bool Box3fIdentical(const void* A, const void* B) { return *static_cast<const FBox3f*>(A) == *static_cast<const FBox3f*>(B); }
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

// FTransform() is the identity. Cooked data writes it as properties (no native serializer).
void TransformConstruct(void* Dest) { new (Dest) FTransform(); }
FCppStructOps MakeTransformOps() {
    FCppStructOps Ops = PlainStructOps<FTransform, double>();
    Ops.Construct = &TransformConstruct;
    Ops.bHasZeroConstructor = false;
    Ops.Serialize = nullptr;
    return Ops;
}
const FCppStructOps TransformOps = MakeTransformOps();

// FRandomStream: its seeds are properties.
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

// FBoxSphereBounds has no native serializer either.
FCppStructOps MakeBoxSphereBoundsOps() {
    FCppStructOps Ops = PlainStructOps<FBoxSphereBounds, double>();
    Ops.Serialize = nullptr;
    return Ops;
}
const FCppStructOps BoxSphereBoundsOps = MakeBoxSphereBoundsOps();

bool SoftObjectPathIdentical(const void* A, const void* B) {
    return *static_cast<const FSoftObjectPath*>(A) == *static_cast<const FSoftObjectPath*>(B);
}
uint32 SoftObjectPathHash(const void* Src) { return GetTypeHash(*static_cast<const FSoftObjectPath*>(Src)); }
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

// FTopLevelAssetPath: a package and an asset name; cooked data writes it as the two names.
bool TopLevelAssetPathIdentical(const void* A, const void* B) {
    return *static_cast<const FTopLevelAssetPath*>(A) == *static_cast<const FTopLevelAssetPath*>(B);
}
bool TopLevelAssetPathSerialize(FArchive& Ar, void* Data) {
    FTopLevelAssetPath& Path = *static_cast<FTopLevelAssetPath*>(Data);
    Ar << Path.PackageName << Path.AssetName;
    return true;
}
const FCppStructOps TopLevelAssetPathOps = ObjectStructOps<FTopLevelAssetPath>(&TopLevelAssetPathSerialize, &TopLevelAssetPathIdentical);

// TPerPlatformProperty: bCooked, the default, and (not cooked, 5.8+) the per-platform map.
template <class T, class TValue>
bool PerPlatformSerialize(FArchive& Ar, void* Data) {
    T& Value = *static_cast<T*>(Data);
    bool bCooked = false;
    Ar << bCooked << Value.Default;
    if (!bCooked && Ar.EngineVer() >= EngineVersion::UE5_8) {
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
const FCppStructOps PerPlatformIntOps = ObjectStructOps<FPerPlatformInt>(&PerPlatformSerialize<FPerPlatformInt, int32>,
                                                                         &PerPlatformIdentical<FPerPlatformInt>);
const FCppStructOps PerPlatformFloatOps = ObjectStructOps<FPerPlatformFloat>(&PerPlatformSerialize<FPerPlatformFloat, float>,
                                                                             &PerPlatformIdentical<FPerPlatformFloat>);
const FCppStructOps PerPlatformBoolOps = ObjectStructOps<FPerPlatformBool>(&PerPlatformSerialize<FPerPlatformBool, bool>,
                                                                           &PerPlatformIdentical<FPerPlatformBool>);
const FCppStructOps PerPlatformFrameRateOps = ObjectStructOps<FPerPlatformFrameRate>(
    &PerPlatformSerialize<FPerPlatformFrameRate, FFrameRate>, &PerPlatformIdentical<FPerPlatformFrameRate>);
bool PerPlatformFStringIdentical(const void* A, const void* B) {
    return static_cast<const FPerPlatformFString*>(A)->Default.Equals(static_cast<const FPerPlatformFString*>(B)->Default);
}
const FCppStructOps PerPlatformFStringOps = ObjectStructOps<FPerPlatformFString>(&PerPlatformSerialize<FPerPlatformFString, FString>,
                                                                                 &PerPlatformFStringIdentical);

// FPerQualityLevelProperty: bCooked, the default, then the map of quality level to value.
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
const FCppStructOps PerQualityLevelIntOps = ObjectStructOps<FPerQualityLevelInt>(&PerQualitySerialize<FPerQualityLevelInt, int32>,
                                                                                 &PerPlatformIdentical<FPerQualityLevelInt>);
const FCppStructOps PerQualityLevelFloatOps = ObjectStructOps<FPerQualityLevelFloat>(&PerQualitySerialize<FPerQualityLevelFloat, float>,
                                                                                     &PerPlatformIdentical<FPerQualityLevelFloat>);

bool InstancedStructSerialize(FArchive& Ar, void* Data) {
    static_cast<FInstancedStruct*>(Data)->Serialize(Ar);
    return true;
}
bool InstancedStructIdentical(const void* A, const void* B) {
    return *static_cast<const FInstancedStruct*>(A) == *static_cast<const FInstancedStruct*>(B);
}
const FCppStructOps InstancedStructOps = ObjectStructOps<FInstancedStruct>(&InstancedStructSerialize, &InstancedStructIdentical);

const FCppStructOps Empty0Ops = OpaqueStructOps<0>();

// FClothTetherData (ClothingSystemRuntimeCommon): its properties, of which the struct has none, then the tethers as an
// array of arrays of (start, end, length). Nothing reads them.
bool ClothTetherDataSerialize(FArchive& Ar, void*) {
    // The unversioned header of no values: fragments up to the last one.
    for (uint16 Fragment = 0; (Fragment & 0x0100) == 0 && !Ar.IsError();) {
        Ar << Fragment;
        if ((Fragment >> 9) != 0) Ar.SetError(ErrorCode::InvalidData, "ClothTetherData.Properties");
    }
    const int32 BatchCount = Ar.ReadCount(4);
    for (int32 i = 0; i < BatchCount && !Ar.IsError(); i++) Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(12)) * 12);
    return true;
}
FCppStructOps MakeClothTetherDataOps() {
    FCppStructOps Ops = OpaqueStructOps<0>();
    Ops.Serialize = &ClothTetherDataSerialize;
    return Ops;
}
const FCppStructOps ClothTetherDataOps = MakeClothTetherDataOps();

// FClothLODDataCommon (ClothingSystemRuntimeCommon): its properties, then the up and down transition skin data, two
// arrays of FMeshToMeshVertData (64 bytes each). Nothing reads the transitions.
bool ClothLODDataCommonSerialize(FArchive& Ar, void* Data) {
    static UScriptStruct* const Struct = FindNativeStruct(FName("ClothLODDataCommon"));
    Struct->SerializeTaggedProperties(Ar, static_cast<uint8*>(Data));
    for (int32 Transition = 0; Transition < 2 && !Ar.IsError(); Transition++) Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(64)) * 64);
    return true;
}
FCppStructOps MakeClothLODDataCommonOps() {
    FCppStructOps Ops = OpaqueStructOps<0>();
    Ops.Serialize = &ClothLODDataCommonSerialize;
    return Ops;
}
const FCppStructOps ClothLODDataCommonOps = MakeClothLODDataCommonOps();
const FCppStructOps Opaque4Ops = OpaqueStructOps<4>();
const FCppStructOps Opaque8Ops = OpaqueStructOps<8>();
const FCppStructOps Opaque12Ops = OpaqueStructOps<12>();
const FCppStructOps Opaque32Ops = OpaqueStructOps<32>();
const FCppStructOps Opaque80Ops = OpaqueStructOps<80>();

#define MEMBER(TType, TName, TMember) FNativeMember{TName, static_cast<int32>(offsetof(TType, TMember)), static_cast<int32>(sizeof(TType::TMember))}

const FNativeMember VectorMembers[] = {MEMBER(FVector, "X", X), MEMBER(FVector, "Y", Y), MEMBER(FVector, "Z", Z)};
const FNativeMember Vector2DMembers[] = {MEMBER(FVector2D, "X", X), MEMBER(FVector2D, "Y", Y)};
const FNativeMember Vector4Members[] = {MEMBER(FVector4, "X", X), MEMBER(FVector4, "Y", Y), MEMBER(FVector4, "Z", Z), MEMBER(FVector4, "W", W)};
const FNativeMember Vector3fMembers[] = {MEMBER(FVector3f, "X", X), MEMBER(FVector3f, "Y", Y), MEMBER(FVector3f, "Z", Z)};
const FNativeMember Vector2fMembers[] = {MEMBER(FVector2f, "X", X), MEMBER(FVector2f, "Y", Y)};
const FNativeMember Vector4fMembers[] = {MEMBER(FVector4f, "X", X), MEMBER(FVector4f, "Y", Y), MEMBER(FVector4f, "Z", Z),
                                         MEMBER(FVector4f, "W", W)};
const FNativeMember RotatorMembers[] = {MEMBER(FRotator, "Pitch", Pitch), MEMBER(FRotator, "Yaw", Yaw), MEMBER(FRotator, "Roll", Roll)};
const FNativeMember Rotator3fMembers[] = {MEMBER(FRotator3f, "Pitch", Pitch), MEMBER(FRotator3f, "Yaw", Yaw), MEMBER(FRotator3f, "Roll", Roll)};
const FNativeMember QuatMembers[] = {MEMBER(FQuat, "X", X), MEMBER(FQuat, "Y", Y), MEMBER(FQuat, "Z", Z), MEMBER(FQuat, "W", W)};
const FNativeMember Quat4fMembers[] = {MEMBER(FQuat4f, "X", X), MEMBER(FQuat4f, "Y", Y), MEMBER(FQuat4f, "Z", Z), MEMBER(FQuat4f, "W", W)};
const FNativeMember PlaneMembers[] = {MEMBER(FPlane, "W", W)};
// The rows of a matrix are its reflected planes.
const FNativeMember MatrixMembers[] = {MEMBER(FMatrix, "XPlane", M[0]), MEMBER(FMatrix, "YPlane", M[1]), MEMBER(FMatrix, "ZPlane", M[2]),
                                       MEMBER(FMatrix, "WPlane", M[3])};
const FNativeMember TransformMembers[] = {MEMBER(FTransform, "Rotation", Rotation), MEMBER(FTransform, "Translation", Translation),
                                          MEMBER(FTransform, "Scale3D", Scale3D)};
const FNativeMember BoxSphereBoundsMembers[] = {MEMBER(FBoxSphereBounds, "Origin", Origin), MEMBER(FBoxSphereBounds, "BoxExtent", BoxExtent),
                                                MEMBER(FBoxSphereBounds, "SphereRadius", SphereRadius)};
const FNativeMember LinearColorMembers[] = {MEMBER(FLinearColor, "R", R), MEMBER(FLinearColor, "G", G), MEMBER(FLinearColor, "B", B),
                                            MEMBER(FLinearColor, "A", A)};
const FNativeMember ColorMembers[] = {MEMBER(FColor, "B", B), MEMBER(FColor, "G", G), MEMBER(FColor, "R", R), MEMBER(FColor, "A", A)};
const FNativeMember GuidMembers[] = {MEMBER(FGuid, "A", a), MEMBER(FGuid, "B", b), MEMBER(FGuid, "C", c), MEMBER(FGuid, "D", d)};
const FNativeMember IntPointMembers[] = {MEMBER(FIntPoint, "X", X), MEMBER(FIntPoint, "Y", Y)};
const FNativeMember IntVectorMembers[] = {MEMBER(FIntVector, "X", X), MEMBER(FIntVector, "Y", Y), MEMBER(FIntVector, "Z", Z)};
const FNativeMember BoxMembers[] = {MEMBER(FBox, "Min", Min), MEMBER(FBox, "Max", Max), MEMBER(FBox, "IsValid", IsValid)};
const FNativeMember Box2DMembers[] = {MEMBER(FBox2D, "Min", Min), MEMBER(FBox2D, "Max", Max), MEMBER(FBox2D, "bIsValid", bIsValid)};
const FNativeMember DateTimeMembers[] = {MEMBER(FDateTime, "Ticks", Ticks)};
const FNativeMember TimespanMembers[] = {MEMBER(FTimespan, "Ticks", Ticks)};
const FNativeMember RandomStreamMembers[] = {MEMBER(FRandomStream, "InitialSeed", InitialSeed), MEMBER(FRandomStream, "Seed", Seed)};
const FNativeMember FrameNumberMembers[] = {MEMBER(FFrameNumber, "Value", Value)};
const FNativeMember FrameRateMembers[] = {MEMBER(FFrameRate, "Numerator", Numerator), MEMBER(FFrameRate, "Denominator", Denominator)};
const FNativeMember PerPlatformFrameRateMembers[] = {MEMBER(FPerPlatformFrameRate, "Default", Default)};
const FNativeMember TopLevelAssetPathMembers[] = {MEMBER(FTopLevelAssetPath, "PackageName", PackageName),
                                                  MEMBER(FTopLevelAssetPath, "AssetName", AssetName)};
const FNativeMember SoftObjectPathMembers[] = {MEMBER(FSoftObjectPath, "AssetPath", AssetPath),
                                               MEMBER(FSoftObjectPath, "SubPathString", SubPathString)};
const FNativeMember PerPlatformIntMembers[] = {MEMBER(FPerPlatformInt, "Default", Default)};
const FNativeMember PerPlatformFloatMembers[] = {MEMBER(FPerPlatformFloat, "Default", Default)};
const FNativeMember PerPlatformBoolMembers[] = {MEMBER(FPerPlatformBool, "Default", Default)};
const FNativeMember PerQualityLevelIntMembers[] = {MEMBER(FPerQualityLevelInt, "Default", Default)};
const FNativeMember PerQualityLevelFloatMembers[] = {MEMBER(FPerQualityLevelFloat, "Default", Default)};

constexpr const char* CoreUObjectPackage = "/Script/CoreUObject";

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
    // As CUE4Parse reads it: an FTransform of doubles.
    {CoreUObjectPackage, "Transform3f", &Opaque80Ops},
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
    {CoreUObjectPackage, "TopLevelAssetPath", &TopLevelAssetPathOps, TopLevelAssetPathMembers},
    {CoreUObjectPackage, "SoftObjectPath", &SoftObjectPathOps, SoftObjectPathMembers},
    {CoreUObjectPackage, "SoftClassPath", &SoftObjectPathOps, SoftObjectPathMembers},
    {CoreUObjectPackage, "StringAssetReference", &SoftObjectPathOps},
    {CoreUObjectPackage, "StringClassReference", &SoftObjectPathOps},
    {CoreUObjectPackage, "PerPlatformInt", &PerPlatformIntOps, PerPlatformIntMembers},
    {CoreUObjectPackage, "PerPlatformFloat", &PerPlatformFloatOps, PerPlatformFloatMembers},
    {CoreUObjectPackage, "PerPlatformBool", &PerPlatformBoolOps, PerPlatformBoolMembers},
    {CoreUObjectPackage, "PerPlatformFrameRate", &PerPlatformFrameRateOps, PerPlatformFrameRateMembers},
    {CoreUObjectPackage, "PerPlatformFString", &PerPlatformFStringOps},
    {CoreUObjectPackage, "PerQualityLevelInt", &PerQualityLevelIntOps, PerQualityLevelIntMembers},
    {CoreUObjectPackage, "PerQualityLevelFloat", &PerQualityLevelFloatOps, PerQualityLevelFloatMembers},
    {CoreUObjectPackage, "InstancedStruct", &InstancedStructOps},
    // Native structs of other modules that only need to stay in step.
    {"/Script/SlateCore", "DeprecateSlateVector2D", &Opaque8Ops},
    {"/Script/NavigationSystem", "NavAgentSelector", &Opaque4Ops},
    {"/Script/MovieScene", "MovieSceneEvaluationKey", &Opaque12Ops},
    {"/Script/MovieScene", "MovieSceneFrameRange", &Opaque32Ops},
    {"/Script/MovieScene", "MovieSceneSegmentIdentifier", &Opaque4Ops},
    {"/Script/MovieScene", "MovieSceneSequenceID", &Opaque4Ops},
    {"/Script/MovieScene", "MovieSceneTrackIdentifier", &Opaque4Ops},
    {CoreUObjectPackage, "UInt32Property", &Opaque4Ops},
    {"/Script/ClothingSystemRuntimeCommon", "ClothTetherData", &ClothTetherDataOps},
    {"/Script/ClothingSystemRuntimeCommon", "ClothLODDataCommon", &ClothLODDataCommonOps},
    // FKeyHandleMap serializes only when transacting in the editor: nothing in cooked data.
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
    NATIVE_CLASS_INFO(USparseDelegateFunction, CoreUObjectPackage, "SparseDelegateFunction", "DelegateFunction", 0,
                      CASTCLASS_USparseDelegateFunction),
    NATIVE_CLASS_INFO(UClass, CoreUObjectPackage, "Class", "Struct", 0, CASTCLASS_UClass),
    NATIVE_CLASS_INFO(UEnum, CoreUObjectPackage, "Enum", "Field", 0, CASTCLASS_UEnum),
    NATIVE_CLASS_INFO(UPackage, CoreUObjectPackage, "Package", "Object", 0, CASTCLASS_UPackage),
    NATIVE_CLASS_INFO(UInterface, CoreUObjectPackage, "Interface", "Object", CLASS_Interface | CLASS_Abstract),
};

}

FInstancedStruct& FInstancedStruct::operator=(const FInstancedStruct& Other) {
    if (this == &Other) return *this;
    InitializeAs(Other.ScriptStruct);
    if (ScriptStruct != nullptr && Other.StructMemory != nullptr) ScriptStruct->CopyScriptStruct(StructMemory, Other.StructMemory);
    return *this;
}

void FInstancedStruct::InitializeAs(const UScriptStruct* InScriptStruct) {
    Reset();
    if (InScriptStruct == nullptr) return;
    ScriptStruct = InScriptStruct;
    int32 Size = std::max(ScriptStruct->GetStructureSize(), 1);
    StructMemory = static_cast<uint8*>(RuntimeMalloc(static_cast<std::size_t>(Size), static_cast<std::size_t>(std::max(ScriptStruct->GetMinAlignment(), 16))));
    ScriptStruct->InitializeStruct(StructMemory);
}

void FInstancedStruct::Reset() {
    if (StructMemory != nullptr) {
        ScriptStruct->DestroyStruct(StructMemory);
        RuntimeFree(StructMemory);
    }
    StructMemory = nullptr;
    ScriptStruct = nullptr;
}

bool FInstancedStruct::operator==(const FInstancedStruct& Other) const {
    if (ScriptStruct != Other.ScriptStruct) return false;
    if (ScriptStruct == nullptr) return true;
    return ScriptStruct->CompareScriptStruct(StructMemory, Other.StructMemory);
}

void FInstancedStruct::Serialize(FArchive& Ar) {
    if (Ar.EngineVer() < EngineVersion::UE5_3) {
        // The legacy editor header is optional; then one version byte.
        int64 At = Ar.Tell();
        uint32 Header = 0;
        Ar << Header;
        if (Header != 0xABABABABu && !Ar.IsError()) Ar.Seek(At);
        uint8 Version = 0;
        Ar << Version;
    }
    UObject* StructObject = nullptr;
    Ar << StructObject;
    int32 SerialSize = 0;
    Ar << SerialSize;
    if (SerialSize < 0 || SerialSize > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::InvalidData, "FInstancedStruct.Size");
        return;
    }
    const int64 End = Ar.Tell() + SerialSize;
    InitializeAs(Cast<UScriptStruct>(StructObject));
    if (ScriptStruct != nullptr && SerialSize > 0) {
        ScriptStruct->SerializeItem(Ar, StructMemory);
        if (Ar.IsError()) return;
    }
    // The size lets a reader pass a struct it does not know.
    Ar.Seek(End);
}

void InitializeModule_CoreUObject() {
    InitializeScriptCore();
    RegisterNativeClasses(CoreClasses);
    RegisterNativeStructs(CoreStructs);
}

}
