#include "Engine/Source/Runtime/Engine/Animation/ExposedValueHandler.h"

#include <cstring>

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

void* GetDestAddress(const FExposedValueCopyRecord& Copy, UObject* Instance, const FStructProperty* NodeProperty) {
    void* Container = Copy.bInstanceIsTarget ? static_cast<void*>(Instance) : NodeProperty->ContainerPtrToValuePtr<void>(Instance);
    const FProperty* DestProperty = Copy.DestProperty.Get();
    if (const auto* DestArray = CastField<FArrayProperty>(DestProperty)) {
        FScriptArrayHelper Helper(DestArray, DestArray->ContainerPtrToValuePtr<void>(Container));
        return Helper.GetRawPtr(Copy.DestArrayIndex);
    }
    return DestProperty->ContainerPtrToValuePtr<void>(Container, Copy.DestArrayIndex);
}

const void* GetSourceAddress(const FExposedValueCopyRecord& Copy, const UObject* Instance) {
    const FProperty* SourceProperty = Copy.CachedSourceProperty.Get();
    if (const FProperty* SubProperty = Copy.CachedSourceStructSubProperty.Get())
        return SubProperty->ContainerPtrToValuePtr<void>(SourceProperty->ContainerPtrToValuePtr<void>(Instance), Copy.SourceArrayIndex);
    return SourceProperty->ContainerPtrToValuePtr<void>(Instance, Copy.SourceArrayIndex);
}

void ProcessDirectCopy(const FExposedValueCopyRecord& Copy, UObject* Instance, const FStructProperty* NodeProperty) {
    const FProperty* SourceProperty = Copy.CachedSourceStructSubProperty.Get() ? Copy.CachedSourceStructSubProperty.Get() : Copy.CachedSourceProperty.Get();
    const FProperty* DestProperty = Copy.DestProperty.Get();
    void* Dest = GetDestAddress(Copy, Instance, NodeProperty);
    const void* Source = GetSourceAddress(Copy, Instance);
    if (Copy.PostCopyOperation == EPostCopyOperation::LogicalNegateBool) {
        const bool bValue = static_cast<const FBoolProperty*>(SourceProperty)->GetPropertyValue(Source);
        const auto* DestArray = CastField<FArrayProperty>(DestProperty);
        static_cast<const FBoolProperty*>(DestArray ? DestArray->Inner : DestProperty)->SetPropertyValue(Dest, !bValue);
        return;
    }
    if (const auto* DestBool = CastField<FBoolProperty>(DestProperty))
        DestBool->SetPropertyValue(Dest, static_cast<const FBoolProperty*>(SourceProperty)->GetPropertyValue(Source));
    else if (const auto* DestStruct = CastField<FStructProperty>(DestProperty))
        DestStruct->Struct->CopyScriptStruct(Dest, Source);
    else if (const auto* DestObject = CastField<FObjectPropertyBase>(DestProperty))
        DestObject->SetObjectPropertyValue(Dest, static_cast<const FObjectPropertyBase*>(SourceProperty)->GetObjectPropertyValue(Source));
    else
        std::memcpy(Dest, Source, static_cast<std::size_t>(Copy.Size));
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

//! UE 4.21 and older resolve the copy records for each anim instance; UE 4.22 to 4.25 resolve them once for the class.
void FExposedValueHandler::InitializeDirectCopies(const UClass* InClass, const FStructProperty* InNodeProperty) {
    Function = FindBoundFunction(InClass, BoundFunction);
    NodeProperty = InNodeProperty;
    for (FExposedValueCopyRecord& Copy : CopyRecords) {
        FProperty* SourceProperty = InClass->FindPropertyByName(Copy.SourcePropertyName);
        UE5PARSE_CHECK(SourceProperty != nullptr);
        Copy.CachedSourceProperty = SourceProperty;
        Copy.Size = SourceProperty->GetSize();
        if (Copy.SourceSubPropertyName.isNone())
            continue;
        const auto* SourceStruct = CastField<FStructProperty>(SourceProperty);
        UE5PARSE_CHECK(SourceStruct != nullptr);
        FProperty* SubProperty = SourceStruct->Struct->FindPropertyByName(Copy.SourceSubPropertyName);
        UE5PARSE_CHECK(SubProperty != nullptr);
        Copy.CachedSourceStructSubProperty = SubProperty;
        Copy.Size = SubProperty->GetSize();
    }
}

void FExposedValueHandler::Execute(const FAnimationBaseContext& Context) const {
    if (Handler != nullptr) {
        Handler->Execute(Context);
        return;
    }
    if (Function != nullptr)
        Context.GetAnimInstanceObject()->ProcessEvent(Function, nullptr);
    if (NodeProperty == nullptr) {
        ProcessCopyRecords(Context, PropertyAccessLibrary, CopyRecords);
        return;
    }
    for (const FExposedValueCopyRecord& Copy : CopyRecords)
        ProcessDirectCopy(Copy, Context.GetAnimInstanceObject(), NodeProperty);
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
