#pragma once

#include "Engine/Source/Runtime/Core/Math/Float16.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "VoyagePackedLocalTransform.generated.h"

namespace ue5parse::runtime {

//! The Last Caretaker saves the rotation as a rotator and the scale as half floats.
USTRUCT()
struct FVoyagePackedLocalTransform {
    GENERATED_BODY()
    UPROPERTY()
    FVector3f Location;
    UPROPERTY()
    FQuat4f Rotation;
    UPROPERTY()
    FVector3f Scale;

    bool Serialize(FArchive& Ar) {
        FFloat16 Packed[6];
        Ar << Location;
        for (FFloat16& Value : Packed)
            Ar << Value;
        Rotation = FRotator3f(Packed[0].GetFloat(), Packed[1].GetFloat(), Packed[2].GetFloat()).Quaternion();
        Scale = FVector3f(Packed[3].GetFloat(), Packed[4].GetFloat(), Packed[5].GetFloat());
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FVoyagePackedLocalTransform> : TStructOpsTypeTraitsBase2<FVoyagePackedLocalTransform> {
    enum { WithSerializer = true };
};

}
