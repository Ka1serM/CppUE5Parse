#pragma once

#include "Engine/Plugins/Runtime/Metasound/Source/MetasoundEngine/Interfaces/MetasoundOutputFormatInterfaces.h"
#include "Engine/Plugins/Runtime/Metasound/Source/MetasoundEngine/MetasoundSettings.h"
#include "Engine/Plugins/Runtime/Metasound/Source/MetasoundFrontend/MetasoundChannelAgnosticAutoTypes.h"
#include "Engine/Plugins/Runtime/Metasound/Source/MetasoundFrontend/MetasoundFrontendDocument.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundWaveProcedural.h"

#include "MetasoundSource.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMetaSoundSource : public USoundWaveProcedural {
    GENERATED_BODY()
public:
    explicit UMetaSoundSource(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    FMetasoundFrontendDocument RootMetasoundDocument;
    UPROPERTY()
    TSet<FString> ReferencedAssetClassKeys;
    UPROPERTY()
    TSet<TObjectPtr<UObject>> ReferencedAssetClassObjects;
    UPROPERTY()
    TSet<FSoftObjectPath> ReferenceAssetClassCache;
    UPROPERTY()
    EMetaSoundOutputAudioFormat OutputFormat = EMetaSoundOutputAudioFormat::Mono;
    UPROPERTY()
    EMetasoundChannelAgnosticSourceFormatChooser ChannelAgnosticFormatChooser = EMetasoundChannelAgnosticSourceFormatChooser::Auto;
    UPROPERTY()
    FName ChannelAgnosticCustomFormat;
    UPROPERTY()
    FGuid AssetClassID;

    FMetaSoundQualitySettings CookedQualitySettings;
};

}
