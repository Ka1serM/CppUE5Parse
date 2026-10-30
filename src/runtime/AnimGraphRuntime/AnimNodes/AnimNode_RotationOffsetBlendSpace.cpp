#include <string>
#include "ue5parse/runtime/Engine/Animation/PropertyAccess.h"
#include "ue5parse/runtime/Engine/Animation/ExposedValueHandler.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include <cstdio>
#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"
#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_RotationOffsetBlendSpace.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_RotationOffsetBlendSpace::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_BlendSpacePlayer::Initialize_AnyThread(Context);
    BasePose.Initialize(Context);
}

void FAnimNode_RotationOffsetBlendSpace::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    FAnimNode_BlendSpacePlayer::CacheBones_AnyThread(Context);
    BasePose.CacheBones(Context);
}

// The runtime evaluates LOD 0, where every node is enabled.
void FAnimNode_RotationOffsetBlendSpace::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    bIsLODEnabled = true;
    GetEvaluateGraphExposedInputs().Execute(Context);
    switch (AlphaInputType) {
        case EAnimAlphaInputType::Float: ActualAlpha = AlphaScaleBias.ApplyTo(AlphaScaleBiasClamp.ApplyTo(Alpha, Context.GetDeltaTime())); break;
        case EAnimAlphaInputType::Bool: ActualAlpha = AlphaBoolBlend.ApplyTo(bAlphaBoolEnabled, Context.GetDeltaTime()); break;
        case EAnimAlphaInputType::Curve:
            ActualAlpha = AlphaScaleBiasClamp.ApplyTo(Context.AnimInstanceProxy->GetAnimInstance()->GetCurveValue(AlphaCurveName), Context.GetDeltaTime());
            break;
    }
    ActualAlpha = FMath::Clamp(ActualAlpha, 0.f, 1.f);
    if (static int PrintedBind = 0; GetBlendSpace() == nullptr && PrintedBind++ < 2) {
        const FExposedValueHandler& H = GetEvaluateGraphExposedInputs();
        std::fprintf(stderr, "AOBIND fn=%s bound=%s copies=%d\n", H.Function ? H.Function->GetName().ToUtf8().c_str() : "-", H.BoundFunction.toString().c_str(), H.CopyRecords.Num());
        const FPropertyAccessLibrary* L = Context.GetAnimClass()->GetPropertyAccessLibrary();
        for (const FExposedValueCopyRecord& C : H.CopyRecords) {
            if (!L) break;
            const FPropertyAccessCopy& Copy = L->CopyBatchArray[0].Copies[C.CopyIndex];
            std::string Src, Dst;
            const FPropertyAccessPath& SP = L->SrcPaths[Copy.AccessIndex];
            for (int32 I = 0; I < SP.PathSegmentCount; ++I) Src += L->PathSegments[SP.PathSegmentStartIndex + I].Name.toString() + ".";
            for (int32 D = Copy.DestAccessStartIndex; D < Copy.DestAccessEndIndex; ++D) {
                const FPropertyAccessPath& DP = L->DestPaths[D];
                for (int32 I = 0; I < DP.PathSegmentCount; ++I) Dst += L->PathSegments[DP.PathSegmentStartIndex + I].Name.toString() + ".";
                Dst += " ";
            }
            std::fprintf(stderr, "   copy %s -> %s resolved=%d\n", Src.c_str(), Dst.c_str(), L->SrcAccesses[Copy.AccessIndex].Property != nullptr);
        }
    }
    if (FAnimWeight::IsRelevant(ActualAlpha)) UpdateInternal(Context);
    BasePose.Update(Context);
}

void FAnimNode_RotationOffsetBlendSpace::Evaluate_AnyThread(FPoseContext& Output) {
    BasePose.Evaluate(Output);
    if (!bIsLODEnabled || !FAnimWeight::IsRelevant(ActualAlpha)) return;
    FPoseContext MeshSpaceRotationAdditivePoseContext(Output);
    FAnimNode_BlendSpacePlayer::Evaluate_AnyThread(MeshSpaceRotationAdditivePoseContext);
    FAnimationPoseData BaseAnimationPoseData = Output.GetAnimationPoseData();
    const FAnimationPoseData AdditiveAnimationPoseData = MeshSpaceRotationAdditivePoseContext.GetAnimationPoseData();
    // DEBUG-AO
    const TArray<FTransform> BaseBones = Output.Pose.GetBones();
    FAnimationRuntime::AccumulateAdditivePose(BaseAnimationPoseData, AdditiveAnimationPoseData, ActualAlpha, AAT_RotationOffsetMeshSpace);
    static int Printed = 0;
    const TArray<FTransform>& Out = Output.Pose.GetBones();
    for (int32 I = 0; I < Out.Num() && Printed < 6; ++I) {
        if (Out[I].GetScale3D().X < 1.5) continue;
        ++Printed;
        const UBlendSpace* BS = GetBlendSpace();
        const FTransform& Add = MeshSpaceRotationAdditivePoseContext.Pose.GetBones()[I];
        std::fprintf(stderr, "AO bone %d bs=%s valid=%d alpha=%f samples=%d base=(%f %f) add=(t %f %f %f s %f) \n", I, BS ? BS->GetPathName().ToUtf8().c_str() : "null",
                     BS ? int(BS->IsValidAdditive()) : -1, ActualAlpha, BlendSampleDataCache.Num(), BaseBones[I].GetScale3D().X, BaseBones[I].GetTranslation().X,
                     Add.GetTranslation().X, Add.GetTranslation().Y, Add.GetTranslation().Z, Add.GetScale3D().X);
        for (const FBlendSampleData& D : BlendSampleDataCache) {
            const FBlendSample* Sm = BS && BS->GetBlendSamples().IsValidIndex(D.SampleDataIndex) ? &BS->GetBlendSamples()[D.SampleDataIndex] : nullptr;
            const UAnimSequenceBase* A = Sm ? Sm->Animation : nullptr;
            std::fprintf(stderr, "   sample %d w=%f anim=%s additive=%d type=%d\n", D.SampleDataIndex, D.TotalWeight, A ? A->GetPathName().ToUtf8().c_str() : "null",
                         A ? int(A->IsValidAdditive()) : -1, A ? int(A->GetAdditiveAnimType()) : -1);
        }
        break;
    }
}

}
