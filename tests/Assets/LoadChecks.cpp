#include <cstdio>
#include <string>
#include <vector>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"

namespace test::assets {

namespace {

std::vector<std::string> problems;

void collectProblem(const char* code, const rt::FString& message) {
    if (!isOutsideCook(code, message))
        problems.push_back(std::string(code) + ": " + message.ToUtf8());
}

std::string_view getPackageOf(std::string_view objectPath) { return objectPath.substr(0, objectPath.find('.')); }

//! A cooked blueprint keeps only its generated class: /Game/BP_X.BP_X becomes /Game/BP_X.BP_X_C.
bool isBlueprintClass(std::string_view className) { return className.ends_with("Blueprint"); }

void loadAsset(const std::string& path, std::string_view className) {
    problems.clear();
    std::string objectPath = isBlueprintClass(className) ? path + "_C" : path;
    rt::UObject* object = rt::StaticLoadObject(nullptr, nullptr, rt::FString(objectPath));
    std::string_view package = getPackageOf(path);
    int own = 0;
    for (const std::string& problem : problems) {
        bool mine = problem.find(std::string(package) + ".") != std::string::npos || problem.find(std::string(package) + ":") != std::string::npos;
        own += mine ? 1 : 0;
        std::fprintf(stderr, "  %s %s\n", mine ? "load problem" : "(dependency)", problem.c_str());
    }
    if (object == nullptr) {
        fail(objectPath, "cannot be loaded");
        return;
    }
    if (own > 0)
        fail(objectPath, std::to_string(own) + " load problems");
    std::string loadedClass = object->GetClass()->GetName().ToUtf8();
    if (isBlueprintClass(className)) {
        if (!loadedClass.ends_with("GeneratedClass"))
            fail(objectPath, "is a " + loadedClass + ", expected a generated class");
    } else
        checkText(loadedClass, className, describe(objectPath, "class"));
}

}

void checkLoadClass(const Cook& cook, std::string_view className) {
    const Json* assets = cook.expected.find("assets");
    if (assets == nullptr) {
        fail(cook.version, "the reference values have no assets");
        return;
    }
    withProvider(cook, [&](const ue5parse::FileProvider& provider) {
        rt::FRuntime runtime(provider);
        rt::SetLoadProblemHandler(collectProblem);
        int count = 0;
        for (const auto& [path, assetClass] : assets->getObject())
            if (assetClass.getString() == className) {
                loadAsset(path, className);
                count++;
            }
        rt::SetLoadProblemHandler(nullptr);
        if (count == 0)
            fail(className, "no asset of this class in the reference values");
    });
}

}
