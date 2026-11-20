#include "Engine/Source/Runtime/Engine/Engine/SpecularProfile.h"

namespace ue5parse::runtime {

namespace {

void AddWhiteKeys(FRuntimeCurveLinearColor& Curve) {
    for (int32 Channel = 0; Channel < 3; ++Channel) {
        Curve.ColorCurves[Channel].AddKey(0, 1.f);
        Curve.ColorCurves[Channel].AddKey(1, 1.f);
    }
}

}

FSpecularProfileStruct::FSpecularProfileStruct() {
    AddWhiteKeys(ViewColor);
    AddWhiteKeys(LightColor);
}

}
