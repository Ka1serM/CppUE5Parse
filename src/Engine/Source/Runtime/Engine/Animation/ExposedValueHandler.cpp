#include "Engine/Source/Runtime/Engine/Animation/ExposedValueHandler.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"
#include "Engine/Source/Runtime/Engine/Animation/PropertyAccess.h"

namespace ue5parse::runtime {

namespace {

void NegateBool(const FProperty* Property, void* Address) {
    const auto* BoolProperty = static_cast<const FBoolProperty*>(Property);
    BoolProperty->SetPropertyValue(Address, !BoolProperty->GetPropertyValue(Address));
}

UFunction* FindBoundFunction(const UClass* InClass, FName BoundFunction) {
    if (BoundFunction.isNone())
        return nullptr;
    UFunction* Function = InClass->FindFunctionByName(BoundFunction);
    UE5PARSE_CHECK(Function != nullptr);
    return Function;
}

void ProcessCopyRecords(const FAnimationBaseContext& Context, const FPropertyAccessLibrary* Library, const TArray<FExposedValueCopyRecord>& CopyRecords) {
    if (CopyRecords.Num() == 0 || Library == nullptr)
        return;
    UObject* Instance = Context.GetAnimInstanceObject();
    for (const FExposedValueCopyRecord& Copy : CopyRecords)
        if (!Copy.bOnlyUpdateWhenActive || Context.IsActive())
            Library->ProcessCopy(Instance, EAnimPropertyAccessCallSite::WorkerThread_Unbatched, Copy.CopyIndex,
                                 Copy.PostCopyOperation == EPostCopyOperation::LogicalNegateBool ? NegateBool : nullptr);
}

}

void FExposedValueHandler::Initialize(UClass* InClass, const FPropertyAccessLibrary& InPropertyAccessLibrary) {
    if (Handler != nullptr) {
        Handler->Initialize(InClass);
        return;
    }
    Function = FindBoundFunction(InClass, BoundFunction);
    PropertyAccessLibrary = &InPropertyAccessLibrary;
}

void FExposedValueHandler::Execute(const FAnimationBaseContext& Context) const {
    if (Handler != nullptr) {
        Handler->Execute(Context);
        return;
    }
    if (Function != nullptr)
        Context.GetAnimInstanceObject()->ProcessEvent(Function, nullptr);
    ProcessCopyRecords(Context, PropertyAccessLibrary, CopyRecords);
}

void FAnimNodeExposedValueHandler_Base::Initialize(const UClass* InClass) {
    Function = FindBoundFunction(InClass, BoundFunction);
}

void FAnimNodeExposedValueHandler_Base::Execute(const FAnimationBaseContext& InContext) const {
    if (Function != nullptr)
        InContext.GetAnimInstanceObject()->ProcessEvent(Function, nullptr);
}

void FAnimNodeExposedValueHandler_PropertyAccess::Initialize(const UClass* InClass) {
    Super::Initialize(InClass);
    if (const auto* AnimClass = Cast<UAnimBlueprintGeneratedClass>(InClass))
        PropertyAccessLibrary = AnimClass->GetPropertyAccessLibrary();
}

void FAnimNodeExposedValueHandler_PropertyAccess::Execute(const FAnimationBaseContext& InContext) const {
    Super::Execute(InContext);
    ProcessCopyRecords(InContext, PropertyAccessLibrary, CopyRecords);
}

}
