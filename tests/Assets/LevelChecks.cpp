#include <map>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Engine/Level.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

namespace test::assets {

namespace {

void checkLevelActors(const std::string& path, const Json& value) {
    rt::UWorld* world = loadObject<rt::UWorld>(path);
    if (world == nullptr || !CHECK(world->PersistentLevel != nullptr))
        return;
    std::map<std::string, const rt::AActor*, std::less<>> actors;
    for (const rt::AActor* actor : world->PersistentLevel->Actors)
        if (actor != nullptr)
            actors.emplace(actor->GetName().ToUtf8(), actor);
    for (const auto& [name, expected] : value.getObject()) {
        auto found = actors.find(name);
        if (found == actors.end()) {
            fail(describe(path, name), "no such actor");
            continue;
        }
        const Json::Array& classAndTransform = expected.getArray();
        checkText(getPathName(found->second->GetClass()), classAndTransform[0].getString(), describe(path, name));
        checkTransform(found->second->GetActorTransform(), classAndTransform[1], describe(path, name));
    }
}

}

void addLevelChecks(Checks& checks) {
    checks.values.emplace("level.actor", checkLevelActors);
}

}
