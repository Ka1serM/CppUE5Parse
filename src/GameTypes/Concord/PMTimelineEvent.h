#pragma once

#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PMTimelineEvent.generated.h"

namespace ue5parse::runtime {

//! Concord saves the execution properties of the event as a tagged property list after the properties.
UCLASS()
class UPMTimelineEvent : public UObject {
    GENERATED_BODY()
public:
    explicit UPMTimelineEvent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    FInstancedStruct Properties;
};

//! Concord saves the type of the mapping and then a mapping object, a constant or nothing.
USTRUCT()
struct FPMFloatMapping {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> mMappingDef = nullptr;
    UPROPERTY()
    float mConstant = 0;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FPMFloatMapping> : TStructOpsTypeTraitsBase2<FPMFloatMapping> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPMTimelineSectionFloatMapping : public FPMFloatMapping {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FPMTimelineSectionFloatMapping> : TStructOpsTypeTraitsBase2<FPMTimelineSectionFloatMapping> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPMTimelineSectionEventFloatMapping : public FPMFloatMapping {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FPMTimelineSectionEventFloatMapping> : TStructOpsTypeTraitsBase2<FPMTimelineSectionEventFloatMapping> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPMBoundFloatMapping : public FPMFloatMapping {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FPMBoundFloatMapping> : TStructOpsTypeTraitsBase2<FPMBoundFloatMapping> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FFWAbilityResourceFloatMapping : public FPMFloatMapping {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FFWAbilityResourceFloatMapping> : TStructOpsTypeTraitsBase2<FFWAbilityResourceFloatMapping> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FGameplayEffectFloatMapping : public FPMFloatMapping {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FGameplayEffectFloatMapping> : TStructOpsTypeTraitsBase2<FGameplayEffectFloatMapping> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPMTimelineFloatMapping : public FPMFloatMapping {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FPMTimelineFloatMapping> : TStructOpsTypeTraitsBase2<FPMTimelineFloatMapping> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FFWPerkContextFloatMapping : public FPMFloatMapping {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FFWPerkContextFloatMapping> : TStructOpsTypeTraitsBase2<FFWPerkContextFloatMapping> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPMTimelineObjectBindingDef {
    GENERATED_BODY()
    UPROPERTY()
    FInstancedStruct NonConstStruct;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FPMTimelineObjectBindingDef> : TStructOpsTypeTraitsBase2<FPMTimelineObjectBindingDef> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FGameplayEffectApplicationDataHandle {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UObject> StructType = nullptr;
    UPROPERTY()
    TArray<int32> UnknownValues;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FGameplayEffectApplicationDataHandle> : TStructOpsTypeTraitsBase2<FGameplayEffectApplicationDataHandle> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FPMTimelineRelevancy {
    GENERATED_BODY()
    UPROPERTY()
    FInstancedStruct NonConstStruct;

    bool Serialize(FArchive& Ar) {
        return NonConstStruct.Serialize(Ar);
    }
};

template <>
struct TStructOpsTypeTraits<FPMTimelineRelevancy> : TStructOpsTypeTraitsBase2<FPMTimelineRelevancy> {
    enum { WithSerializer = true };
};

}
