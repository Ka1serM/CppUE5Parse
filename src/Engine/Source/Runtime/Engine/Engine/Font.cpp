#include "Engine/Source/Runtime/Engine/Engine/Font.h"

namespace ue5parse::runtime {

void UFont::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar << CharRemap;
}

}
