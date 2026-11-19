#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "Engine/Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/SkeletalMesh.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"

namespace test::assets {

namespace {

constexpr int tickCount = 30;
constexpr float tickSeconds = 1.0f / 30.0f;
constexpr double poseTolerance = 1e-3;

std::vector<std::string> findAssets(const Json& assets, std::string_view className) {
    std::vector<std::string> paths;
    for (const auto& [path, assetClass] : assets.getObject())
        if (assetClass.getString() == className)
            paths.push_back(path);
    return paths;
}

rt::USkeletalMesh* findMesh(const std::vector<std::string>& meshPaths, const rt::USkeleton* skeleton) {
    for (const std::string& path : meshPaths)
        if (auto* mesh = loadObject<rt::USkeletalMesh>(path); mesh != nullptr && mesh->GetSkeleton() == skeleton)
            return mesh;
    return nullptr;
}

bool containsNaN(const rt::TArray<rt::FTransform>& transforms) {
    for (const rt::FTransform& transform : transforms)
        if (transform.ContainsNaN())
            return true;
    return false;
}

int countMovedBones(const rt::TArray<rt::FTransform>& pose, const rt::FReferenceSkeleton& skeleton) {
    int moved = 0;
    for (rt::int32 bone = 0; bone < pose.Num(); bone++)
        if (!pose[bone].Equals(skeleton.GetRefBonePose()[bone], poseTolerance))
            moved++;
    return moved;
}

void runAnimBlueprint(const std::string& blueprintPath, const std::vector<std::string>& meshPaths) {
    std::string classPath = blueprintPath + "_C";
    auto* animClass = loadObject<rt::UAnimBlueprintGeneratedClass>(classPath);
    if (animClass == nullptr)
        return;
    rt::USkeletalMesh* mesh = findMesh(meshPaths, animClass->GetTargetSkeleton());
    if (mesh == nullptr) {
        fail(classPath, "no skeletal mesh of the cook uses the target skeleton");
        return;
    }
    auto* component = rt::NewObject<rt::USkeletalMeshComponent>(nullptr);
    component->SkinnedAsset = mesh;
    component->AnimClass = animClass;
    component->RegisterComponentWithWorld(rt::UWorld::CreateWorld(rt::EWorldType::Game));
    if (!CHECK(component->GetAnimInstance() != nullptr))
        return;
    for (int tick = 0; tick < tickCount; tick++) {
        component->TickAnimation(tickSeconds);
        component->RefreshBoneTransforms();
        if (containsNaN(component->GetComponentSpaceTransforms())) {
            fail(classPath, "gives a pose with NaN at tick " + std::to_string(tick));
            return;
        }
    }
    const rt::FReferenceSkeleton& skeleton = mesh->GetRefSkeleton();
    const rt::TArray<rt::FTransform>& pose = component->GetBoneSpaceTransforms();
    if (!CHECK(pose.Num() == skeleton.GetNum()))
        return;
    if (countMovedBones(pose, skeleton) == 0)
        fail(classPath, "keeps the reference pose");
}

void checkAnimBlueprints(const Cook& cook) {
    const Json* assets = cook.expected.find("assets");
    if (assets == nullptr) {
        fail(cook.version, "the reference values have no assets");
        return;
    }
    std::vector<std::string> blueprints = findAssets(*assets, "AnimBlueprint");
    std::vector<std::string> meshes = findAssets(*assets, "SkeletalMesh");
    CHECK(!blueprints.empty());
    withRuntime(cook, [&](const ue5parse::FileProvider&) {
        for (const std::string& blueprint : blueprints)
            runAnimBlueprint(blueprint, meshes);
    });
}

}

void addAnimBlueprintChecks(Checks& checks) {
    checks.cook.emplace("animbp", checkAnimBlueprints);
}

}
