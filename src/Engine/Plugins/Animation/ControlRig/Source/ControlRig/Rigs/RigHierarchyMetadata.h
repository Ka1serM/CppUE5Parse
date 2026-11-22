#pragma once

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyDefines.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Core/Math/Rotator.h"

#include "RigHierarchyMetadata.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRigBaseMetadata {
    GENERATED_BODY()

    virtual ~FRigBaseMetadata() = default;

    static FRigBaseMetadata* MakeMetadata(const FName& InName, ERigMetadataType InType);

    virtual void Serialize(FArchive& Ar);

    bool IsValid() const { return GetType() != ERigMetadataType::Invalid; }
    const FName& GetName() const { return Name; }
    ERigMetadataType GetType() const { return Type; }

protected:
    UPROPERTY()
    FName Name;
    UPROPERTY()
    ERigMetadataType Type = ERigMetadataType::Invalid;
};

USTRUCT()
struct FRigBoolMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const bool& GetValue() const { return Value; }

protected:
    UPROPERTY()
    bool Value = false;
};

USTRUCT()
struct FRigBoolArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<bool>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<bool> Value;
};

USTRUCT()
struct FRigFloatMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const float& GetValue() const { return Value; }

protected:
    UPROPERTY()
    float Value = 0;
};

USTRUCT()
struct FRigFloatArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<float>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<float> Value;
};

USTRUCT()
struct FRigInt32Metadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const int32& GetValue() const { return Value; }

protected:
    UPROPERTY()
    int32 Value = 0;
};

USTRUCT()
struct FRigInt32ArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<int32>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<int32> Value;
};

USTRUCT()
struct FRigNameMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const FName& GetValue() const { return Value; }

protected:
    UPROPERTY()
    FName Value;
};

USTRUCT()
struct FRigNameArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<FName>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<FName> Value;
};

USTRUCT()
struct FRigVectorMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const FVector& GetValue() const { return Value; }

protected:
    UPROPERTY()
    FVector Value;
};

USTRUCT()
struct FRigVectorArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<FVector>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<FVector> Value;
};

USTRUCT()
struct FRigRotatorMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const FRotator& GetValue() const { return Value; }

protected:
    UPROPERTY()
    FRotator Value;
};

USTRUCT()
struct FRigRotatorArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<FRotator>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<FRotator> Value;
};

USTRUCT()
struct FRigQuatMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const FQuat& GetValue() const { return Value; }

protected:
    UPROPERTY()
    FQuat Value;
};

USTRUCT()
struct FRigQuatArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<FQuat>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<FQuat> Value;
};

USTRUCT()
struct FRigTransformMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const FTransform& GetValue() const { return Value; }

protected:
    UPROPERTY()
    FTransform Value;
};

USTRUCT()
struct FRigTransformArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<FTransform>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<FTransform> Value;
};

USTRUCT()
struct FRigLinearColorMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const FLinearColor& GetValue() const { return Value; }

protected:
    UPROPERTY()
    FLinearColor Value;
};

USTRUCT()
struct FRigLinearColorArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<FLinearColor>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<FLinearColor> Value;
};

USTRUCT()
struct FRigRigElementKeyMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const FRigElementKey& GetValue() const { return Value; }

protected:
    UPROPERTY()
    FRigElementKey Value;
};

USTRUCT()
struct FRigRigElementKeyArrayMetadata : public FRigBaseMetadata {
    GENERATED_BODY()

    void Serialize(FArchive& Ar) override {
        FRigBaseMetadata::Serialize(Ar);
        Ar << Value;
    }
    const TArray<FRigElementKey>& GetValue() const { return Value; }

protected:
    UPROPERTY()
    TArray<FRigElementKey> Value;
};

}
