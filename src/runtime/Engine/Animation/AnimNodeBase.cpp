#include <cstdio>
#include <mutex>
#include <set>
#include <string>
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

const UAnimBlueprintGeneratedClass* FAnimationBaseContext::GetAnimClass() const { return AnimInstanceProxy->GetAnimClassInterface(); }

UObject* FAnimationBaseContext::GetAnimInstanceObject() const { return AnimInstanceProxy->GetAnimInstanceObject(); }

FPoseContext::FPoseContext(FAnimInstanceProxy* InAnimInstanceProxy, bool bInExpectsAdditivePose)
    : FAnimationBaseContext(InAnimInstanceProxy), bExpectsAdditivePose(bInExpectsAdditivePose) {
    Pose.SetBoneContainer(&InAnimInstanceProxy->GetRequiredBones());
}

FPoseContext::FPoseContext(const FPoseContext& SourceContext, bool bOverrideAdditive)
    : FPoseContext(SourceContext.AnimInstanceProxy, SourceContext.bExpectsAdditivePose || bOverrideAdditive) {}

FComponentSpacePoseContext::FComponentSpacePoseContext(FAnimInstanceProxy* InAnimInstanceProxy) : FAnimationBaseContext(InAnimInstanceProxy) {
    Pose.InitPose(&InAnimInstanceProxy->GetRequiredBones());
}

void FComponentSpacePoseContext::ResetToRefPose() {
    Pose.InitPose(&AnimInstanceProxy->GetRequiredBones());
    Curve.Empty();
}

void FPoseLinkBase::AttemptRelink(const FAnimationBaseContext& Context) {
    if (LinkedNode != nullptr || LinkID == INDEX_NONE) return;
    const UAnimBlueprintGeneratedClass* AnimClass = Context.GetAnimClass();
    const int32 Target = AnimClass->GetLinkTarget(LinkID);
    if (Target == INDEX_NONE) return;
    LinkedNode = AnimClass->GetAnimNodeProperties()[Target]->ContainerPtrToValuePtr<FAnimNode_Base>(Context.GetAnimInstanceObject());
}

void FPoseLinkBase::Initialize(const FAnimationInitializeContext& Context) {
    AttemptRelink(Context);
    if (LinkedNode != nullptr) LinkedNode->Initialize_AnyThread(Context);
}

void FPoseLinkBase::CacheBones(const FAnimationCacheBonesContext& Context) {
    if (LinkedNode != nullptr) LinkedNode->CacheBones_AnyThread(Context);
}

void FPoseLinkBase::Update(const FAnimationUpdateContext& Context) {
    if (LinkedNode != nullptr) LinkedNode->Update_AnyThread(Context);
}

void FPoseLinkBase::SetDynamicLinkNode(FPoseLinkBase* InPoseLink) {
    LinkedNode = InPoseLink ? InPoseLink->LinkedNode : nullptr;
    LinkID = InPoseLink ? InPoseLink->LinkID : INDEX_NONE;
}

void FPoseLink::Evaluate(FPoseContext& Output) {
    if (LinkedNode == nullptr) {
        Output.ResetToRefPose();
        return;
    }
    LinkedNode->Evaluate_AnyThread(Output);
    // DEBUG-SCALE
    for (const FTransform& Bone : Output.Pose.GetBones()) {
        if (Bone.GetScale3D().X < 1.5) continue;
        const UAnimBlueprintGeneratedClass* AnimClass = Output.GetAnimClass();
        std::string Where = AnimClass ? AnimClass->GetName().ToUtf8() : std::string("?");
        if (AnimClass && LinkID != INDEX_NONE && AnimClass->GetLinkTarget(LinkID) != INDEX_NONE)
            Where += " " + AnimClass->GetAnimNodeProperties()[AnimClass->GetLinkTarget(LinkID)]->GetName().ToUtf8();
        static std::mutex Mutex;
        static std::set<std::string> Seen;
        const std::lock_guard Lock(Mutex);
        if (Seen.insert(Where).second) std::fprintf(stderr, "SCALE2 %s\n", Where.c_str());
        break;
    }
}

void FComponentSpacePoseLink::EvaluateComponentSpace(FComponentSpacePoseContext& Output) {
    if (LinkedNode == nullptr) {
        Output.ResetToRefPose();
        return;
    }
    LinkedNode->EvaluateComponentSpace_AnyThread(Output);
}

void FAnimNode_Base::Evaluate_AnyThread(FPoseContext& Output) { Output.ResetToRefPose(); }

void FAnimNode_Base::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) { Output.ResetToRefPose(); }

const FExposedValueHandler& FAnimNode_Base::GetEvaluateGraphExposedInputs() const {
    static const FExposedValueHandler Default;
    if (NodeData == nullptr) return Default;
    const FExposedValueHandler* Handler = NodeData->GetAnimClass().GetExposedValueHandler(NodeData->GetNodeIndex());
    return Handler ? *Handler : Default;
}

namespace UE::Anim {

FNodeDataId::FNodeDataId(FName InPropertyName, const FAnimNode_Base* InNode, const UScriptStruct* InNodeStruct)
    : Index(InNode->GetNodeData()->GetAnimClass().GetAnimNodePropertyIndex(InNodeStruct, InPropertyName)) {
    UE5PARSE_CHECK(Index != INDEX_NONE);
}

}

const UAnimBlueprintGeneratedClass& FAnimNodeData::GetAnimClass() const {
    return *static_cast<const UAnimBlueprintGeneratedClass*>(AnimClassInterface.GetObject());
}

const void* FAnimNodeData::GetData(UE::Anim::FNodeDataId InId, const FAnimNode_Base* InNode) const {
    const uint32 Entry = Entries[InId.GetIndex()];
    const int32 EntryIndex = static_cast<int32>(Entry & ~InstanceDataFlag);
    if ((Entry & InstanceDataFlag) == 0) return GetAnimClass().GetConstantNodeValueRaw(EntryIndex);
    return GetAnimClass().GetMutableNodeValueRaw(EntryIndex, GetAnimClass().GetObjectPtrFromAnimNode(InNode));
}

const void* FAnimNodeData::GetInstanceData(UE::Anim::FNodeDataId InId, const FAnimNode_Base* InNode) const {
    const uint32 Entry = Entries[InId.GetIndex()];
    if ((Entry & InstanceDataFlag) == 0) return nullptr;
    return GetAnimClass().GetMutableNodeValueRaw(static_cast<int32>(Entry & ~InstanceDataFlag), GetAnimClass().GetObjectPtrFromAnimNode(InNode));
}

}
