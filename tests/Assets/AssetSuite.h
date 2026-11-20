#pragma once

#include <cstdio>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Core/Versions.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Json.h"
#include "Test.h"

namespace test::assets {

namespace rt = ue5parse::runtime;

struct Cook {
    std::vector<std::filesystem::path> archives;
    std::string version;
    ue5parse::EGame game;
    Json expected;
};

using CookCheck = void (*)(const Cook& cook);
using ValueCheck = void (*)(const std::string& path, const Json& value);

struct Checks {
    std::map<std::string, CookCheck, std::less<>> cook;
    std::map<std::string, ValueCheck, std::less<>> values;
};

void addPackageChecks(Checks& checks);
void addAnimBlueprintChecks(Checks& checks);
void addAnimationChecks(Checks& checks);
void addBlueprintChecks(Checks& checks);
void addDataChecks(Checks& checks);
void addJsonChecks(Checks& checks);
void addLevelChecks(Checks& checks);
void addMaterialChecks(Checks& checks);
void addMeshChecks(Checks& checks);
void addSoundChecks(Checks& checks);
void addTextureChecks(Checks& checks);

void checkLoadClass(const Cook& cook, std::string_view className);
void registerRefGenNatives(ue5parse::EGame game);

std::optional<Cook> openCook(std::string_view version, std::vector<std::filesystem::path> archives);
void printError(std::string_view step, const ue5parse::Error& error);
void withProvider(const Cook& cook, const std::function<void(const ue5parse::FileProvider&)>& check);
void withRuntime(const Cook& cook, const std::function<void(const ue5parse::FileProvider&)>& check);
std::vector<std::string> listPackages(const ue5parse::FileSystem& files);
std::vector<std::string> loadMainObjectPaths(const ue5parse::FileProvider& provider);

template <class T>
T* loadObject(std::string_view path) {
    T* object = rt::Cast<T>(rt::StaticLoadObject(nullptr, nullptr, rt::FString(std::string(path))));
    if (object == nullptr)
        std::fprintf(stderr, "  cannot load %.*s as the expected class\n", static_cast<int>(path.size()), path.data());
    CHECK(object != nullptr);
    return object;
}

std::string getPathName(const rt::UObject* object);
bool isOutsideCook(const char* code, const rt::FString& message);
bool isReportedMissing(std::string_view objectPath);
std::string describe(std::string_view path, std::string_view key);
std::pair<std::string_view, double> splitAt(std::string_view key);

bool fail(std::string_view what, std::string_view problem);
bool checkNear(double actual, double expected, double tolerance, std::string_view what);
bool checkText(std::string_view actual, std::string_view expected, std::string_view what);
void checkNumbers(std::span<const double> actual, const Json& expected, double tolerance, std::string_view what);
void checkVector(const rt::FVector& actual, const Json& expected, double tolerance, std::string_view what);
void checkColor(const rt::FLinearColor& actual, const Json& expected, std::string_view what);
void checkTransform(const rt::FTransform& actual, const Json& expected, std::string_view what);
void checkBounds(const rt::FBoxSphereBounds& bounds, const Json& expected, std::string_view path);
void checkPropertyValue(const rt::FProperty* property, const void* value, const Json& expected, const std::string& what);
void checkReflectedProperty(const std::string& path, std::string_view propertyName, const Json& expected);

constexpr double valueTolerance = 1e-4;

}
