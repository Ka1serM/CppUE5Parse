#pragma once

#include "Engine/Plugins/PCG/Source/PCG/PCGData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PCGSettings.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EPCGSettingsExecutionMode : uint8 { Enabled, Debug, Isolated, Disabled };

USTRUCT()
struct FPCGPropertyAliases {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FName> Aliases;
};

USTRUCT()
struct FPCGSettingsOverridableParam {
    GENERATED_BODY()

    UPROPERTY()
    FName Label;
    UPROPERTY()
    TArray<FName> PropertiesNames;
    UPROPERTY()
    TObjectPtr<const UStruct> PropertyClass = nullptr;
    UPROPERTY()
    TMap<int32, FPCGPropertyAliases> MapOfAliases;
    UPROPERTY()
    bool bHasNameClash = false;
    UPROPERTY()
    bool bSupportsGPU = false;
    UPROPERTY()
    bool bRequiresGPUReadback = false;
    UPROPERTY()
    int32 NumContainers = 0;
};

UCLASS(abstract)
class UPCGSettingsInterface : public UPCGData {
    GENERATED_BODY()
public:
    explicit UPCGSettingsInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bEnabled = true;
    UPROPERTY()
    bool bDebug = false;
};

UCLASS(abstract)
class UPCGSettings : public UPCGSettingsInterface {
    GENERATED_BODY()
public:
    explicit UPCGSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    int32 Seed = static_cast<int32>(0xC35A9631);
    UPROPERTY()
    TSet<FString> FilterOnTags;
    UPROPERTY()
    bool bPassThroughFilteredOutInputs = true;
    UPROPERTY()
    TSet<FString> TagsAppliedOnOutput;
    UPROPERTY()
    EPCGSettingsExecutionMode ExecutionMode = EPCGSettingsExecutionMode::Enabled;
    UPROPERTY()
    bool bUseSeed = false;
    UPROPERTY()
    TArray<FPCGSettingsOverridableParam> CachedOverridableParams;
    UPROPERTY()
    bool bExecuteOnGPU = false;
    UPROPERTY()
    bool bDumpCookedHLSL = false;
    UPROPERTY()
    bool bDumpDataDescriptions = false;
    UPROPERTY()
    bool bPrintShaderDebugValues = false;
    UPROPERTY()
    int32 DebugBufferSize = 16;
    UPROPERTY()
    bool bExecutionDependencyRequired = false;
};

UCLASS()
class UPCGTrivialSettings : public UPCGSettings {
    GENERATED_BODY()
public:
    explicit UPCGTrivialSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UPCGSettingsInstance : public UPCGSettingsInterface {
    GENERATED_BODY()
public:
    explicit UPCGSettingsInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UPCGSettings> Settings = nullptr;
};

}
