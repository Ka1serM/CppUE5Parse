#pragma once

#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectPersistentCollection.h"

#include "SmartObjectTestTypes.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ASmartObjectTestCollection : public ASmartObjectPersistentCollection {
    GENERATED_BODY()
public:
    explicit ASmartObjectTestCollection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
