#include "ue5parse/runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include <algorithm>
#include <cstring>

#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/Engine/Components/ActorComponent.h"
#include "ue5parse/runtime/Engine/Components/TimelineComponent.h"
#include "ue5parse/runtime/Engine/Engine/DynamicBlueprintBinding.h"
#include "ue5parse/runtime/Engine/Engine/TimelineTemplate.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's BlueprintGeneratedClass.cpp (UE 5.3).

namespace ue5parse::runtime {

void UBlueprintGeneratedClass::InitPropertiesFromCustomList(uint8* DataPtr, const uint8* DefaultDataPtr) {
    for (FProperty* Property = PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext) {
        if (!Property->HasAnyFlags(RF_MarkAsNative)) continue;
        // A transient property that references instanced subobjects does not take the defaults' value.
        const bool bIsTransient = Property->HasAnyPropertyFlags(CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient);
        if (bIsTransient && Property->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference)) continue;
        Property->CopyCompleteValue_InContainer(DataPtr, DefaultDataPtr);
    }
}

void UBlueprintGeneratedClass::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    // CookedEditorTags (FEditorTags, a map of name to string): unused at run time.
    const int32 TagCount = Ar.ReadCount(12);
    for (int32 i = 0; i < TagCount && !Ar.IsError(); i++) {
        FName Key;
        FString Value;
        Ar << Key << Value;
    }
}

void UBlueprintGeneratedClass::Link() {
    Super::Link();
    UberGraphFramePointerProperty = nullptr;
    for (FField* Field = ChildProperties; Field != nullptr; Field = Field->Next) {
        if (Field->GetFName() != GetUberGraphFrameName()) continue;
        UberGraphFramePointerProperty = CastField<FStructProperty>(Field);
        break;
    }
}

void UBlueprintGeneratedClass::SerializeDefaultObject(UObject* Object, FArchive& Ar) {
    Super::SerializeDefaultObject(Object, Ar);
    if (Object == ClassDefaultObject) CreatePersistentUberGraphFrame(Object, true);
}

FName UBlueprintGeneratedClass::GetUberGraphFrameName() {
    static const FName UberGraphFrameName("UberGraphFrame");
    return UberGraphFrameName;
}

uint8* UBlueprintGeneratedClass::GetPersistentUberGraphFrame(UObject* Obj, UFunction* FuncToCheck) const {
    if (Obj != nullptr && UberGraphFramePointerProperty != nullptr && UberGraphFunction != nullptr && UberGraphFunction == FuncToCheck)
        return UberGraphFramePointerProperty->ContainerPtrToValuePtr<FPointerToUberGraphFrame>(Obj)->RawPointer;
    return GetSuperClass()->GetPersistentUberGraphFrame(Obj, FuncToCheck);
}

void UBlueprintGeneratedClass::CreatePersistentUberGraphFrame(UObject* Obj, bool bCreateOnlyIfEmpty, bool bSkipSuperClass, UClass* OldClass) const {
    if (Obj != nullptr && UberGraphFramePointerProperty != nullptr && UberGraphFunction != nullptr) {
        FPointerToUberGraphFrame* PointerToUberGraphFrame = UberGraphFramePointerProperty->ContainerPtrToValuePtr<FPointerToUberGraphFrame>(Obj);
        if (!bCreateOnlyIfEmpty && PointerToUberGraphFrame->RawPointer != nullptr) {
            RuntimeFree(PointerToUberGraphFrame->RawPointer);
            PointerToUberGraphFrame->RawPointer = nullptr;
        }
        if (PointerToUberGraphFrame->RawPointer == nullptr) {
            const std::size_t Size = static_cast<std::size_t>(UberGraphFunction->GetStructureSize());
            uint8* FrameMemory = static_cast<uint8*>(RuntimeMalloc(Size, static_cast<std::size_t>(std::max(UberGraphFunction->GetMinAlignment(), 16))));
            std::memset(FrameMemory, 0, Size);
            for (FProperty* Property = UberGraphFunction->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
                Property->InitializeValue_InContainer(FrameMemory);
            PointerToUberGraphFrame->RawPointer = FrameMemory;
        }
    }
    if (!bSkipSuperClass) GetSuperClass()->CreatePersistentUberGraphFrame(Obj, bCreateOnlyIfEmpty);
}

void UBlueprintGeneratedClass::DestroyPersistentUberGraphFrame(UObject* Obj, bool bSkipSuperClass) const {
    if (Obj != nullptr && UberGraphFramePointerProperty != nullptr && UberGraphFunction != nullptr) {
        FPointerToUberGraphFrame* PointerToUberGraphFrame = UberGraphFramePointerProperty->ContainerPtrToValuePtr<FPointerToUberGraphFrame>(Obj);
        uint8* FrameMemory = PointerToUberGraphFrame->RawPointer;
        PointerToUberGraphFrame->RawPointer = nullptr;
        if (FrameMemory != nullptr) {
            for (FProperty* Property = UberGraphFunction->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
                Property->DestroyValue_InContainer(FrameMemory);
            RuntimeFree(FrameMemory);
        }
    }
    if (!bSkipSuperClass) GetSuperClass()->DestroyPersistentUberGraphFrame(Obj);
}

// UE's UObject::PostInitProperties.
void UBlueprintGeneratedClass::PostInitInstance(UObject* Obj) const { CreatePersistentUberGraphFrame(Obj, true); }

bool UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(const UClass* InClass, TArray<const UBlueprintGeneratedClass*>& OutBPGClasses) {
    OutBPGClasses.Empty();
    bool bNoErrors = true;
    while (const UBlueprintGeneratedClass* BPGClass = Cast<UBlueprintGeneratedClass>(InClass)) {
        OutBPGClasses.Add(BPGClass);
        InClass = BPGClass->GetSuperClass();
    }
    // A chain that ends before a native class (a missing super) is incomplete.
    if (InClass == nullptr) bNoErrors = false;
    return bNoErrors;
}

UActorComponent* UBlueprintGeneratedClass::FindComponentTemplateByName(FName TemplateName) const {
    for (UActorComponent* Template : ComponentTemplates)
        if (Template != nullptr && Template->GetFName() == TemplateName) return Template;
    return nullptr;
}

void UBlueprintGeneratedClass::CreateComponentsForActor(const UClass* ThisClass, AActor* Actor) {
    if (Actor->IsTemplate() || !IsValid(Actor)) return;
    const UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(ThisClass);
    if (BPGC == nullptr) return;
    for (UTimelineTemplate* TimelineTemplate : BPGC->Timelines)
        if (TimelineTemplate != nullptr) UTimelineComponent::CreateForActor(Actor, TimelineTemplate);
}

void UBlueprintGeneratedClass::BindDynamicDelegates(const UClass* ThisClass, UObject* InInstance) {
    TArray<const UBlueprintGeneratedClass*> Hierarchy;
    GetGeneratedClassesHierarchy(ThisClass, Hierarchy);
    for (const UBlueprintGeneratedClass* BPGC : Hierarchy)
        for (const UDynamicBlueprintBinding* Binding : BPGC->DynamicBindingObjects)
            if (Binding != nullptr) Binding->BindDynamicDelegates(InInstance);
}

}
