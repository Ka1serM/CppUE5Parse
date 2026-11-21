#pragma once

#include "Engine/Plugins/Runtime/PropertyBindingUtils/Source/PropertyBindingUtils/PropertyBindingPath.h"

#include "PropertyBindingBindingCollection.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FPropertyBindingBindingCollection {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPropertyBindingCopyInfoBatch> CopyBatches;
};

}
