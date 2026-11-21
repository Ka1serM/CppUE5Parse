#pragma once

#include "Engine/Plugins/Runtime/Metasound/Source/MetasoundFrontend/MetasoundFrontendDocument.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "Metasound.generated.h"

namespace ue5parse::runtime {

//! The 5.0 patch class, which 5.1 renamed to UMetaSoundPatch.
UCLASS()
class UMetaSound : public UObject {
    GENERATED_BODY()
public:
    explicit UMetaSound(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FMetasoundFrontendDocument RootMetaSoundDocument;
    UPROPERTY()
    TSet<FString> ReferencedAssetClassKeys;
    UPROPERTY()
    TSet<FSoftObjectPath> ReferenceAssetClassCache;
    UPROPERTY()
    FGuid AssetClassID;
};

UCLASS()
class UMetaSoundPatch : public UObject {
    GENERATED_BODY()
public:
    explicit UMetaSoundPatch(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FMetasoundFrontendDocument RootMetaSoundDocument;
    UPROPERTY()
    TSet<FString> ReferencedAssetClassKeys;
    UPROPERTY()
    TSet<TObjectPtr<UObject>> ReferencedAssetClassObjects;
    UPROPERTY()
    TSet<FSoftObjectPath> ReferenceAssetClassCache;
    UPROPERTY()
    FGuid AssetClassID;
};

}
