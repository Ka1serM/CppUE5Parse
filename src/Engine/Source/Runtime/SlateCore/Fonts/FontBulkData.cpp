#include "Engine/Source/Runtime/SlateCore/Fonts/FontBulkData.h"

namespace ue5parse::runtime {

void UFontBulkData::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    BulkData.Serialize(Ar);
}

}
