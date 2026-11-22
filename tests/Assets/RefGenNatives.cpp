#include <cstddef>
#include <vector>

#include "AssetSuite.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/AbilitySystemComponent.h"
#include "Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/AttributeSet.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Containers/Set.h"
#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Math/Quat.h"
#include "Engine/Source/Runtime/Core/Math/Rotator.h"
#include "Engine/Source/Runtime/Core/Misc/DateTime.h"
#include "Engine/Source/Runtime/Core/Misc/Timespan.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveFloat.h"
#include "Engine/Source/Runtime/Engine/Engine/DataAsset.h"
#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"

namespace test::assets {

namespace {

using ue5parse::EGame;
using rt::FNativeClassInfo;
using rt::FNativeMember;
using rt::InternalConstructor;
using rt::int32;

enum class ERefNativeEnum : rt::uint8 { First, Second, Third };

struct FRefInnerStruct {
    int32 count = 0;
    rt::FName label;
    rt::TArray<float> weights;
};

struct FRefTableRow : rt::FTableRowBase {
    rt::FText displayName;
    int32 damage = 0;
    float range = 0.f;
    ERefNativeEnum kind = ERefNativeEnum::First;
    rt::TSoftObjectPtr<rt::UObject> asset;
    rt::FGameplayTag tag;
};

class URefDataAsset : public rt::UPrimaryDataAsset {
public:
    using UPrimaryDataAsset::UPrimaryDataAsset;

    bool bFlag = false;
    rt::uint8 byteValue = 0;
    rt::int8 int8Value = 0;
    rt::int16 int16Value = 0;
    rt::uint16 uint16Value = 0;
    int32 int32Value = 0;
    rt::uint32 uint32Value = 0;
    rt::int64 int64Value = 0;
    rt::uint64 uint64Value = 0;
    float floatValue = 0.f;
    double doubleValue = 0.0;
    rt::FString string;
    rt::FName name;
    rt::FText text;
    ERefNativeEnum enumValue = ERefNativeEnum::First;
    int32 flags = 0;
    rt::FVector vector = rt::FVector::ZeroVector;
    rt::FVector2D vector2D = rt::FVector2D::ZeroVector;
    rt::FVector4 vector4 = rt::FVector4(0, 0, 0, 0);
    rt::FRotator rotator = rt::FRotator::ZeroRotator;
    rt::FQuat quat = rt::FQuat::Identity;
    rt::FTransform transform;
    rt::FLinearColor linearColor = rt::FLinearColor::White;
    rt::FColor color = rt::FColor::White;
    rt::FBox box;
    rt::FGuid guid;
    rt::FDateTime dateTime;
    rt::FTimespan timespan;
    rt::FSoftObjectPath softPath;
    rt::FSoftClassPath softClassPath;
    rt::TObjectPtr<rt::UObject> hardObject = nullptr;
    rt::TSoftObjectPtr<rt::UObject> softObject;
    rt::TSubclassOf<rt::AActor> actorClass;
    rt::TSoftClassPtr<rt::AActor> softActorClass;
    rt::TWeakObjectPtr<rt::UObject> weakObject;
    rt::FGameplayTag tag;
    rt::FGameplayTagContainer tags;
    FRefInnerStruct inner;
    rt::TArray<int32> intArray;
    rt::TArray<rt::FString> stringArray;
    rt::TArray<FRefInnerStruct> structArray;
    rt::TArray<rt::TObjectPtr<rt::UObject>> objectArray;
    rt::TSet<rt::FName> nameSet;
    rt::TMap<rt::FName, int32> nameToInt;
    rt::TMap<rt::FString, FRefInnerStruct> stringToStruct;
    rt::TMap<int32, rt::TSoftObjectPtr<rt::UObject>> intToSoft;
    rt::FDataTableRowHandle rowHandle;
    rt::FRuntimeFloatCurve runtimeCurve;
    rt::TObjectPtr<rt::UObject> instancedSubobject = nullptr;
    int32 staticArray[3] = {0, 0, 0};
};

class URefInstancedObject : public rt::UObject {
public:
    using UObject::UObject;

    float value = 0.f;
    rt::FString note;
};

class URefAttributeSet : public rt::UAttributeSet {
public:
    using UAttributeSet::UAttributeSet;

    rt::FGameplayAttributeData health;
    rt::FGameplayAttributeData armor;
};

//! RefGen's ue4_compat.py turns the attributes into plain floats before UE 4.13.
class URefFloatAttributeSet : public rt::UAttributeSet {
public:
    using UAttributeSet::UAttributeSet;

    float health = 0.f;
    float armor = 0.f;
};

class ARefNativeActor : public rt::AActor {
public:
    explicit ARefNativeActor(const rt::FObjectInitializer& initializer) : AActor(initializer) {
        root = initializer.CreateDefaultSubobject<rt::USceneComponent>(rt::FName("Root"));
        RootComponent = root;
        if (rt::GetRuntimeProvider().getGame() >= EGame::GAME_UE4_8)
            abilitySystem = initializer.CreateDefaultSubobject<rt::UAbilitySystemComponent>(rt::FName("AbilitySystem"));
    }

    rt::TObjectPtr<rt::USceneComponent> root = nullptr;
    rt::TObjectPtr<rt::UAbilitySystemComponent> abilitySystem = nullptr;
    rt::TArray<rt::TObjectPtr<rt::AActor>> linkedActors;
    FRefInnerStruct settings;
};

//! RefGen's ue4_compat.py renames a member named like a core struct to <Name>Value before UE 4.8.
struct FRefMember {
    FNativeMember native;
    EGame since = EGame::GAME_UE4_0;
    const char* nameBeforeUE48 = nullptr;
};

constexpr rt::uint64 instancedFlags = rt::CPF_PersistentInstance | rt::CPF_ExportObject | rt::CPF_InstancedReference;
constexpr rt::uint64 componentFlags = rt::CPF_InstancedReference | rt::CPF_ExportObject;

#define REF_MEMBER(Owner, Member, Name, Type, ...)                                                                                                                  \
    FRefMember { FNativeMember{Name, Type, static_cast<int32>(offsetof(Owner, Member)), static_cast<int32>(sizeof(Owner::Member))}, __VA_ARGS__ }

#define REF_MEMBER_FLAGS(Owner, Member, Name, Type, Flags)                                                                                                          \
    FRefMember { FNativeMember{Name, Type, static_cast<int32>(offsetof(Owner, Member)), static_cast<int32>(sizeof(Owner::Member)), Flags} }

const FRefMember innerStructMembers[] = {
    REF_MEMBER(FRefInnerStruct, count, "Count", "int32"),
    REF_MEMBER(FRefInnerStruct, label, "Label", "Name"),
    REF_MEMBER(FRefInnerStruct, weights, "Weights", "Array<float>"),
};

const FRefMember tableRowMembers[] = {
    REF_MEMBER(FRefTableRow, displayName, "DisplayName", "Text"),
    REF_MEMBER(FRefTableRow, damage, "Damage", "int32"),
    REF_MEMBER(FRefTableRow, range, "Range", "float"),
    REF_MEMBER(FRefTableRow, kind, "Kind", "Enum:ERefNativeEnum:uint8"),
    REF_MEMBER(FRefTableRow, asset, "Asset", "SoftObject:Object"),
    REF_MEMBER(FRefTableRow, tag, "Tag", "Struct:GameplayTag", EGame::GAME_UE4_8),
};

const FRefMember dataAssetMembers[] = {
    REF_MEMBER(URefDataAsset, bFlag, "bFlag", "bool"),
    REF_MEMBER(URefDataAsset, byteValue, "Byte", "uint8"),
    REF_MEMBER(URefDataAsset, int8Value, "Int8", "int8"),
    REF_MEMBER(URefDataAsset, int16Value, "Int16", "int16"),
    REF_MEMBER(URefDataAsset, uint16Value, "UInt16", "uint16"),
    REF_MEMBER(URefDataAsset, int32Value, "Int32", "int32"),
    REF_MEMBER(URefDataAsset, uint32Value, "UInt32", "uint32"),
    REF_MEMBER(URefDataAsset, int64Value, "Int64", "int64"),
    REF_MEMBER(URefDataAsset, uint64Value, "UInt64", "uint64"),
    REF_MEMBER(URefDataAsset, floatValue, "Float", "float"),
    REF_MEMBER(URefDataAsset, doubleValue, "Double", "double"),
    REF_MEMBER(URefDataAsset, string, "String", "Str"),
    REF_MEMBER(URefDataAsset, name, "Name", "Name"),
    REF_MEMBER(URefDataAsset, text, "Text", "Text"),
    REF_MEMBER(URefDataAsset, enumValue, "Enum", "Enum:ERefNativeEnum:uint8"),
    REF_MEMBER(URefDataAsset, flags, "Flags", "int32"),
    REF_MEMBER(URefDataAsset, vector, "Vector", "Struct:Vector", EGame::GAME_UE4_0, "VectorValue"),
    REF_MEMBER(URefDataAsset, vector2D, "Vector2D", "Struct:Vector2D", EGame::GAME_UE4_0, "Vector2DValue"),
    REF_MEMBER(URefDataAsset, vector4, "Vector4", "Struct:Vector4", EGame::GAME_UE4_0, "Vector4Value"),
    REF_MEMBER(URefDataAsset, rotator, "Rotator", "Struct:Rotator", EGame::GAME_UE4_0, "RotatorValue"),
    REF_MEMBER(URefDataAsset, quat, "Quat", "Struct:Quat", EGame::GAME_UE4_0, "QuatValue"),
    REF_MEMBER(URefDataAsset, transform, "Transform", "Struct:Transform", EGame::GAME_UE4_0, "TransformValue"),
    REF_MEMBER(URefDataAsset, linearColor, "LinearColor", "Struct:LinearColor", EGame::GAME_UE4_0, "LinearColorValue"),
    REF_MEMBER(URefDataAsset, color, "Color", "Struct:Color", EGame::GAME_UE4_0, "ColorValue"),
    REF_MEMBER(URefDataAsset, box, "Box", "Struct:Box", EGame::GAME_UE4_0, "BoxValue"),
    REF_MEMBER(URefDataAsset, guid, "Guid", "Struct:Guid", EGame::GAME_UE4_0, "GuidValue"),
    REF_MEMBER(URefDataAsset, dateTime, "DateTime", "Struct:DateTime", EGame::GAME_UE4_0, "DateTimeValue"),
    REF_MEMBER(URefDataAsset, timespan, "Timespan", "Struct:Timespan", EGame::GAME_UE4_0, "TimespanValue"),
    REF_MEMBER(URefDataAsset, softPath, "SoftPath", "Struct:SoftObjectPath"),
    REF_MEMBER(URefDataAsset, softClassPath, "SoftClassPath", "Struct:SoftClassPath"),
    REF_MEMBER(URefDataAsset, hardObject, "HardObject", "Object:Object"),
    REF_MEMBER(URefDataAsset, softObject, "SoftObject", "SoftObject:Object"),
    REF_MEMBER(URefDataAsset, actorClass, "ActorClass", "Class:Actor"),
    REF_MEMBER(URefDataAsset, softActorClass, "SoftActorClass", "SoftClass:Actor"),
    REF_MEMBER(URefDataAsset, weakObject, "WeakObject", "WeakObject:Object"),
    REF_MEMBER(URefDataAsset, tag, "Tag", "Struct:GameplayTag", EGame::GAME_UE4_8),
    REF_MEMBER(URefDataAsset, tags, "Tags", "Struct:GameplayTagContainer", EGame::GAME_UE4_8),
    REF_MEMBER(URefDataAsset, inner, "Inner", "Struct:RefInnerStruct"),
    REF_MEMBER(URefDataAsset, intArray, "IntArray", "Array<int32>"),
    REF_MEMBER(URefDataAsset, stringArray, "StringArray", "Array<Str>"),
    REF_MEMBER(URefDataAsset, structArray, "StructArray", "Array<Struct:RefInnerStruct>"),
    REF_MEMBER(URefDataAsset, objectArray, "ObjectArray", "Array<Object:Object>"),
    REF_MEMBER(URefDataAsset, nameSet, "NameSet", "Set<Name>", EGame::GAME_UE4_13),
    REF_MEMBER(URefDataAsset, nameToInt, "NameToInt", "Map<Name,int32>", EGame::GAME_UE4_8),
    REF_MEMBER(URefDataAsset, stringToStruct, "StringToStruct", "Map<Str,Struct:RefInnerStruct>", EGame::GAME_UE4_8),
    REF_MEMBER(URefDataAsset, intToSoft, "IntToSoft", "Map<int32,SoftObject:Object>", EGame::GAME_UE4_8),
    REF_MEMBER(URefDataAsset, rowHandle, "RowHandle", "Struct:DataTableRowHandle"),
    REF_MEMBER(URefDataAsset, runtimeCurve, "RuntimeCurve", "Struct:RuntimeFloatCurve"),
    REF_MEMBER_FLAGS(URefDataAsset, instancedSubobject, "InstancedSubobject", "Object:Object", instancedFlags),
    REF_MEMBER(URefDataAsset, staticArray, "StaticArray", "int32"),
};

const FRefMember instancedObjectMembers[] = {
    REF_MEMBER(URefInstancedObject, value, "Value", "float"),
    REF_MEMBER(URefInstancedObject, note, "Note", "Str"),
};

const FRefMember attributeSetMembers[] = {
    REF_MEMBER(URefAttributeSet, health, "Health", "Struct:GameplayAttributeData"),
    REF_MEMBER(URefAttributeSet, armor, "Armor", "Struct:GameplayAttributeData"),
};

const FRefMember floatAttributeSetMembers[] = {
    REF_MEMBER(URefFloatAttributeSet, health, "Health", "float"),
    REF_MEMBER(URefFloatAttributeSet, armor, "Armor", "float"),
};

const FRefMember nativeActorMembers[] = {
    REF_MEMBER_FLAGS(ARefNativeActor, root, "Root", "Object:SceneComponent", componentFlags),
    FRefMember{FNativeMember{"AbilitySystem", "Object:AbilitySystemComponent", static_cast<int32>(offsetof(ARefNativeActor, abilitySystem)),
                             static_cast<int32>(sizeof(ARefNativeActor::abilitySystem)), componentFlags},
               EGame::GAME_UE4_8},
    REF_MEMBER(ARefNativeActor, linkedActors, "LinkedActors", "Array<Object:Actor>"),
    REF_MEMBER(ARefNativeActor, settings, "Settings", "Struct:RefInnerStruct"),
};

#undef REF_MEMBER
#undef REF_MEMBER_FLAGS

const rt::FNativeEnumName nativeEnumNames[] = {{"ERefNativeEnum::First", 0}, {"ERefNativeEnum::Second", 1}, {"ERefNativeEnum::Third", 2}};
const rt::FNativeEnumName flagNames[] = {{"ERefFlags::None", 0}, {"ERefFlags::A", 1}, {"ERefFlags::B", 2}, {"ERefFlags::C", 4}};
const rt::FNativeEnumInfo enums[] = {{"/Script/RefGen", "ERefNativeEnum", nativeEnumNames}, {"/Script/RefGen", "ERefFlags", flagNames}};

constexpr rt::FCppStructOps innerStructOps = rt::TCppStructOps<FRefInnerStruct>();
constexpr rt::FCppStructOps tableRowOps = rt::TCppStructOps<FRefTableRow>();

class RefGenNatives {
public:
    explicit RefGenNatives(EGame game)
        : innerStruct(select(innerStructMembers, game)), tableRow(select(tableRowMembers, game)), dataAsset(select(dataAssetMembers, game)),
          instancedObject(select(instancedObjectMembers, game)), nativeActor(select(nativeActorMembers, game)),
          attributeSet(game >= EGame::GAME_UE4_13 ? select(attributeSetMembers, game) : select(floatAttributeSetMembers, game)) {
        structs.push_back({"/Script/RefGen", "RefInnerStruct", &innerStructOps, innerStruct});
        structs.push_back({"/Script/RefGen", "RefTableRow", &tableRowOps, tableRow, "TableRowBase"});
        //! RefGen's ue4_compat.py derives the data asset from UDataAsset before UE 4.16, which has no UPrimaryDataAsset.
        const char* dataAssetSuper = game >= EGame::GAME_UE4_16 ? "PrimaryDataAsset" : "DataAsset";
        classes.push_back(NATIVE_CLASS_INFO(URefDataAsset, "/Script/RefGen", "RefDataAsset", dataAssetSuper, 0, 0, dataAsset));
        classes.push_back(NATIVE_CLASS_INFO(URefInstancedObject, "/Script/RefGen", "RefInstancedObject", "Object",
                                                rt::CLASS_EditInlineNew | rt::CLASS_DefaultToInstanced, 0, instancedObject));
        classes.push_back(NATIVE_CLASS_INFO(ARefNativeActor, "/Script/RefGen", "RefNativeActor", "Actor", 0, rt::CASTCLASS_AActor, nativeActor));
        //! UE 4.7 and older have no GameplayAbilities, so RefGen drops the attribute set.
        if (game >= EGame::GAME_UE4_13)
            classes.push_back(NATIVE_CLASS_INFO(URefAttributeSet, "/Script/RefGen", "RefAttributeSet", "AttributeSet", 0, 0, attributeSet));
        else if (game >= EGame::GAME_UE4_8)
            classes.push_back(NATIVE_CLASS_INFO(URefFloatAttributeSet, "/Script/RefGen", "RefAttributeSet", "AttributeSet", 0, 0, attributeSet));
        rt::RegisterNativeEnums(enums);
        rt::RegisterNativeStructs(structs);
        rt::RegisterNativeClasses(classes);
    }

private:
    static std::vector<FNativeMember> select(std::span<const FRefMember> members, EGame game) {
        std::vector<FNativeMember> selected;
        for (const FRefMember& member : members) {
            if (game < member.since)
                continue;
            selected.push_back(member.native);
            if (game < EGame::GAME_UE4_8 && member.nameBeforeUE48 != nullptr)
                selected.back().Name = member.nameBeforeUE48;
        }
        return selected;
    }

    std::vector<FNativeMember> innerStruct;
    std::vector<FNativeMember> tableRow;
    std::vector<FNativeMember> dataAsset;
    std::vector<FNativeMember> instancedObject;
    std::vector<FNativeMember> nativeActor;
    std::vector<FNativeMember> attributeSet;
    std::vector<rt::FNativeStructInfo> structs;
    std::vector<rt::FNativeClassInfo> classes;
};

}

void registerRefGenNatives(EGame game) {
    static const RefGenNatives natives(game);
}

}
