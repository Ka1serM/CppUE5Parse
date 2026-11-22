#include <algorithm>
#include <map>
#include <string_view>
#include <vector>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Engine/Level.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

namespace test::assets {

namespace {

using ActorMap = std::map<std::string, const rt::AActor*, std::less<>>;

std::string_view childActorBase(std::string_view name) {
    const std::size_t cat = name.rfind("_CAT_");
    return cat == std::string_view::npos ? std::string_view() : name.substr(0, cat);
}

int childActorNumber(std::string_view name) {
    return std::stoi(std::string(name.substr(name.rfind('_') + 1)));
}

//! Before 4.23 the cooker respawns child actors under new unique names, so a missing child actor pairs with the unexpected one of the same base in number order.
void pairChildActors(ActorMap& actors, const Json::Object& expected) {
    std::map<std::string_view, std::vector<std::string_view>> missing;
    std::map<std::string_view, std::vector<std::string_view>> unexpected;
    for (const auto& [name, value] : expected)
        if (!childActorBase(name).empty() && !actors.contains(name))
            missing[childActorBase(name)].push_back(name);
    const auto isExpected = [&](std::string_view name) { return std::ranges::find(expected, name, &Json::Object::value_type::first) != expected.end(); };
    for (const auto& [name, actor] : actors)
        if (!childActorBase(name).empty() && !isExpected(name))
            unexpected[childActorBase(name)].push_back(name);
    const auto byNumber = [](std::string_view a, std::string_view b) { return childActorNumber(a) < childActorNumber(b); };
    ActorMap paired;
    for (auto& [base, names] : missing) {
        std::vector<std::string_view>& found = unexpected[base];
        std::ranges::sort(names, byNumber);
        std::ranges::sort(found, byNumber);
        for (std::size_t i = 0; i < names.size() && i < found.size(); i++)
            paired.emplace(std::string(names[i]), actors.find(found[i])->second);
    }
    actors.merge(paired);
}

void checkLevelActors(const std::string& path, const Json& value) {
    rt::UWorld* world = loadObject<rt::UWorld>(path);
    if (world == nullptr || !CHECK(world->PersistentLevel != nullptr))
        return;
    ActorMap actors;
    for (const rt::AActor* actor : world->PersistentLevel->Actors)
        if (actor != nullptr)
            actors.emplace(actor->GetName().ToUtf8(), actor);
    pairChildActors(actors, value.getObject());
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
