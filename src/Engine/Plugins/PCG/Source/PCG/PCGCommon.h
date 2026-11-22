#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "PCGCommon.generated.h"

namespace ue5parse::runtime {

UENUM(meta = (Bitflags))
enum class EPCGDataType : uint32 {
    None = 0,
    Point = 1 << 1,
    Spline = 1 << 2,
    LandscapeSpline = 1 << 3,
    PolyLine = Spline | LandscapeSpline,
    Landscape = 1 << 4,
    Texture = 1 << 5,
    RenderTarget = 1 << 6,
    BaseTexture = Texture | RenderTarget,
    Surface = Landscape | BaseTexture,
    Volume = 1 << 7,
    Primitive = 1 << 8,
    DynamicMesh = 1 << 10,
    Concrete = Point | PolyLine | Surface | Volume | Primitive | DynamicMesh,
    Composite = 1 << 9,
    Spatial = Composite | Concrete,
    Param = 1 << 27,
    PointOrParam = Point | Param,
    Settings = 1 << 28,
    Other = 1 << 29,
    Any = (1 << 30) - 1,
};

UENUM()
enum class EPCGHiGenGrid : uint32 {
    Uninitialized = 0,
    Grid4 = 4,
    Grid8 = 8,
    Grid16 = 16,
    Grid32 = 32,
    Grid64 = 64,
    Grid128 = 128,
    Grid256 = 256,
    Grid512 = 512,
    Grid1024 = 1024,
    Grid2048 = 2048,
    Grid4096 = 4096,
    Grid8192 = 8192,
    Grid16384 = 16384,
    Grid32768 = 32768,
    Grid65536 = 65536,
    Grid131072 = 131072,
    Grid262144 = 262144,
    Grid524288 = 524288,
    Grid1048576 = 1048576,
    Grid2097152 = 2097152,
    Grid4194304 = 4194304,
    Unbounded = 2147483648u,
};

USTRUCT()
struct FPCGRuntimeGenerationRadii {
    GENERATED_BODY()

    UPROPERTY()
    FPerQualityLevelFloat RadiusUnbounded{4294967296.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius400{800.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius800{1600.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius1600{3200.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius3200{6400.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius6400{12800.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius12800{25600.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius25600{51200.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius51200{102400.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius102400{204800.0f};
    UPROPERTY()
    FPerQualityLevelFloat Radius204800{409600.0f};
    UPROPERTY()
    FPerQualityLevelFloat CleanupRadiusScalar{1.1f};
    UPROPERTY()
    double GenerationRadius = 4294967296.0;
    UPROPERTY()
    double GenerationRadius400 = 800.0;
    UPROPERTY()
    double GenerationRadius800 = 1600.0;
    UPROPERTY()
    double GenerationRadius1600 = 3200.0;
    UPROPERTY()
    double GenerationRadius3200 = 6400.0;
    UPROPERTY()
    double GenerationRadius6400 = 12800.0;
    UPROPERTY()
    double GenerationRadius12800 = 25600.0;
    UPROPERTY()
    double GenerationRadius25600 = 51200.0;
    UPROPERTY()
    double GenerationRadius51200 = 102400.0;
    UPROPERTY()
    double GenerationRadius102400 = 204800.0;
    UPROPERTY()
    double GenerationRadius204800 = 409600.0;
    UPROPERTY()
    double CleanupRadiusMultiplier = 1.1;
};

}
