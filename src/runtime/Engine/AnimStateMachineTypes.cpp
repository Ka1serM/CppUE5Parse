#include "ue5parse/runtime/Engine/Animation/AnimStateMachineTypes.h"

namespace ue5parse::runtime {

int32 FBakedAnimationStateMachine::FindStateIndex(const FName Name) const {
    for (int32 Index = 0; Index < States.Num(); ++Index)
        if (States[Index].StateName == Name) return Index;
    return INDEX_NONE;
}

int32 FBakedAnimationStateMachine::FindTransitionIndex(const int32 From, const int32 To) const {
    for (int32 Index = 0; Index < Transitions.Num(); ++Index)
        if (Transitions[Index].PreviousState == From && Transitions[Index].NextState == To) return Index;
    return INDEX_NONE;
}

}
