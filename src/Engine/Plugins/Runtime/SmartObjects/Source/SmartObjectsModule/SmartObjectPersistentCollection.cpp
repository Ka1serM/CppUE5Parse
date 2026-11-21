#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectPersistentCollection.h"

namespace ue5parse::runtime {

ASmartObjectPersistentCollection::ASmartObjectPersistentCollection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bNetLoadOnClient = false;
    bCanBeDamaged = false;
}

}
