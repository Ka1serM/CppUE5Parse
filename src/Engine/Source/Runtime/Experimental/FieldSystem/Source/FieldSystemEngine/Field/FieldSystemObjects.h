#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "FieldSystemObjects.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UFieldSystemMetaData : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UFieldSystemMetaData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UFieldSystemMetaDataIteration : public UFieldSystemMetaData {
    GENERATED_BODY()
public:
    explicit UFieldSystemMetaDataIteration(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UFieldSystemMetaDataProcessingResolution : public UFieldSystemMetaData {
    GENERATED_BODY()
public:
    explicit UFieldSystemMetaDataProcessingResolution(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UFieldSystemMetaDataFilter : public UFieldSystemMetaData {
    GENERATED_BODY()
public:
    explicit UFieldSystemMetaDataFilter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS(abstract)
class UFieldNodeBase : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UFieldNodeBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS(abstract)
class UFieldNodeInt : public UFieldNodeBase {
    GENERATED_BODY()
public:
    explicit UFieldNodeInt(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS(abstract)
class UFieldNodeFloat : public UFieldNodeBase {
    GENERATED_BODY()
public:
    explicit UFieldNodeFloat(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS(abstract)
class UFieldNodeVector : public UFieldNodeBase {
    GENERATED_BODY()
public:
    explicit UFieldNodeVector(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UUniformInteger : public UFieldNodeInt {
    GENERATED_BODY()
public:
    explicit UUniformInteger(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class URadialIntMask : public UFieldNodeInt {
    GENERATED_BODY()
public:
    explicit URadialIntMask(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UUniformScalar : public UFieldNodeFloat {
    GENERATED_BODY()
public:
    explicit UUniformScalar(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UWaveScalar : public UFieldNodeFloat {
    GENERATED_BODY()
public:
    explicit UWaveScalar(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class URadialFalloff : public UFieldNodeFloat {
    GENERATED_BODY()
public:
    explicit URadialFalloff(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UPlaneFalloff : public UFieldNodeFloat {
    GENERATED_BODY()
public:
    explicit UPlaneFalloff(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UBoxFalloff : public UFieldNodeFloat {
    GENERATED_BODY()
public:
    explicit UBoxFalloff(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UNoiseField : public UFieldNodeFloat {
    GENERATED_BODY()
public:
    explicit UNoiseField(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UUniformVector : public UFieldNodeVector {
    GENERATED_BODY()
public:
    explicit UUniformVector(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class URadialVector : public UFieldNodeVector {
    GENERATED_BODY()
public:
    explicit URadialVector(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class URotatedRadialVector : public UFieldNodeVector {
    GENERATED_BODY()
public:
    explicit URotatedRadialVector(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class URandomVector : public UFieldNodeVector {
    GENERATED_BODY()
public:
    explicit URandomVector(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UOperatorField : public UFieldNodeBase {
    GENERATED_BODY()
public:
    explicit UOperatorField(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UToIntegerField : public UFieldNodeInt {
    GENERATED_BODY()
public:
    explicit UToIntegerField(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UToFloatField : public UFieldNodeFloat {
    GENERATED_BODY()
public:
    explicit UToFloatField(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UCullingField : public UFieldNodeBase {
    GENERATED_BODY()
public:
    explicit UCullingField(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UReturnResultsTerminal : public UFieldNodeBase {
    GENERATED_BODY()
public:
    explicit UReturnResultsTerminal(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
