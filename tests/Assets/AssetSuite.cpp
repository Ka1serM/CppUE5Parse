#include "AssetSuite.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_set>

#include "CppUE5Parse/Crypto/Decryptor.h"
#include "CppUE5Parse/IO/FileSource.h"
#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Package/ZenTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"

namespace test::assets {

using namespace ue5parse;

namespace {

constexpr std::string_view contentFolder = "RefGen/Content/";
constexpr std::string_view aesKey = "52656647656e546573744b657930313233343536373839616263646566676869";
constexpr double translationTolerance = 0.05;
constexpr double rotationTolerance = 1e-3;

int loadProblems = 0;
std::unordered_set<std::string> missingPackages;

void countLoadProblem(const char* code, const rt::FString& message) {
    if (isOutsideCook(code, message)) {
        const std::string text = message.ToUtf8();
        missingPackages.insert(text.substr(0, text.find(" in /")));
        return;
    }
    loadProblems++;
    std::fprintf(stderr, "  load problem %s: %s\n", code, message.ToUtf8().c_str());
}

std::optional<EGame> findGame(std::string_view version) {
    std::size_t firstDot = version.find('.');
    std::size_t secondDot = version.find('.', firstDot + 1);
    std::string name = "GAME_UE" + std::string(version.substr(0, firstDot)) + "_" + std::string(version.substr(firstDot + 1, secondDot - firstDot - 1));
    return gameOfName(name);
}

std::optional<Json> readJson(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file)
        return std::nullopt;
    std::stringstream text;
    text << file.rdbuf();
    return Json::parse(text.str());
}

std::optional<MountedGame> mountCook(const Cook& cook) {
    MountOptions options;
    options.game = cook.game;
    KeyRing keys;
    keys.add(FGuid{}, *parseAesKey(aesKey));
    options.decryptor = std::make_unique<AesDecryptor>(keys);
    for (const std::filesystem::path& archive : cook.archives) {
        Result<std::unique_ptr<FileSource>> source = openLocalFile(archive);
        if (!source)
            printError(archive.string(), source.error());
        if (!CHECK(source.has_value()))
            return std::nullopt;
        options.archives.push_back(std::move(*source));
    }
    Result<MountedGame> mounted = mount(std::move(options));
    if (!mounted)
        printError("mount", mounted.error());
    if (!CHECK(mounted.has_value()))
        return std::nullopt;
    return std::move(*mounted);
}

//! A World Partition cell package holds the world of its source map, which keeps the map's name.
std::string getMainAssetName(std::string_view packagePath) {
    std::filesystem::path path(packagePath);
    std::filesystem::path folder = path.parent_path();
    if (folder.filename() == "_Generated_")
        return folder.parent_path().filename().string();
    return path.filename().string();
}

//! UE 4.3 FFbxImporter::MakeNameForMesh names a mesh <package name>_<FBX node name>, so the reference assets give the main object name.
std::vector<std::string> getMainAssetNames(const Json& expected, std::string_view packagePath) {
    std::vector<std::string> names{getMainAssetName(packagePath)};
    const Json* assets = expected.find("assets");
    if (assets == nullptr)
        return names;
    for (const auto& [objectPath, className] : assets->getObject())
        if (objectPath.size() > packagePath.size() && objectPath.starts_with(packagePath) && objectPath[packagePath.size()] == '.')
            names.push_back(objectPath.substr(packagePath.size() + 1));
    return names;
}

std::optional<std::size_t> findMainExport(const Package& package, const std::vector<std::string>& assetNames) {
    for (const std::string& assetName : assetNames)
        for (std::size_t i = 0; i < package.getExportCount(); i++) {
            std::string path = package.getExportPathName(i);
            std::string_view name = std::string_view(path).substr(path.rfind('.') + 1);
            if (name == assetName || name == assetName + "_C")
                return i;
        }
    return std::nullopt;
}

std::string getEnumName(const rt::UEnum* enumType, rt::int64 value) {
    std::string name = enumType->GetNameStringByValue(value).ToUtf8();
    std::size_t scope = name.rfind("::");
    return scope == std::string::npos ? name : name.substr(scope + 2);
}

const rt::UEnum* findEnum(const rt::FProperty* property) {
    if (const auto* enumProperty = rt::CastField<rt::FEnumProperty>(property))
        return enumProperty->GetEnum();
    if (const auto* numeric = rt::CastField<rt::FNumericProperty>(property))
        return numeric->GetIntPropertyEnum();
    return nullptr;
}

//! The editor's Python API names an enumerator of a user defined enum by its display name.
std::string getEnumDisplayName(const rt::UEnum* enumType, const rt::FProperty* property, const void* value) {
    const auto* enumProperty = rt::CastField<rt::FEnumProperty>(property);
    const rt::FNumericProperty* numeric = enumProperty != nullptr ? enumProperty->GetUnderlyingProperty() : rt::CastField<rt::FNumericProperty>(property);
    return enumType->GetDisplayNameTextByIndex(enumType->GetIndexByValue(numeric->GetSignedIntPropertyValue(value))).ToString().ToUtf8();
}

//! The reference values spell enumerators in upper snake case, as the editor's Python API prints them.
std::string normalizeEnumName(std::string_view name) {
    std::string normalized;
    for (char c : name)
        if (c != '_')
            normalized += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return normalized;
}

//! UE 4.26 names TC_EncodedReflectionCapture TC_ReflectionCapture; BaseEngine.ini redirects the old name to the new one.
std::string normalizeExpectedEnumName(std::string_view name) {
    std::string normalized = normalizeEnumName(name);
    return normalized == "tcreflectioncapture" ? "tcencodedreflectioncapture" : normalized;
}

std::optional<std::string> exportValue(const rt::FProperty* property, const void* value) {
    if (const auto* boolean = rt::CastField<rt::FBoolProperty>(property))
        return boolean->GetPropertyValue(value) ? "True" : "False";
    if (const auto* enumProperty = rt::CastField<rt::FEnumProperty>(property))
        return getEnumName(enumProperty->GetEnum(), enumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(value));
    if (const auto* numeric = rt::CastField<rt::FNumericProperty>(property)) {
        if (numeric->IsEnum())
            return getEnumName(numeric->GetIntPropertyEnum(), numeric->GetSignedIntPropertyValue(value));
        char text[64];
        if (numeric->IsFloatingPoint())
            std::snprintf(text, sizeof(text), "%f", numeric->GetFloatingPointPropertyValue(value));
        else
            std::snprintf(text, sizeof(text), "%lld", static_cast<long long>(numeric->GetSignedIntPropertyValue(value)));
        return text;
    }
    if (rt::CastField<rt::FNameProperty>(property))
        return static_cast<const rt::FName*>(value)->toString();
    if (rt::CastField<rt::FStrProperty>(property))
        return static_cast<const rt::FString*>(value)->ToUtf8();
    if (rt::CastField<rt::FTextProperty>(property))
        return static_cast<const rt::FText*>(value)->ToString().ToUtf8();
    if (rt::CastField<rt::FSoftObjectProperty>(property)) {
        const rt::FSoftObjectPath& path = static_cast<const rt::FSoftObjectPtr*>(value)->GetUniqueID();
        return path.IsNull() ? "None" : path.ToString().ToUtf8();
    }
    if (const auto* object = rt::CastField<rt::FObjectPropertyBase>(property))
        return getPathName(object->GetObjectPropertyValue(value));
    return std::nullopt;
}

std::string getStructName(const rt::FProperty* property) {
    const auto* structProperty = rt::CastField<rt::FStructProperty>(property);
    return structProperty == nullptr ? std::string() : structProperty->Struct->GetName().ToUtf8();
}

void checkArrayValue(const rt::FArrayProperty* array, const void* value, const Json& expected, const std::string& what) {
    rt::FScriptArrayHelper elements(array, value);
    const Json::Array& items = expected.getArray();
    if (!CHECK(elements.Num() == static_cast<rt::int32>(items.size())))
        return;
    for (rt::int32 i = 0; i < elements.Num(); i++)
        checkPropertyValue(array->Inner, elements.GetRawPtr(i), items[i], what + "[" + std::to_string(i) + "]");
}

void checkMapValue(const rt::FMapProperty* map, const void* value, const Json& expected, const std::string& what) {
    rt::FScriptMapHelper pairs(map, value);
    const Json::Object& items = expected.getObject();
    if (!CHECK(pairs.Num() == static_cast<rt::int32>(items.size())))
        return;
    for (const auto& [key, item] : items) {
        rt::int32 found = rt::INDEX_NONE;
        for (rt::int32 i = 0; i < pairs.GetMaxIndex() && found == rt::INDEX_NONE; i++)
            if (pairs.IsValidIndex(i) && exportValue(map->KeyProp, pairs.GetKeyPtr(i)) == key)
                found = i;
        if (found == rt::INDEX_NONE)
            fail(what, "has no key " + key);
        else
            checkPropertyValue(map->ValueProp, pairs.GetValuePtr(found), item, what + "[" + key + "]");
    }
}

//! A user defined struct names each member <name>_<index>_<GUID>, with the GUID as 32 hex digits.
std::string getAuthoredName(std::string name) {
    std::size_t guid = name.rfind('_');
    if (guid == std::string::npos || guid == 0 || name.size() - guid - 1 != 32)
        return name;
    std::size_t index = name.rfind('_', guid - 1);
    return index == std::string::npos ? name : name.substr(0, index);
}

const rt::FProperty* findMember(const rt::UStruct* type, std::string_view name) {
    for (const rt::FProperty* member = type->PropertyLink; member != nullptr; member = member->PropertyLinkNext)
        if (getAuthoredName(member->GetName().ToUtf8()) == name)
            return member;
    return nullptr;
}

void checkStructValue(const rt::FStructProperty* structProperty, const void* value, const Json& expected, const std::string& what) {
    for (const auto& [name, item] : expected.getObject()) {
        const rt::FProperty* member = findMember(structProperty->Struct, name);
        if (member == nullptr)
            fail(what, "has no member " + name);
        else
            checkPropertyValue(member, member->ContainerPtrToValuePtr<void>(value), item, what + "." + name);
    }
}

void checkSetValue(const rt::FSetProperty* set, const void* value, const Json& expected, const std::string& what) {
    rt::FScriptSetHelper elements(set, value);
    const Json::Array& items = expected.getArray();
    if (!CHECK(elements.Num() == static_cast<rt::int32>(items.size())))
        return;
    std::size_t item = 0;
    for (rt::int32 i = 0; i < elements.GetMaxIndex(); i++)
        if (elements.IsValidIndex(i)) {
            checkPropertyValue(set->ElementProp, elements.GetElementPtr(i), items[item], what + "[" + std::to_string(item) + "]");
            item++;
        }
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
        text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        text.remove_suffix(1);
    return text;
}

std::vector<std::string_view> splitTopLevel(std::string_view text) {
    std::vector<std::string_view> parts;
    int depth = 0;
    bool quoted = false;
    std::size_t start = 0;
    for (std::size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (quoted) {
            if (c == '\\')
                i++;
            else if (c == '"')
                quoted = false;
        } else if (c == '"')
            quoted = true;
        else if (c == '(' || c == '{' || c == '<')
            depth++;
        else if (c == ')' || c == '}' || c == '>')
            depth--;
        else if (c == ',' && depth == 0) {
            parts.push_back(trim(text.substr(start, i - start)));
            start = i + 1;
        }
    }
    if (!trim(text.substr(start)).empty())
        parts.push_back(trim(text.substr(start)));
    return parts;
}

std::string unquote(std::string_view text) {
    text = trim(text);
    if (text.size() < 2 || text.front() != '"' || text.back() != '"')
        return std::string(text);
    std::string result;
    for (std::size_t i = 1; i + 1 < text.size(); i++) {
        if (text[i] == '\\' && i + 2 < text.size())
            i++;
        result += text[i];
    }
    return result;
}

//! Both text forms write an object as Class'Path', with the path or the whole reference in double quotes.
std::string getObjectPathFromText(std::string_view text) {
    std::string unquoted = unquote(text);
    std::size_t open = unquoted.find('\'');
    std::size_t close = unquoted.rfind('\'');
    if (open == std::string::npos || close <= open)
        return unquoted;
    return unquote(std::string_view(unquoted).substr(open + 1, close - open - 1));
}

void checkTextValue(const rt::FProperty* property, const void* value, std::string_view text, const std::string& what);

//! UE 5.1 and later export a struct as (Name=Value,...); the references of 4.x and 5.0 hold the Python repr <Struct 'Name' (0x...) {name: value, ...}>.
void checkStructText(const rt::FStructProperty* structProperty, const void* value, std::string_view text, const std::string& what) {
    bool repr = text.starts_with("<Struct ");
    std::string_view members;
    if (repr) {
        std::size_t nameStart = text.find('\'') + 1;
        std::size_t nameEnd = text.find('\'', nameStart);
        checkText(structProperty->Struct->GetName().ToUtf8(), text.substr(nameStart, nameEnd - nameStart), what);
        std::size_t open = text.find('{');
        std::size_t close = text.rfind('}');
        if (open == std::string_view::npos || close < open) {
            fail(what, "has no members in the reference text");
            return;
        }
        members = text.substr(open + 1, close - open - 1);
    } else if (text.starts_with('(') && text.ends_with(')'))
        members = text.substr(1, text.size() - 2);
    else {
        fail(what, "is not a struct in the reference text");
        return;
    }
    for (std::string_view part : splitTopLevel(members)) {
        std::size_t separator = repr ? part.find(':') : part.find('=');
        if (separator == std::string_view::npos) {
            fail(what, "has a member without a value in the reference text");
            continue;
        }
        std::string name(trim(part.substr(0, separator)));
        std::string_view memberText = trim(part.substr(separator + 1));
        const rt::FProperty* member = nullptr;
        for (const rt::FProperty* candidate = structProperty->Struct->PropertyLink; candidate != nullptr && member == nullptr;
             candidate = candidate->PropertyLinkNext)
            if (normalizeEnumName(getAuthoredName(candidate->GetName().ToUtf8())) == normalizeEnumName(getAuthoredName(name)))
                member = candidate;
        if (member == nullptr)
            fail(what, "has no member " + name);
        else
            checkTextValue(member, member->ContainerPtrToValuePtr<void>(value), memberText, what + "." + getAuthoredName(name));
    }
}

void checkTextValue(const rt::FProperty* property, const void* value, std::string_view text, const std::string& what) {
    if (const auto* structProperty = rt::CastField<rt::FStructProperty>(property)) {
        checkStructText(structProperty, value, text, what);
        return;
    }
    if (const auto* array = rt::CastField<rt::FArrayProperty>(property)) {
        rt::FScriptArrayHelper elements(array, value);
        std::vector<std::string_view> items =
            text.starts_with('(') && text.ends_with(')') ? splitTopLevel(text.substr(1, text.size() - 2)) : splitTopLevel(text);
        if (!CHECK(elements.Num() == static_cast<rt::int32>(items.size())))
            return;
        for (rt::int32 i = 0; i < elements.Num(); i++)
            checkTextValue(array->Inner, elements.GetRawPtr(i), items[static_cast<std::size_t>(i)], what + "[" + std::to_string(i) + "]");
        return;
    }
    std::optional<std::string> actual = exportValue(property, value);
    if (!actual) {
        fail(what, "the test cannot export this property type as text");
        return;
    }
    if (rt::CastField<rt::FObjectPropertyBase>(property) || rt::CastField<rt::FSoftObjectProperty>(property)) {
        checkText(*actual, getObjectPathFromText(text), what);
        return;
    }
    if (const rt::UEnum* enumType = findEnum(property)) {
        std::string expected = normalizeEnumName(unquote(text));
        if (normalizeEnumName(*actual) != expected)
            checkText(normalizeEnumName(getEnumDisplayName(enumType, property, value)), expected, what);
        return;
    }
    if (const auto* numeric = rt::CastField<rt::FNumericProperty>(property)) {
        checkNear(numeric->IsFloatingPoint() ? numeric->GetFloatingPointPropertyValue(value) : static_cast<double>(numeric->GetSignedIntPropertyValue(value)),
                  std::strtod(std::string(text).c_str(), nullptr), valueTolerance, what);
        return;
    }
    //! FNameProperty::ExportText_Internal writes None as "" when the text is delimited, as in a struct.
    if (rt::CastField<rt::FNameProperty>(property) && text == "\"\"") {
        checkText(*actual, "None", what);
        return;
    }
    checkText(*actual, unquote(text), what);
}

bool isCooked(const std::set<std::string, std::less<>>& packages, std::string_view path) {
    return packages.contains(path.substr(0, path.find('.')));
}

Json keepCookedEntries(const Json& entries, const std::set<std::string, std::less<>>& packages) {
    Json::Object kept;
    for (const auto& [path, value] : entries.getObject())
        if (isCooked(packages, path))
            kept.emplace_back(path, value);
    return Json(std::move(kept));
}

//! The reference lists every asset of the editor, but the cook leaves out editor-only assets and the maps that are not in MapsToCook.
void keepCookedAssets(Cook& cook) {
    std::optional<MountedGame> mounted = mountCook(cook);
    if (!mounted)
        return;
    FileProvider provider(std::move(*mounted));
    std::vector<std::string> listed = listPackages(provider.getFiles());
    std::set<std::string, std::less<>> packages(listed.begin(), listed.end());
    Json::Object expected;
    for (const auto& [key, value] : cook.expected.getObject()) {
        if (key == "assets") {
            expected.emplace_back(key, keepCookedEntries(value, packages));
            continue;
        }
        if (key != "values") {
            expected.emplace_back(key, value);
            continue;
        }
        Json::Object kinds;
        for (const auto& [kind, entries] : value.getObject())
            kinds.emplace_back(kind, keepCookedEntries(entries, packages));
        expected.emplace_back(key, Json(std::move(kinds)));
    }
    cook.expected = Json(std::move(expected));
}

}

std::optional<Cook> openCook(std::string_view version, std::vector<std::filesystem::path> archives) {
    std::optional<EGame> game = findGame(version);
    if (!game)
        std::fprintf(stderr, "  no EGame preset for %.*s\n", static_cast<int>(version.size()), version.data());
    std::filesystem::path referencePath = archives.front().parent_path() / (std::string(version) + ".json");
    std::optional<Json> expected = readJson(referencePath);
    if (expected && (expected->find("assets") == nullptr || expected->find("values") == nullptr))
        expected.reset();
    if (!expected)
        std::fprintf(stderr, "  %s is missing or invalid\n", referencePath.string().c_str());
    if (!CHECK(game.has_value()) || !CHECK(expected.has_value()))
        return std::nullopt;
    Cook cook{std::move(archives), std::string(version), *game, std::move(*expected)};
    keepCookedAssets(cook);
    return cook;
}

void printError(std::string_view step, const Error& error) {
    std::fprintf(stderr, "  %.*s: %s at %s (%llu)\n", static_cast<int>(step.size()), step.data(), getErrorName(error.code), error.where,
                 static_cast<unsigned long long>(error.offset));
}

void withProvider(const Cook& cook, const std::function<void(const FileProvider&)>& check) {
    std::optional<MountedGame> mounted = mountCook(cook);
    if (!mounted)
        return;
    FileProvider provider(std::move(*mounted));
    check(provider);
}

void withRuntime(const Cook& cook, const std::function<void(const FileProvider&)>& check) {
    withProvider(cook, [&](const FileProvider& provider) {
        rt::FRuntime runtime(provider);
        loadProblems = 0;
        missingPackages.clear();
        rt::SetLoadProblemHandler(countLoadProblem);
        check(provider);
        rt::SetLoadProblemHandler(nullptr);
        CHECK(loadProblems == 0);
    });
}

std::vector<std::string> listPackages(const FileSystem& files) {
    std::vector<std::string> packages;
    char buffer[1024];
    for (const FileEntry& file : files.getFiles()) {
        std::string_view path = files.writePath(file, buffer);
        if (!path.starts_with(contentFolder) || !(path.ends_with(".uasset") || path.ends_with(".umap")))
            continue;
        std::filesystem::path relative(path.substr(contentFolder.size()));
        packages.push_back("/Game/" + relative.replace_extension().generic_string());
    }
    std::ranges::sort(packages);
    packages.erase(std::unique(packages.begin(), packages.end()), packages.end());
    return packages;
}

std::vector<std::string> loadMainObjectPaths(const FileProvider& provider, const Json& expected) {
    std::vector<std::string> packages = listPackages(provider.getFiles());
    CHECK(!packages.empty());
    std::vector<std::string> objectPaths;
    for (const std::string& packagePath : packages) {
        Result<const Package*> package = provider.loadPackage(packagePath);
        if (!package)
            printError(packagePath, package.error());
        if (!CHECK(package.has_value()))
            continue;
        std::optional<std::size_t> mainExport = findMainExport(**package, getMainAssetNames(expected, packagePath));
        if (CHECK(mainExport.has_value()))
            objectPaths.push_back((*package)->getExportPathName(*mainExport));
    }
    return objectPaths;
}

std::string getPathName(const rt::UObject* object) {
    return object == nullptr ? "None" : object->GetPathName().ToUtf8();
}

//! The reference cooks hold only /Game; an engine package that they import is not in them. A zen package before UE 5.3 keeps only the id of an imported package.
bool isOutsideCook(const char* code, const rt::FString& message) {
    return std::string_view(code) == "load.package_missing" && !message.ToUtf8().starts_with("/Game/");
}

bool isReportedMissing(std::string_view objectPath) {
    if (objectPath.starts_with("/Game/"))
        return false;
    const std::string package(objectPath.substr(0, objectPath.find('.')));
    return missingPackages.contains(std::string(objectPath)) || missingPackages.contains(package)
           || missingPackages.contains(std::format("package id {:016X}", FPackageId::fromName(package).value));
}

std::string describe(std::string_view path, std::string_view key) {
    return std::string(path) + " " + std::string(key);
}

std::pair<std::string_view, double> splitAt(std::string_view key) {
    std::size_t at = key.rfind('@');
    return {key.substr(0, at), std::strtod(std::string(key.substr(at + 1)).c_str(), nullptr)};
}

bool fail(std::string_view what, std::string_view problem) {
    std::fprintf(stderr, "  %.*s: %.*s\n", static_cast<int>(what.size()), what.data(), static_cast<int>(problem.size()), problem.data());
    return CHECK(false);
}

bool checkNear(double actual, double expected, double tolerance, std::string_view what) {
    double scale = std::max(1.0, std::abs(expected));
    if (std::abs(actual - expected) <= tolerance * scale)
        return true;
    std::fprintf(stderr, "  %.*s: %.7g, expected %.7g\n", static_cast<int>(what.size()), what.data(), actual, expected);
    return CHECK(false);
}

bool checkText(std::string_view actual, std::string_view expected, std::string_view what) {
    if (actual == expected)
        return true;
    std::fprintf(stderr, "  %.*s: %.*s, expected %.*s\n", static_cast<int>(what.size()), what.data(), static_cast<int>(actual.size()), actual.data(),
                 static_cast<int>(expected.size()), expected.data());
    return CHECK(false);
}

void checkNumbers(std::span<const double> actual, const Json& expected, double tolerance, std::string_view what) {
    if (!CHECK(expected.isArray() && expected.getArray().size() == actual.size()))
        return;
    for (std::size_t i = 0; i < actual.size(); i++)
        checkNear(actual[i], expected.getArray()[i].getNumber(), tolerance, what);
}

void checkVector(const rt::FVector& actual, const Json& expected, double tolerance, std::string_view what) {
    double values[] = {actual.X, actual.Y, actual.Z};
    checkNumbers(values, expected, tolerance, what);
}

void checkColor(const rt::FLinearColor& actual, const Json& expected, std::string_view what) {
    double values[] = {actual.R, actual.G, actual.B, actual.A};
    checkNumbers(values, expected, valueTolerance, what);
}

void checkTransform(const rt::FTransform& actual, const Json& expected, std::string_view what) {
    checkVector(actual.GetTranslation(), *expected.find("t"), translationTolerance, what);
    checkVector(actual.GetScale3D(), *expected.find("s"), valueTolerance, what);
    const Json::Array& rotation = expected.find("r")->getArray();
    rt::FQuat q = actual.GetRotation();
    double dot = q.X * rotation[0].getNumber() + q.Y * rotation[1].getNumber() + q.Z * rotation[2].getNumber() + q.W * rotation[3].getNumber();
    checkNear(std::abs(dot), 1.0, rotationTolerance, what);
}

void checkBounds(const rt::FBoxSphereBounds& bounds, const Json& expected, std::string_view path) {
    const Json::Array& values = expected.getArray();
    checkVector(bounds.Origin, values[0], valueTolerance, describe(path, "origin"));
    checkVector(bounds.BoxExtent, values[1], valueTolerance, describe(path, "extent"));
}

void checkPropertyValue(const rt::FProperty* property, const void* value, const Json& expected, const std::string& what) {
    if (expected.isBool()) {
        const auto* boolean = rt::CastField<rt::FBoolProperty>(property);
        if (boolean == nullptr)
            fail(what, "is not a bool");
        else if (boolean->GetPropertyValue(value) != expected.getBool())
            fail(what, expected.getBool() ? "is false, expected true" : "is true, expected false");
        return;
    }
    if (expected.isNumber()) {
        const auto* numeric = rt::CastField<rt::FNumericProperty>(property);
        if (numeric == nullptr || numeric->IsEnum()) {
            fail(what, "is not a number");
            return;
        }
        double actual =
            numeric->IsFloatingPoint() ? numeric->GetFloatingPointPropertyValue(value) : static_cast<double>(numeric->GetSignedIntPropertyValue(value));
        checkNear(actual, expected.getNumber(), valueTolerance, what);
        return;
    }
    if (const auto* structProperty = rt::CastField<rt::FStructProperty>(property); structProperty && expected.isString()) {
        checkStructText(structProperty, value, expected.getString(), what);
        return;
    }
    if (expected.isString()) {
        std::optional<std::string> actual = exportValue(property, value);
        if (!actual)
            fail(what, "the test cannot export this property type as text");
        else if (const rt::UEnum* enumType = findEnum(property))
            checkText(normalizeEnumName(getEnumDisplayName(enumType, property, value)), normalizeExpectedEnumName(expected.getString()), what);
        else
            checkText(*actual, expected.getString(), what);
        return;
    }
    std::string structName = getStructName(property);
    if (expected.isArray() && structName == "Vector") {
        checkVector(*static_cast<const rt::FVector*>(value), expected, valueTolerance, what);
        return;
    }
    if (expected.isArray() && structName == "LinearColor") {
        checkColor(*static_cast<const rt::FLinearColor*>(value), expected, what);
        return;
    }
    if (expected.isObject() && structName == "Transform") {
        checkTransform(*static_cast<const rt::FTransform*>(value), expected, what);
        return;
    }
    if (const auto* array = rt::CastField<rt::FArrayProperty>(property); array && expected.isArray()) {
        checkArrayValue(array, value, expected, what);
        return;
    }
    if (const auto* set = rt::CastField<rt::FSetProperty>(property); set && expected.isArray()) {
        checkSetValue(set, value, expected, what);
        return;
    }
    if (const auto* map = rt::CastField<rt::FMapProperty>(property); map && expected.isObject()) {
        checkMapValue(map, value, expected, what);
        return;
    }
    if (const auto* structProperty = rt::CastField<rt::FStructProperty>(property); structProperty && expected.isObject()) {
        checkStructValue(structProperty, value, expected, what);
        return;
    }
    fail(what, "the test cannot compare this property type");
}

void checkReflectedProperty(const std::string& path, std::string_view propertyName, const Json& expected) {
    rt::UObject* object = loadObject<rt::UObject>(path);
    if (object == nullptr)
        return;
    const rt::FProperty* property = object->GetClass()->FindPropertyByName(rt::FName(std::string(propertyName)));
    if (property == nullptr) {
        fail(describe(path, propertyName), "no such property");
        return;
    }
    checkPropertyValue(property, property->ContainerPtrToValuePtr<void>(object), expected, describe(path, propertyName));
}

}
