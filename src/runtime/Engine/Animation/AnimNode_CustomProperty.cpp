#include "ue5parse/runtime/Engine/Animation/AnimNode_CustomProperty.h"

#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

void FAnimNode_CustomProperty::PropagateInputProperties(const UObject* InSourceInstance) {
    if (TargetInstance == nullptr) return;
    for (int32 PropIdx = 0; PropIdx < SourceProperties.Num(); ++PropIdx) {
        FProperty* CallerProperty = SourceProperties[PropIdx];
        FProperty* SubProperty = DestProperties[PropIdx];
        if (CallerProperty == nullptr || SubProperty == nullptr) continue;
        const uint8* SrcPtr = CallerProperty->ContainerPtrToValuePtr<uint8>(InSourceInstance);
        uint8* DestPtr = SubProperty->ContainerPtrToValuePtr<uint8>(TargetInstance);
        CallerProperty->CopyCompleteValue(DestPtr, SrcPtr);
    }
}

void FAnimNode_CustomProperty::InitializeProperties(const UObject* InSourceInstance, UClass* InTargetClass) {
    if (InTargetClass == nullptr) return;
    const UClass* SourceClass = InSourceInstance->GetClass();
    SourceProperties.Reset(SourcePropertyNames.Num());
    DestProperties.Reset(SourcePropertyNames.Num());
    for (int32 Idx = 0; Idx < SourcePropertyNames.Num() && Idx < DestPropertyNames.Num(); ++Idx) {
        FProperty* DestProperty = InTargetClass->FindPropertyByName(DestPropertyNames[Idx]);
        if (DestProperty == nullptr) continue;
        FProperty* SourceProperty = SourceClass->FindPropertyByName(SourcePropertyNames[Idx]);
        if (SourceProperty == nullptr) continue;
        SourceProperties.Add(SourceProperty);
        DestProperties.Add(DestProperty);
    }
}

}
