#include "Engine/Plugins/Runtime/HairStrands/Source/HairStrandsCore/GroomAssetDataflow.h"

namespace ue5parse::runtime {

bool FGroomDataflowSettings::Serialize(FArchive& Ar) {
    StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this));
    if (Ar.IsError())
        return true;
    Chaos::FChaosArchive ChaosAr(Ar);
    RestCollection->Serialize(ChaosAr);
    ChaosAr.ReportErrorTo(Ar);
    return true;
}

}
