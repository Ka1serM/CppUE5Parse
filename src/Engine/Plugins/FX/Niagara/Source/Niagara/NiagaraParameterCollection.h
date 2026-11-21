#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraUserRedirectionParameterStore.h"

#include "NiagaraParameterCollection.generated.h"

namespace ue5parse::runtime {

class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;
class UNiagaraParameterCollection;

UCLASS()
class UNiagaraParameterCollectionInstance : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraParameterCollectionInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UNiagaraParameterCollection> Collection = nullptr;
    UPROPERTY()
    TArray<FNiagaraVariable> OverridenParameters;
    UPROPERTY()
    FNiagaraParameterStore ParameterStorage;
    UPROPERTY()
    TObjectPtr<UMaterialParameterCollectionInstance> SourceMaterialCollectionInstance = nullptr;
};

UCLASS()
class UNiagaraParameterCollection : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraParameterCollection(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<FNiagaraVariable> Parameters;
    UPROPERTY()
    TObjectPtr<UNiagaraParameterCollectionInstance> DefaultInstance = nullptr;
    UPROPERTY()
    FString UniqueName;
    UPROPERTY()
    FName Namespace;
    UPROPERTY()
    FGuid CompileId;
    UPROPERTY()
    TObjectPtr<UMaterialParameterCollection> SourceMaterialCollection = nullptr;
};

}
