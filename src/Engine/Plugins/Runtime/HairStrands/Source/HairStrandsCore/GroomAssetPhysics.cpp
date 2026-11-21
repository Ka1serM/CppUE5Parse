#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetPhysics.h"

namespace ue5parse::runtime {

namespace {

void AddCubicKeys(FRuntimeFloatCurve& Curve, float StartValue, float EndValue) {
    FRichCurve* RichCurve = Curve.GetRichCurve();
    RichCurve->Keys[RichCurve->AddKey(0.0f, StartValue)].InterpMode = RCIM_Cubic;
    RichCurve->Keys[RichCurve->AddKey(1.0f, EndValue)].InterpMode = RCIM_Cubic;
}

}

FHairBendConstraint::FHairBendConstraint() {
    AddCubicKeys(BendScale, 1.0f, 1.0f);
}

FHairStretchConstraint::FHairStretchConstraint() {
    AddCubicKeys(StretchScale, 1.0f, 1.0f);
}

FHairCollisionConstraint::FHairCollisionConstraint() {
    AddCubicKeys(RadiusScale, 1.0f, 0.1f);
}

FHairStrandsParameters::FHairStrandsParameters() {
    AddCubicKeys(ThicknessScale, 1.0f, 1.0f);
}

}
