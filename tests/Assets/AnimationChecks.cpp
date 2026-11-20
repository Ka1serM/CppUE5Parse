#include <cstdlib>
#include <map>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimMontage.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Engine/Source/Runtime/Engine/Animation/Skeleton.h"
#include "Engine/Source/Runtime/Engine/AnimationRuntime.h"

namespace test::assets {

namespace {

void checkAnimCurves(const std::string& path, const Json& value) {
    rt::UAnimSequence* sequence = loadObject<rt::UAnimSequence>(path);
    if (sequence == nullptr)
        return;
    for (const auto& [key, expected] : value.getObject()) {
        auto [curveName, time] = splitAt(key);
        checkNear(sequence->EvaluateCurveData(rt::FName(std::string(curveName)), static_cast<float>(time)), expected.getNumber(), valueTolerance,
                  describe(path, key));
    }
}

void checkAnimLength(const std::string& path, const Json& value) {
    if (rt::UAnimSequenceBase* sequence = loadObject<rt::UAnimSequenceBase>(path))
        checkNear(sequence->GetPlayLength(), value.getNumber(), valueTolerance, describe(path, "length"));
}

//! The editor reference retrieves an additive sequence as a full pose: its base pose with the additive applied.
void evaluatePose(const rt::UAnimSequence& sequence, double time, rt::FCompactPose& pose) {
    rt::FBlendedCurve curve;
    rt::FAnimationPoseData poseData(pose, curve);
    rt::FAnimExtractContext context(time);
    if (!sequence.IsValidAdditive()) {
        sequence.GetAnimationPose(poseData, context);
        return;
    }
    rt::FCompactPose additivePose;
    additivePose.SetBoneContainer(&pose.GetBoneContainer());
    rt::FBlendedCurve additiveCurve;
    rt::FAnimationPoseData additivePoseData(additivePose, additiveCurve);
    sequence.GetAnimationPose(additivePoseData, context);
    sequence.GetAdditiveBasePose(poseData, context);
    rt::FAnimationRuntime::AccumulateAdditivePose(poseData, additivePoseData, 1.f, sequence.GetAdditiveAnimType());
}

bool isVirtualBone(const rt::USkeleton& skeleton, rt::FName boneName) {
    for (const rt::FVirtualBone& bone : skeleton.GetVirtualBones())
        if (bone.VirtualBoneName == boneName)
            return true;
    return false;
}

void checkAnimPose(const std::string& path, const Json& value) {
    rt::UAnimSequence* sequence = loadObject<rt::UAnimSequence>(path);
    if (sequence == nullptr || !CHECK(sequence->GetSkeleton() != nullptr))
        return;
    const rt::FReferenceSkeleton& skeleton = sequence->GetSkeleton()->GetReferenceSkeleton();
    rt::TArray<rt::FBoneIndexType> requiredBones;
    for (rt::int32 bone = 0; bone < skeleton.GetNum(); bone++)
        requiredBones.Add(static_cast<rt::FBoneIndexType>(bone));
    rt::FBoneContainer boneContainer(requiredBones, *sequence->GetSkeleton());
    std::map<double, rt::FCompactPose> poses;
    for (const auto& [key, transform] : value.getObject()) {
        auto [boneName, time] = splitAt(key);
        auto [pose, added] = poses.try_emplace(time);
        if (added) {
            pose->second.SetBoneContainer(&boneContainer);
            evaluatePose(*sequence, time, pose->second);
        }
        rt::FName name{std::string(boneName)};
        rt::int32 bone = skeleton.FindBoneIndex(name);
        if (bone == rt::INDEX_NONE)
            fail(describe(path, boneName), "no such bone");
        //! The editor reference reads the raw tracks, and a virtual bone has none, so the reference holds its reference pose.
        else if (isVirtualBone(*sequence->GetSkeleton(), name))
            checkTransform(skeleton.GetRefBonePose()[bone], transform, describe(path, key));
        else
            checkTransform(pose->second[rt::FCompactPoseBoneIndex(bone)], transform, describe(path, key));
    }
}

void checkMontageSections(const std::string& path, const Json& value) {
    rt::UAnimMontage* montage = loadObject<rt::UAnimMontage>(path);
    if (montage == nullptr || !CHECK(montage->CompositeSections.Num() == static_cast<rt::int32>(value.getObject().size())))
        return;
    for (const auto& [index, name] : value.getObject())
        checkText(montage->CompositeSections[std::atoi(index.c_str())].SectionName.toString(), name.getString(), describe(path, index));
}

void checkSkeletonBones(const std::string& path, const Json& value) {
    rt::USkeleton* skeleton = loadObject<rt::USkeleton>(path);
    if (skeleton == nullptr)
        return;
    const rt::FReferenceSkeleton& reference = skeleton->GetReferenceSkeleton();
    for (const auto& [name, transform] : value.getObject()) {
        rt::int32 bone = reference.FindBoneIndex(rt::FName(name));
        if (bone == rt::INDEX_NONE)
            fail(describe(path, name), "no such bone");
        else
            checkTransform(reference.GetRefBonePose()[bone], transform, describe(path, name));
    }
}

}

void addAnimationChecks(Checks& checks) {
    checks.values.emplace("anim.curve", checkAnimCurves);
    checks.values.emplace("anim.length", checkAnimLength);
    checks.values.emplace("anim.pose", checkAnimPose);
    checks.values.emplace("montage.length", checkAnimLength);
    checks.values.emplace("montage.section", checkMontageSections);
    checks.values.emplace("skeleton.bone", checkSkeletonBones);
}

}
