#include "ue5parse/runtime/Engine/Animation/ExposedValueHandler.h"

#include "ue5parse/runtime/Engine/Animation/PropertyAccess.h"

namespace ue5parse::runtime {

bool FExposedValueHandler::Execute(UObject* Instance, const FPropertyAccessLibrary& Library) const {
    if (!Instance) return false;
    if (!BoundFunction.isNone()) {
        UFunction* Bound = Function ? Function : Instance->GetClass()->FindFunctionByName(BoundFunction);
        if (!Bound) return false;
        Instance->ProcessEvent(Bound, nullptr);
    }
    for (const FExposedValueCopyRecord& Copy : CopyRecords)
        if (!Library.ProcessCopy(Instance, Copy.CopyIndex,
                Copy.PostCopyOperation.GetValue() == EPostCopyOperation::LogicalNegateBool)) return false;
    return true;
}

}
