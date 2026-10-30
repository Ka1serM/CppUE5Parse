#include "ue5parse/runtime/Engine/Animation/ExposedValueHandler.h"

#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

void NegateBool(const FProperty* Property, void* Address) {
    const auto* BoolProperty = static_cast<const FBoolProperty*>(Property);
    BoolProperty->SetPropertyValue(Address, !BoolProperty->GetPropertyValue(Address));
}

}

void FExposedValueHandler::Execute(const FAnimationBaseContext& Context) const {
    UObject* Instance = Context.GetAnimInstanceObject();
    if (Function != nullptr) Instance->ProcessEvent(Function, nullptr);
    if (CopyRecords.Num() == 0) return;
    const FPropertyAccessLibrary* Library = Context.GetAnimClass()->GetPropertyAccessLibrary();
    if (Library == nullptr) return;
    for (const FExposedValueCopyRecord& Copy : CopyRecords)
        Library->ProcessCopy(Instance, EAnimPropertyAccessCallSite::WorkerThread_Unbatched, Copy.CopyIndex,
                             Copy.PostCopyOperation == EPostCopyOperation::LogicalNegateBool ? NegateBool : nullptr);
}

}
