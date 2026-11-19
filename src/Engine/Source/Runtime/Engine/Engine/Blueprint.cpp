#include "Engine/Source/Runtime/Engine/Engine/Blueprint.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

void UBlueprintCore::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar << bLegacyGeneratedClassIsAuthoritative;
}

}
