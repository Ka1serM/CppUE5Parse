#include "Engine/Source/Runtime/Engine/VectorField/VectorFieldStatic.h"

namespace ue5parse::runtime {

void UVectorFieldStatic::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    SourceData.Serialize(Ar);
}

}
