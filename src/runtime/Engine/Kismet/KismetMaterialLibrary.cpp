#include "ue5parse/runtime/Engine/Kismet/KismetMaterialLibrary.h"

#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInstanceDynamic.h"
#include "ue5parse/runtime/Engine/Materials/MaterialParameterCollection.h"

// UE's Kismet/KismetMaterialLibrary.cpp (UE 5.3).

namespace ue5parse::runtime {

namespace {

UMaterialParameterCollectionInstance* CollectionInstance(const UObject* WorldContextObject, const UMaterialParameterCollection* Collection) {
    UWorld* World = Collection != nullptr && WorldContextObject != nullptr ? WorldContextObject->GetWorld() : nullptr;
    return World != nullptr ? World->GetParameterCollectionInstance(Collection) : nullptr;
}

// UE logs a missing parameter to the PIE message log once for each instance.
void ReportMissingParameter(UMaterialParameterCollectionInstance& Instance, const FName ParameterName) {
    if (Instance.bLoggedMissingParameterWarning)
        return;
    Instance.bLoggedMissingParameterWarning = true;
    const FString Message = FString("Unable to find parameter ") + NameToString(ParameterName) + FString(" in material parameter collection ") +
                            Instance.Collection->GetPathName();
    if (FFrame* Frame = FFrame::GetThreadLocalTopStackFrame())
        Frame->KismetExecutionMessage("MissingCollectionParameter", Message.ToUtf8(), EBlueprintExceptionType::NonFatalError);
}

}

UMaterialInstanceDynamic* UKismetMaterialLibrary::CreateDynamicMaterialInstance(UObject* WorldContextObject, UMaterialInterface* Parent,
                                                                                const FName OptionalName, const EMIDCreationFlags CreationFlags) {
    if (Parent == nullptr)
        return nullptr;
    UObject* Outer = CreationFlags == EMIDCreationFlags::Transient || WorldContextObject == nullptr ? GetTransientPackage() : WorldContextObject;
    return UMaterialInstanceDynamic::Create(Parent, Outer, OptionalName);
}

void UKismetMaterialLibrary::SetScalarParameterValue(UObject* WorldContextObject, UMaterialParameterCollection* Collection, const FName ParameterName,
                                                     const float ParameterValue) {
    UMaterialParameterCollectionInstance* Instance = CollectionInstance(WorldContextObject, Collection);
    if (Instance != nullptr && !Instance->SetScalarParameterValue(ParameterName, ParameterValue))
        ReportMissingParameter(*Instance, ParameterName);
}

void UKismetMaterialLibrary::SetVectorParameterValue(UObject* WorldContextObject, UMaterialParameterCollection* Collection, const FName ParameterName,
                                                     const FLinearColor& ParameterValue) {
    UMaterialParameterCollectionInstance* Instance = CollectionInstance(WorldContextObject, Collection);
    if (Instance != nullptr && !Instance->SetVectorParameterValue(ParameterName, ParameterValue))
        ReportMissingParameter(*Instance, ParameterName);
}

float UKismetMaterialLibrary::GetScalarParameterValue(UObject* WorldContextObject, UMaterialParameterCollection* Collection, const FName ParameterName) {
    float ParameterValue = 0;
    UMaterialParameterCollectionInstance* Instance = CollectionInstance(WorldContextObject, Collection);
    if (Instance != nullptr && !Instance->GetScalarParameterValue(ParameterName, ParameterValue))
        ReportMissingParameter(*Instance, ParameterName);
    return ParameterValue;
}

FLinearColor UKismetMaterialLibrary::GetVectorParameterValue(UObject* WorldContextObject, UMaterialParameterCollection* Collection, const FName ParameterName) {
    FLinearColor ParameterValue(0, 0, 0, 0);
    UMaterialParameterCollectionInstance* Instance = CollectionInstance(WorldContextObject, Collection);
    if (Instance != nullptr && !Instance->GetVectorParameterValue(ParameterName, ParameterValue))
        ReportMissingParameter(*Instance, ParameterName);
    return ParameterValue;
}

}
