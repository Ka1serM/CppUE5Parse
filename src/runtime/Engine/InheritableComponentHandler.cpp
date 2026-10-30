#include "ue5parse/runtime/Engine/Engine/InheritableComponentHandler.h"

#include "ue5parse/runtime/Engine/Engine/SCS_Node.h"
#include "ue5parse/runtime/Engine/Engine/SimpleConstructionScript.h"

// UE's InheritableComponentHandler.cpp (UE 5.3).

namespace ue5parse::runtime {

FComponentKey::FComponentKey(const USCS_Node* SCSNode) {
    if (SCSNode == nullptr) return;
    if (const USimpleConstructionScript* SCS = SCSNode->GetSCS()) OwnerClass = Cast<UClass>(SCS->GetOuter());
    AssociatedGuid = SCSNode->VariableGuid;
    SCSVariableName = SCSNode->GetVariableName();
}

bool FComponentKey::Match(const FComponentKey& OtherKey) const {
    return OwnerClass == OtherKey.OwnerClass && (SCSVariableName == OtherKey.SCSVariableName || AssociatedGuid == OtherKey.AssociatedGuid);
}

UActorComponent* UInheritableComponentHandler::GetOverridenComponentTemplate(const FComponentKey& Key) const {
    for (const FComponentOverrideRecord& Record : Records)
        if (Record.ComponentKey.Match(Key)) return Record.ComponentTemplate;
    return nullptr;
}

}
