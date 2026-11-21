#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCommon.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraMergeable.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPlatformSet.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"
#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraRendererProperties.generated.h"

namespace ue5parse::runtime {

class UTexture;

UCLASS()
class UNiagaraRendererProperties : public UNiagaraMergeable {
    GENERATED_BODY()
public:
    explicit UNiagaraRendererProperties(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    int32 SortOrderHint = 0;
    UPROPERTY()
    bool bIsEnabled = false;
    UPROPERTY()
    bool bMotionBlurEnabled = false;
    UPROPERTY()
    FNiagaraPlatformSet Platforms;
    UPROPERTY()
    ENiagaraRendererMotionVectorSetting MotionVectorSetting = ENiagaraRendererMotionVectorSetting::AutoDetect;
    UPROPERTY()
    FNiagaraVariableAttributeBinding RendererEnabledBinding;
    UPROPERTY()
    bool bAllowInCullProxies = false;
    UPROPERTY()
    FGuid OuterEmitterVersion;
};

UENUM()
enum class ENiagaraRendererSortPrecision : uint8 { Default, Low, High };

UENUM()
enum class ENiagaraRendererGpuTranslucentLatency : uint8 { ProjectDefault, Immediate, Latent };

USTRUCT()
struct FNiagaraRendererMaterialScalarParameter {
    GENERATED_BODY()

    UPROPERTY()
    FName MaterialParameterName;
    UPROPERTY()
    float Value = 0.0f;
};

USTRUCT()
struct FNiagaraRendererMaterialStaticBoolParameter {
    GENERATED_BODY()

    UPROPERTY()
    FName MaterialParameterName;
    UPROPERTY()
    FName StaticVariableName;
    UPROPERTY()
    TOptional<bool> StaticValue;
};

USTRUCT()
struct FNiagaraRendererMaterialTextureParameter {
    GENERATED_BODY()

    UPROPERTY()
    FName MaterialParameterName;
    UPROPERTY()
    TObjectPtr<UTexture> Texture = nullptr;
};

USTRUCT()
struct FNiagaraRendererMaterialVectorParameter {
    GENERATED_BODY()

    UPROPERTY()
    FName MaterialParameterName;
    UPROPERTY()
    FLinearColor Value;
};

USTRUCT()
struct FNiagaraRendererMaterialParameters {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraMaterialAttributeBinding> AttributeBindings;
    UPROPERTY()
    TArray<FNiagaraRendererMaterialScalarParameter> ScalarParameters;
    UPROPERTY()
    TArray<FNiagaraRendererMaterialVectorParameter> VectorParameters;
    UPROPERTY()
    TArray<FNiagaraRendererMaterialTextureParameter> TextureParameters;
    UPROPERTY()
    TArray<FNiagaraRendererMaterialStaticBoolParameter> StaticBoolParameters;
};

}
