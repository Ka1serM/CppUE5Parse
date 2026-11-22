#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime::Chaos {

//! Chaos serializes the components of every real vector as float, also for double vectors.
template <class T>
FArchive& SerializeReal(FArchive& Ar, TVector<T>& Value) {
    float X = static_cast<float>(Value.X);
    float Y = static_cast<float>(Value.Y);
    float Z = static_cast<float>(Value.Z);
    Ar << X << Y << Z;
    Value = TVector<T>(X, Y, Z);
    return Ar;
}

}
