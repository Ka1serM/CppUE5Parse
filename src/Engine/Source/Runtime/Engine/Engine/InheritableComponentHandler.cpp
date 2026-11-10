#include "Engine/Source/Runtime/Engine/Engine/InheritableComponentHandler.h"

#include "Engine/Source/Runtime/Engine/Engine/SCS_Node.h"
#include "Engine/Source/Runtime/Engine/Engine/SimpleConstructionScript.h"

namespace ue5parse::runtime {

FComponentKey::FComponentKey(const USCS_Node* SCSNode) {
    if (SCSNode == nullptr)
        return;
    if (const USimpleConstructionScript* SCS = SCSNode->GetSCS())
        OwnerClass = Cast<UClass>(SCS->GetOuter());
    AssociatedGuid = SCSNode->VariableGuid;
    SCSVariableName = SCSNode->GetVariableName();
}

bool FComponentKey::Match(const FComponentKey& OtherKey) const {
    return OwnerClass == OtherKey.OwnerClass && (SCSVariableName == OtherKey.SCSVariableName || AssociatedGuid == OtherKey.AssociatedGuid);
}

UActorComponent* UInheritableComponentHandler::GetOverridenComponentTemplate(const FComponentKey& Key) const {
    for (const FComponentOverrideRecord& Record : Records)
        if (Record.ComponentKey.Match(Key))
            return Record.ComponentTemplate;
    return nullptr;
}

}
