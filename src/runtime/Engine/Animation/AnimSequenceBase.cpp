#include "ue5parse/runtime/Engine/Animation/AnimSequenceBase.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifyQueue.h"
#include "ue5parse/runtime/Engine/Animation/AnimationPoseData.h"

namespace ue5parse::runtime {

void UAnimSequenceBase::GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    ReportLoadProblem("anim.asset_unsupported", GetClass()->GetName() + FString(": ") + GetName());
    OutAnimationPoseData.GetPose().ResetToRefPose();
}

void UAnimSequenceBase::EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime) const {
    for (const FFloatCurve& Curve : RawCurveData.FloatCurves) OutCurve.Set(Curve.GetName(), Curve.Evaluate(CurrentTime));
}

// UE's UAnimSequenceBase::TickAssetPlayer with length-based sync: a leader advances by its play rate, a follower moves
// to the leader's position ratio.
void UAnimSequenceBase::TickAssetPlayer(FAnimTickRecord& Instance, FAnimNotifyQueue& NotifyQueue, FAnimAssetTickContext& Context) const {
    float CurrentTime = *Instance.TimeAccumulator;
    if (Context.ShouldResyncToSyncGroup() && !Instance.bIsEvaluator) CurrentTime = Context.GetAnimationPositionRatio() * GetPlayLength();

    float PreviousTime = CurrentTime;
    float DeltaTime = 0;
    const float PlayRate = Instance.PlayRateMultiplier * RateScale;
    if (Context.IsLeader()) {
        DeltaTime = PlayRate * Context.GetDeltaTime();
        Context.SetLeaderDelta(DeltaTime);
        Context.SetPreviousAnimationPositionRatio(PreviousTime / GetPlayLength());
        if (DeltaTime != 0.0f) FAnimationRuntime::AdvanceTime(Instance.bLooping, DeltaTime, CurrentTime, GetPlayLength());
        Context.SetAnimationPositionRatio(CurrentTime / GetPlayLength());
    } else {
        PreviousTime = Context.GetPreviousAnimationPositionRatio() * GetPlayLength();
        CurrentTime = Context.GetAnimationPositionRatio() * GetPlayLength();
        if (CurrentTime != PreviousTime) {
            DeltaTime = CurrentTime - PreviousTime;
            // A move against the play rate went around the end of the asset.
            if (DeltaTime * PlayRate < 0.0f) DeltaTime += FMath::Sign(PlayRate) * GetPlayLength();
        }
    }
    *Instance.TimeAccumulator = CurrentTime;
    Instance.DeltaTimeRecord->Set(PreviousTime, DeltaTime);
    // UE's HandleAssetPlayerTickedInternal.
    FAnimNotifyContext NotifyContext(Instance);
    GetAnimNotifies(PreviousTime, DeltaTime, NotifyContext);
    NotifyQueue.AddAnimNotifies(Context.ShouldGenerateNotifies(), NotifyContext.ActiveNotifies, Instance.EffectiveBlendWeight);
}

void UAnimSequenceBase::GetAnimNotifies(const float& StartTime, const float& DeltaTime, FAnimNotifyContext& NotifyContext) const {
    if (!IsNotifyAvailable()) return;
    const bool bPlayingBackwards = DeltaTime < 0.f;
    float PreviousPosition = StartTime;
    float CurrentPosition = StartTime;
    float DesiredDeltaMove = DeltaTime;
    while (true) {
        // Without looping, so that a loop collects the notifies on both sides of the end.
        const ETypeAdvanceAnim AdvanceType = FAnimationRuntime::AdvanceTime(false, DesiredDeltaMove, CurrentPosition, GetPlayLength());
        GetAnimNotifiesFromDeltaPositions(PreviousPosition, CurrentPosition, NotifyContext);
        if (AdvanceType != ETAA_Finished || NotifyContext.TickRecord == nullptr || !NotifyContext.TickRecord->bLooping) break;
        const float ActualDeltaMove = CurrentPosition - PreviousPosition;
        DesiredDeltaMove -= ActualDeltaMove;
        PreviousPosition = bPlayingBackwards ? GetPlayLength() : 0.f;
        CurrentPosition = PreviousPosition;
    }
}

void UAnimSequenceBase::GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float& CurrentPosition,
                                                          FAnimNotifyContext& NotifyContext) const {
    if (Notifies.Num() == 0) return;
    const bool bPlayingBackwards = CurrentPosition < PreviousPosition;
    for (const FAnimNotifyEvent& AnimNotifyEvent : Notifies) {
        const float NotifyStartTime = AnimNotifyEvent.GetTriggerTime();
        const float NotifyEndTime = AnimNotifyEvent.GetEndTriggerTime();
        const bool bPassed = bPlayingBackwards ? NotifyStartTime < PreviousPosition && NotifyEndTime >= CurrentPosition
                                               : NotifyStartTime <= CurrentPosition && NotifyEndTime > PreviousPosition;
        if (!bPassed) continue;
        NotifyContext.ActiveNotifies.Add(FAnimNotifyEventReference(&AnimNotifyEvent, this));
        if (NotifyContext.TickRecord) NotifyContext.ActiveNotifies.Last().GatherTickRecordData(*NotifyContext.TickRecord);
    }
}

}
