#pragma once

#include "Engine/Source/Runtime/Engine/Components/InstancedStaticMeshComponent.h"

#include "BuildingSocketComponent.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EBuildingSocketType : uint8 {
    Building = 0,
    Door = 1,
    Pillar = 2,
    WallTrim = 3,
    CastleWall = 4,
    Fence_Foundation = 5,
    Fence_Wall = 6,
    Siege_Foundation = 7,
    Gate = 8,
    Rampart_Defense = 9,
    Corner = 10,
    Hatch = 11,
    Strut = 12,
    Ladder = 13,
    Window = 14,
    Custom_Socket_00 = 15,
    Custom_Socket_01 = 16,
    Custom_Socket_02 = 17,
    Custom_Socket_03 = 18,
    Custom_Socket_04 = 19,
    Custom_Socket_05 = 20,
    Custom_Socket_06 = 21,
    Custom_Socket_07 = 22,
    Custom_Socket_08 = 23,
    Custom_Socket_09 = 24,
    Chimney = 25,
    Chimney_Wall = 26,
    Shutters = 27,
    DoubleDoor = 28,
    Scaffolding = 29,
    CurvedWall = 30,
    SlidingDoor = 31,
    Hearth = 32,
    Rope_Bridge = 33,
};

UENUM()
enum class ESocketConfiguration : uint8 {
    Normal = 0,
    Rotated180 = 1,
    Rotated90 = 2,
    Rotated270 = 3,
    Rotated120 = 4,
    Rotated240 = 5,
};

USTRUCT()
struct FSocketStaticData {
    GENERATED_BODY()
    UPROPERTY()
    TArray<EBuildingSocketType> SocketTypes;
    UPROPERTY()
    TArray<EBuildingSocketType> TargetSocketTypes;
    UPROPERTY()
    int32 AttachToCost = 0;
    UPROPERTY()
    int32 AttachCost = 0;
    UPROPERTY()
    TArray<ESocketConfiguration> OverrideSocketRotations;

    friend FArchive& operator<<(FArchive& Ar, FSocketStaticData& Data);
};

//! Conan Exiles saves a memory size and the static data of the sockets after the properties.
UCLASS()
class UBuildingSocketComponent : public UInstancedStaticMeshComponent {
    GENERATED_BODY()
public:
    explicit UBuildingSocketComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TArray<FSocketStaticData> SocketStaticData;
};

}
