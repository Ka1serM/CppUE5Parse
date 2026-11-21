#include "Engine/Source/Runtime/LevelSequence/LevelSequenceBindingReference.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/Engine/UniversalObjectLocators/ActorLocatorFragment.h"
#include "Engine/Source/Runtime/Engine/UniversalObjectLocators/AnimInstanceLocatorFragment.h"
#include "Engine/Source/Runtime/UniversalObjectLocator/SubObjectLocator.h"

#include <algorithm>

namespace ue5parse::runtime {

namespace {

template <class TPayload>
FUniversalObjectLocator MakeLocator(const char* FragmentTypeID, TPayload&& Payload) {
    FUniversalObjectLocator Locator;
    FUniversalObjectLocatorFragment& Fragment = Locator.Fragments.Emplace_GetRef();
    Fragment.FragmentTypeID = FName(FragmentTypeID);
    Fragment.Payload.InitializeAs(std::decay_t<TPayload>::StaticStruct());
    *reinterpret_cast<std::decay_t<TPayload>*>(Fragment.Payload.GetMutableMemory()) = std::forward<TPayload>(Payload);
    return Locator;
}

void AddBinding(TArray<FMovieSceneBindingReference>& SortedReferences, const FGuid& ObjectId, FUniversalObjectLocator&& Locator) {
    const auto Upper = std::upper_bound(SortedReferences.begin(), SortedReferences.end(), ObjectId,
                                        [](const FGuid& Id, const FMovieSceneBindingReference& Reference) { return Id < Reference.ID; });
    FMovieSceneBindingReference Reference;
    Reference.ID = ObjectId;
    Reference.Locator = std::move(Locator);
    SortedReferences.Insert(Reference, static_cast<int32>(Upper - SortedReferences.begin()));
}

}

bool FUpgradedLevelSequenceBindingReferences::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("StructProperty") || Tag.TypeName.GetParameterName(0) != FName("LevelSequenceBindingReferences"))
        return false;
    FLevelSequenceBindingReferences Legacy;
    FLevelSequenceBindingReferences::StaticStruct()->SerializeItem(Ar, &Legacy);
    for (const auto& Pair : Legacy.BindingIdToReferences) {
        for (const FLevelSequenceBindingReference& LegacyReference : Pair.Value.References) {
            if (LegacyReference.ExternalObjectPath.IsNull())
                AddBinding(SortedReferences, Pair.Key, MakeLocator("subobj", FSubObjectLocator{LegacyReference.ObjectPath}));
            else
                AddBinding(SortedReferences, Pair.Key, MakeLocator("actor", FActorLocatorFragment{LegacyReference.ExternalObjectPath}));
        }
    }
    for (const FGuid& AnimInstance : Legacy.AnimSequenceInstances)
        AddBinding(SortedReferences, AnimInstance, MakeLocator("animinst", FAnimInstanceLocatorFragment{EAnimInstanceLocatorFragmentType::AnimInstance}));
    for (const FGuid& AnimInstance : Legacy.PostProcessInstances)
        AddBinding(SortedReferences, AnimInstance, MakeLocator("animinst", FAnimInstanceLocatorFragment{EAnimInstanceLocatorFragmentType::PostProcessAnimInstance}));
    return true;
}

}
