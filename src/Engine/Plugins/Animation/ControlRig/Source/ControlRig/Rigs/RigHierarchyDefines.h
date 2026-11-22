#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"
#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/Core/Templates/TypeHash.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "RigHierarchyDefines.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ERigElementType : uint8 {
    None = 0,
    Bone = 0x001,
    Null = 0x002,
    Space = Null UMETA(Hidden),
    Control = 0x004,
    Curve = 0x008,
    Physics = 0x010 UMETA(Hidden),
    //! UE before 5.5 names the physics type RigidBody.
    RigidBody = Physics UMETA(Hidden),
    Reference = 0x020,
    Connector = 0x040,
    Socket = 0x080,
    All = Bone | Null | Control | Curve | Reference | Connector | Socket,
    First = Bone UMETA(Hidden),
    Last = Socket UMETA(Hidden),
};

UENUM()
enum class ERigBoneType : uint8 { Imported, User };

UENUM()
enum class ERigMetadataType : uint8 {
    Bool,
    BoolArray,
    Float,
    FloatArray,
    Int32,
    Int32Array,
    Name,
    NameArray,
    Vector,
    VectorArray,
    Rotator,
    RotatorArray,
    Quat,
    QuatArray,
    Transform,
    TransformArray,
    LinearColor,
    LinearColorArray,
    RigElementKey,
    RigElementKeyArray,
    Invalid UMETA(Hidden),
};

struct FRigHierarchySerializationSettings {
    explicit FRigHierarchySerializationSettings(const FArchive& InArchive);

    enum ESerializationPhase { StaticData, InterElementData };

    void Load(FArchive& InArchive);

    FControlRigObjectVersion::Type ControlRigVersion = FControlRigObjectVersion::LatestVersion;
    bool bIsSerializingToPackage = false;
    bool bUseCompressedArchive = false;
    bool bStoreCompactTransforms = true;
    bool bSerializeLocalTransform = true;
    bool bSerializeGlobalTransform = true;
    bool bSerializeInitialTransform = true;
    bool bSerializeCurrentTransform = true;
    ESerializationPhase SerializationPhase = ESerializationPhase::StaticData;
};

UENUM()
enum class ERigControlType : uint8 {
    Bool,
    Float,
    Integer,
    Vector2D,
    Position,
    Scale,
    Rotator,
    Transform UMETA(Hidden),
    TransformNoScale UMETA(Hidden),
    EulerTransform,
    ScaleFloat,
};

UENUM()
enum class ERigControlAnimationType : uint8 { AnimationControl, AnimationChannel, ProxyControl, VisualCue };

UENUM()
enum class ERigControlVisibility : uint8 { UserDefined, BasedOnSelection };

UENUM()
enum class ERigControlAxis : uint8 { X, Y, Z };

USTRUCT()
struct FRigControlLimitEnabled {
    GENERATED_BODY()
    UPROPERTY()
    bool bMinimum = false;
    UPROPERTY()
    bool bMaximum = false;

    FRigControlLimitEnabled& Set(bool InValue) {
        bMinimum = bMaximum = InValue;
        return *this;
    }

    void Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigControlLimitEnabled& P) {
        P.Serialize(Ar);
        return Ar;
    }
};

USTRUCT()
struct FRigControlValueStorage {
    GENERATED_BODY()
    UPROPERTY()
    float Float00 = 0;
    UPROPERTY()
    float Float01 = 0;
    UPROPERTY()
    float Float02 = 0;
    UPROPERTY()
    float Float03 = 0;
    UPROPERTY()
    float Float10 = 0;
    UPROPERTY()
    float Float11 = 0;
    UPROPERTY()
    float Float12 = 0;
    UPROPERTY()
    float Float13 = 0;
    UPROPERTY()
    float Float20 = 0;
    UPROPERTY()
    float Float21 = 0;
    UPROPERTY()
    float Float22 = 0;
    UPROPERTY()
    float Float23 = 0;
    UPROPERTY()
    float Float30 = 0;
    UPROPERTY()
    float Float31 = 0;
    UPROPERTY()
    float Float32 = 0;
    UPROPERTY()
    float Float33 = 0;
    UPROPERTY()
    float Float00_2 = 0;
    UPROPERTY()
    float Float01_2 = 0;
    UPROPERTY()
    float Float02_2 = 0;
    UPROPERTY()
    float Float03_2 = 0;
    UPROPERTY()
    float Float10_2 = 0;
    UPROPERTY()
    float Float11_2 = 0;
    UPROPERTY()
    float Float12_2 = 0;
    UPROPERTY()
    float Float13_2 = 0;
    UPROPERTY()
    float Float20_2 = 0;
    UPROPERTY()
    float Float21_2 = 0;
    UPROPERTY()
    float Float22_2 = 0;
    UPROPERTY()
    float Float23_2 = 0;
    UPROPERTY()
    float Float30_2 = 0;
    UPROPERTY()
    float Float31_2 = 0;
    UPROPERTY()
    float Float32_2 = 0;
    UPROPERTY()
    float Float33_2 = 0;
    UPROPERTY()
    bool bValid = false;
};

USTRUCT()
struct FRigControlValue {
    GENERATED_BODY()
    UPROPERTY()
    FRigControlValueStorage FloatStorage;
    UPROPERTY()
    FTransform Storage_DEPRECATED;

    friend FArchive& operator<<(FArchive& Ar, FRigControlValue& Value);
};

USTRUCT()
struct FRigElementKey {
    GENERATED_BODY()
    UPROPERTY()
    ERigElementType Type = ERigElementType::None;
    UPROPERTY()
    FName Name;

    bool IsValid() const { return !Name.isNone() && Type != ERigElementType::None; }

    void Serialize(FArchive& Ar);
    void Load(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigElementKey& P) {
        P.Serialize(Ar);
        return Ar;
    }

    bool operator==(const FRigElementKey& Other) const { return Name == Other.Name && Type == Other.Type; }
    friend uint32 GetTypeHash(const FRigElementKey& Key) { return GetTypeHash(Key.Name) * 10 + static_cast<uint32>(Key.Type); }
};

USTRUCT()
struct FRigComponentKey {
    GENERATED_BODY()
    UPROPERTY()
    FRigElementKey ElementKey;
    UPROPERTY()
    FName Name;

    void Serialize(FArchive& Ar);
    void Load(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigComponentKey& P) {
        P.Serialize(Ar);
        return Ar;
    }

    bool operator==(const FRigComponentKey& Other) const { return Name == Other.Name && ElementKey == Other.ElementKey; }
    friend uint32 GetTypeHash(const FRigComponentKey& Key) { return GetTypeHash(Key.Name) * 15 + GetTypeHash(Key.ElementKey); }
};

USTRUCT()
struct FRigHierarchyKey {
    GENERATED_BODY()

    FRigHierarchyKey() = default;
    FRigHierarchyKey(const FRigElementKey& InElementKey) {
        if (InElementKey.IsValid())
            Element = InElementKey;
    }

    bool IsElement() const { return Element.IsSet(); }
    bool IsComponent() const { return Component.IsSet(); }

    void Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigHierarchyKey& P) {
        P.Serialize(Ar);
        return Ar;
    }

    bool operator==(const FRigHierarchyKey& Other) const {
        if (IsElement() != Other.IsElement() || IsComponent() != Other.IsComponent())
            return false;
        if (IsElement() && !(*Element == *Other.Element))
            return false;
        return !IsComponent() || *Component == *Other.Component;
    }
    friend uint32 GetTypeHash(const FRigHierarchyKey& InKey) {
        if (InKey.IsElement())
            return GetTypeHash(*InKey.Element);
        if (InKey.IsComponent())
            return GetTypeHash(*InKey.Component);
        return 0;
    }

    TOptional<FRigElementKey> Element;
    TOptional<FRigComponentKey> Component;
};

}
