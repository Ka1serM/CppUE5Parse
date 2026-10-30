#include "ue5parse/runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "ue5parse/runtime/Core/Serialization/Archive.h"
#include "ue5parse/runtime/Engine/Components/ActorComponent.h"
#include "ue5parse/runtime/Engine/Components/TimelineComponent.h"
#include "ue5parse/runtime/Engine/Engine/DynamicBlueprintBinding.h"
#include "ue5parse/runtime/Engine/Engine/TimelineTemplate.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

// UE's BlueprintGeneratedClass.cpp (UE 5.3).

namespace ue5parse::runtime {

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
