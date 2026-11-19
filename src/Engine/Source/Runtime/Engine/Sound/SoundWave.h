#pragma once

#include <memory>

#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundBase.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundGroups.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundWaveLoadingBehavior.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundWaveTimecodeInfo.h"

#include "SoundWave.generated.h"

namespace ue5parse::runtime {

class UAssetImportData;
class UCurveTable;
class UWaveformTransformationBase;

UENUM()
enum class ESoundwaveSampleRateSettings : uint8 {
    Max,
    High,
    Medium,
    Low,
    Min,
};

UENUM()
enum class ESoundWaveFFTSize : uint8 {
    VerySmall_64,
    Small_256,
    Medium_512,
    Large_1024,
    VeryLarge_2048,
};

UENUM()
enum class ESoundAssetCompressionType : uint8 {
    BinkAudio,
    ADPCM,
    PCM,
    Opus,
    RADAudio,
    PlatformSpecific,
    ProjectDefined,
};

UENUM()
enum class ESoundWaveCuePointOrigin : uint8 {
    WaveFile,
    MarkerTransformation,
};

UENUM()
enum class ESoundWaveCloudStreamingPlatformEnableType : uint8 {
    Inherited,
    Disabled,
    SWC_MultipleValues,
};

USTRUCT()
struct FSoundWaveCloudStreamingPlatformSettings {
    GENERATED_BODY()
    UPROPERTY()
    ESoundWaveCloudStreamingPlatformEnableType EnablementSetting = ESoundWaveCloudStreamingPlatformEnableType::Inherited;
};

USTRUCT()
struct FSoundWaveSpectralDataEntry {
    GENERATED_BODY()
    UPROPERTY()
    float Magnitude = 0.0f;
    UPROPERTY()
    float NormalizedMagnitude = 0.0f;
};

USTRUCT()
struct FSoundWaveSpectralTimeData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FSoundWaveSpectralDataEntry> Data;
    UPROPERTY()
    float TimeSec = 0.0f;
};

USTRUCT()
struct FSoundWaveEnvelopeTimeData {
    GENERATED_BODY()
    UPROPERTY()
    float Amplitude = 0.0f;
    UPROPERTY()
    float TimeSec = 0.0f;
};

USTRUCT()
struct FSoundWaveCuePoint {
    GENERATED_BODY()
    UPROPERTY()
    int32 CuePointID = INDEX_NONE;
    UPROPERTY()
    FString Label;
    UPROPERTY()
    int64 FramePosition = 0;
    UPROPERTY()
    int64 FrameLength = 0;
    UPROPERTY()
    bool bIsLoopRegion = false;
};

struct FStreamedAudioChunk {
    FByteBulkData BulkData;
    int32 DataSize = 0;
    int32 AudioDataSize = 0;
    int32 SeekOffsetInAudioFrames = INDEX_NONE;

    void Serialize(FArchive& Ar, int32 ChunkIndex);
};

struct FStreamedAudioPlatformData {
    FName AudioFormat;
    TArray<FStreamedAudioChunk> Chunks;

    void Serialize(FArchive& Ar);
};

UCLASS()
class USoundWave : public USoundBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 CompressionQuality = 40;
    UPROPERTY()
    int32 StreamingPriority = 0;
    UPROPERTY()
    ESoundwaveSampleRateSettings SampleRateQuality = ESoundwaveSampleRateSettings::High;
    UPROPERTY()
    TEnumAsByte<ESoundGroup> SoundGroup = SOUNDGROUP_Default;
    UPROPERTY()
    uint8 bLooping : 1 = 0;
    UPROPERTY()
    uint8 bStreaming : 1 = 0;
    UPROPERTY()
    uint8 bSeekableStreaming : 1 = 0;
    UPROPERTY()
    ESoundWaveLoadingBehavior LoadingBehavior = ESoundWaveLoadingBehavior::Inherited;
    UPROPERTY()
    ESoundAssetCompressionType SoundAssetCompressionType = ESoundAssetCompressionType::BinkAudio;
    UPROPERTY()
    uint8 bUseBinkAudio : 1 = 0;
    UPROPERTY()
    uint8 bMature : 1 = 0;
    UPROPERTY()
    uint8 bManualWordWrap : 1 = 0;
    UPROPERTY()
    uint8 bSingleLine : 1 = 0;
    UPROPERTY()
    uint8 bVirtualizeWhenSilent_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 bIsAmbisonics : 1 = 0;
    UPROPERTY()
    uint8 bEnableBakedFFTAnalysis : 1 = 0;
    UPROPERTY()
    uint8 bEnableAmplitudeEnvelopeAnalysis : 1 = 0;
    UPROPERTY()
    uint8 TreatFileAsLoopingForAnalysis : 1 = 0;
    UPROPERTY()
    uint8 bEnableCloudStreaming : 1 = 0;
    UPROPERTY()
    ESoundWaveFFTSize FFTSize = ESoundWaveFFTSize::Medium_512;
    UPROPERTY()
    int32 FFTAnalysisFrameSize = 1024;
    UPROPERTY()
    int32 FFTAnalysisAttackTime = 10;
    UPROPERTY()
    int32 FFTAnalysisReleaseTime = 3000;
    UPROPERTY()
    int32 EnvelopeFollowerFrameSize = 1024;
    UPROPERTY()
    int32 EnvelopeFollowerAttackTime = 10;
    UPROPERTY()
    int32 EnvelopeFollowerReleaseTime = 100;
    UPROPERTY()
    TArray<float> FrequenciesToAnalyze;
    UPROPERTY()
    TArray<FSoundWaveSpectralTimeData> CookedSpectralTimeData;
    UPROPERTY()
    TArray<FSoundWaveEnvelopeTimeData> CookedEnvelopeTimeData;
    UPROPERTY()
    TObjectPtr<USoundWave> OverrideSoundToUseForAnalysis = nullptr;
    UPROPERTY()
    int32 InitialChunkSize_DEPRECATED = 0;
    UPROPERTY()
    FPerPlatformFloat SizeOfFirstAudioChunkInSeconds;
    UPROPERTY()
    TMap<FGuid, FSoundWaveCloudStreamingPlatformSettings> PlatformSettings;
    UPROPERTY()
    FString SpokenText_DEPRECATED;
    UPROPERTY()
    float SubtitlePriority = 10000.0f;
    UPROPERTY()
    float Volume = 1.0f;
    UPROPERTY()
    float Pitch = 1.0f;
    UPROPERTY()
    int32 NumChannels = 0;
    UPROPERTY()
    int32 SampleRate = 0;
    UPROPERTY()
    int32 ImportedSampleRate = 0;
    UPROPERTY()
    int32 RawPCMDataSize = 0;
    UPROPERTY()
    TArray<int32> ChannelOffsets;
    UPROPERTY()
    TArray<int32> ChannelSizes;
    UPROPERTY()
    TArray<FSubtitleCue> Subtitles;
    UPROPERTY()
    TArray<FLocalizedSubtitle> LocalizedSubtitles;
    UPROPERTY()
    FString Comment;
    UPROPERTY()
    FString SourceFilePath_DEPRECATED;
    UPROPERTY()
    FString SourceFileTimestamp_DEPRECATED;
    UPROPERTY()
    TObjectPtr<UAssetImportData> AssetImportData = nullptr;
    UPROPERTY()
    TObjectPtr<UCurveTable> Curves = nullptr;
    UPROPERTY()
    TObjectPtr<UCurveTable> InternalCurves = nullptr;
    UPROPERTY()
    TArray<FSoundWaveCuePoint> CuePoints;
    UPROPERTY()
    ESoundWaveCuePointOrigin CuePointOrigin = ESoundWaveCuePointOrigin::WaveFile;
    UPROPERTY()
    FSoundWaveTimecodeInfo TimecodeInfo;
    UPROPERTY()
    TArray<TObjectPtr<UWaveformTransformationBase>> Transformations;
    UPROPERTY()
    float LUFS = 0.0f;
    UPROPERTY()
    float SamplePeakDB = 0.0f;

    bool bProcedural = false;
    FGuid CompressedDataGuid;
    FFormatContainer CompressedFormatData;
    TArray<FSoundWaveCuePoint> PlatformCuePoints;
    std::unique_ptr<FStreamedAudioPlatformData> RunningPlatformData;

    void Serialize(FArchive& Ar) override;
    virtual void SerializeCuePoints(FArchive& Ar, bool bIsLoadingFromCookedArchive);
    virtual void SerializeCookedPlatformData(FArchive& Ar);
    bool IsStreaming(const FArchive& Ar) const;
};

}
