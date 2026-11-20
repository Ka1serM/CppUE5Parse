#include <span>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimInstance.h"
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

int countMovedBones(std::span<const rt::FTransform> pose, std::span<const rt::FTransform> reference) {
    int moved = 0;
    for (std::size_t bone = 0; bone < reference.size(); bone++)
        if (!pose[bone].Equals(reference[bone], poseTolerance))
            moved++;
    return moved;
}

const rt::FNumericProperty* findNumber(const rt::UObject* object, const char* name) {
    return rt::CastField<rt::FNumericProperty>(object->GetClass()->FindPropertyByName(rt::FName(name)));
}

//! ABP_Ref adds the delta time to Speed and sets bMoving to Speed > 100; its transition reads bMoving and its blend space reads Speed.
std::optional<std::vector<rt::FTransform>> tickAnimBlueprint(const std::string& classPath, rt::UAnimBlueprintGeneratedClass* animClass, rt::USkeletalMesh* mesh,
                                                             double startSpeed, bool runsEventGraph) {
    auto* component = rt::NewObject<rt::USkeletalMeshComponent>(nullptr);
    component->SkinnedAsset = mesh;
    component->AnimClass = animClass;
    component->RegisterComponentWithWorld(rt::UWorld::CreateWorld(rt::EWorldType::Game));
    rt::UAnimInstance* instance = component->GetAnimInstance();
    if (!CHECK(instance != nullptr))
        return std::nullopt;
    const rt::FNumericProperty* speed = findNumber(instance, "Speed");
    const auto* moving = rt::CastField<rt::FBoolProperty>(instance->GetClass()->FindPropertyByName(rt::FName("bMoving")));
    if (!CHECK(speed != nullptr && speed->IsFloatingPoint() && moving != nullptr))
        return std::nullopt;
    speed->SetFloatingPointPropertyValue(speed->ContainerPtrToValuePtr<void>(instance), startSpeed);
    for (int tick = 0; tick < tickCount; tick++) {
        component->TickAnimation(tickSeconds);
        component->RefreshBoneTransforms();
        if (containsNaN(component->GetComponentSpaceTransforms())) {
            fail(classPath, "gives a pose with NaN at tick " + std::to_string(tick));
            return std::nullopt;
        }
    }
    const double endSpeed = speed->GetFloatingPointPropertyValue(speed->ContainerPtrToValuePtr<void>(instance));
    checkNear(endSpeed, runsEventGraph ? startSpeed + tickCount * static_cast<double>(tickSeconds) : startSpeed, 1e-5, describe(classPath, "Speed"));
    if (moving->GetPropertyValue_InContainer(instance) != (runsEventGraph && endSpeed > 100.0))
        fail(classPath, "bMoving does not follow Speed");
    const rt::TArray<rt::FTransform>& pose = component->GetBoneSpaceTransforms();
    return std::vector<rt::FTransform>(pose.GetData(), pose.GetData() + pose.Num());
}

void runAnimBlueprint(const std::string& blueprintPath, const std::vector<std::string>& meshPaths, bool runsEventGraph) {
    std::string classPath = blueprintPath + "_C";
    auto* animClass = loadObject<rt::UAnimBlueprintGeneratedClass>(classPath);
    if (animClass == nullptr)
        return;
    rt::USkeletalMesh* mesh = findMesh(meshPaths, animClass->GetTargetSkeleton());
    if (mesh == nullptr) {
        fail(classPath, "no skeletal mesh of the cook uses the target skeleton");
        return;
    }
    const rt::TArray<rt::FTransform>& referencePose = mesh->GetRefSkeleton().GetRefBonePose();
    const std::span<const rt::FTransform> reference(referencePose.GetData(), static_cast<std::size_t>(referencePose.Num()));
    std::optional<std::vector<rt::FTransform>> idle = tickAnimBlueprint(classPath, animClass, mesh, 0.0, runsEventGraph);
    if (!idle || !CHECK(idle->size() == reference.size()))
        return;
    if (countMovedBones(*idle, reference) == 0)
        fail(classPath, "keeps the reference pose");
    if (!runsEventGraph)
        return;
    std::optional<std::vector<rt::FTransform>> moving = tickAnimBlueprint(classPath, animClass, mesh, 600.0, runsEventGraph);
    if (!moving || !CHECK(moving->size() == idle->size()))
        return;
    if (countMovedBones(*moving, *idle) == 0)
        fail(classPath, "gives the idle pose when Speed starts at 600");
}

//! UE 4.4 to 4.6 compile the unconnected update event of a new child anim blueprint into an empty override of the event graph of its parent.
bool runsParentEventGraph(const Cook& cook, const std::string& blueprintPath) {
    return !blueprintPath.ends_with("_Child") || cook.game < ue5parse::EGame::GAME_UE4_4 || cook.game >= ue5parse::EGame::GAME_UE4_7;
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
            runAnimBlueprint(blueprint, meshes, runsParentEventGraph(cook, blueprint));
    });
}

}

void addAnimBlueprintChecks(Checks& checks) {
    checks.cook.emplace("animbp", checkAnimBlueprints);
}

}
