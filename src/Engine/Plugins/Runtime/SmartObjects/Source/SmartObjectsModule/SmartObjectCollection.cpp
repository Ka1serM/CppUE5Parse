#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectCollection.h"

namespace ue5parse::runtime {

ASmartObjectCollection::ASmartObjectCollection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bNetLoadOnClient = false;
    bCanBeDamaged = false;
}

}
