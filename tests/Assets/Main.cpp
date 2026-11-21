#include <algorithm>
#include <cstdio>
#include <fstream>
#include <set>

#include "AssetSuite.h"

using namespace test::assets;
using test::Json;

namespace {

constexpr std::string_view loadPrefix = "load/";

Checks collectChecks() {
    Checks checks;
    addPackageChecks(checks);
    addAnimBlueprintChecks(checks);
    addAnimationChecks(checks);
    addBlueprintChecks(checks);
    addDataChecks(checks);
    addJsonChecks(checks);
    addLevelChecks(checks);
    addMaterialChecks(checks);
    addMeshChecks(checks);
    addSoundChecks(checks);
    addTextureChecks(checks);
    return checks;
}

void runValueCheck(const Cook& cook, ValueCheck check, std::string_view kind) {
    const Json* values = cook.expected.find("values");
    const Json* entries = values == nullptr ? nullptr : values->find(kind);
    if (entries == nullptr) {
        fail(kind, "the reference values have none of this kind");
        return;
    }
    withRuntime(cook, [&](const ue5parse::FileProvider&) {
        for (const auto& [path, value] : entries->getObject())
            check(path, value);
    });
}

void runCheck(std::string_view version, std::string_view name, std::vector<std::filesystem::path> archives) {
    std::optional<Cook> cook = openCook(version, std::move(archives));
    if (!cook)
        return;
    registerRefGenNatives(cook->game);
    Checks checks = collectChecks();
    if (name.starts_with(loadPrefix))
        checkLoadClass(*cook, name.substr(loadPrefix.size()));
    else if (auto found = checks.cook.find(name); found != checks.cook.end())
        found->second(*cook);
    else if (auto value = checks.values.find(name); value != checks.values.end())
        runValueCheck(*cook, value->second, name);
    else
        fail(name, "the asset suite has no check of this name");
}

std::string quote(std::string_view text) { return "[==[" + std::string(text) + "]==]"; }

struct Writer {
    std::ofstream out;
    std::string executable;
    int count = 0;

    void add(const std::string& name, std::string_view version, std::string_view check, const std::vector<std::filesystem::path>& archives,
             const std::string& labels) {
        out << "add_test(" << quote(name) << " " << quote(executable) << " " << quote(version) << " " << quote(check);
        for (const std::filesystem::path& archive : archives)
            out << " " << quote(archive.generic_string());
        out << ")\nset_tests_properties(" << quote(name) << " PROPERTIES LABELS " << quote(labels) << ")\n";
        count++;
    }
};

void listCook(Writer& writer, const std::string& prefix, const std::string& version, std::string_view cookKind, const std::vector<std::filesystem::path>& archives,
              const std::vector<std::string>& checks) {
    std::string base = prefix + version + "/" + std::string(cookKind) + "/";
    for (const std::string& check : checks) {
        std::string group = check.starts_with(loadPrefix) ? "load;" + check.substr(loadPrefix.size()) : check.substr(0, check.find('.'));
        writer.add(base + check, version, check, archives, "assets;" + prefix + version + ";" + std::string(cookKind) + ";" + group);
    }
}

void listFolder(Writer& writer, const std::filesystem::path& folder, const std::string& prefix) {
    std::vector<std::filesystem::path> paks;
    for (const auto& entry : std::filesystem::directory_iterator(folder))
        if (entry.path().extension() == ".pak")
            paks.push_back(std::filesystem::absolute(entry.path()));
    std::ranges::sort(paks);
    for (const std::filesystem::path& pak : paks) {
        std::string name = pak.stem().string();
        std::string version = name.ends_with("-inline") ? name.substr(0, name.size() - 7) : name;
        std::vector<std::string> checks = {"mount", "files", "packages", "objects", "shaders", "animbp", "json", "texture.pixels"};
        test::getFailures() = 0;
        if (std::optional<Cook> cook = openCook(version, {pak})) {
            if (const Json* values = cook->expected.find("values"))
                for (const auto& [kind, entries] : values->getObject())
                    checks.push_back(kind);
            std::set<std::string> classes;
            if (const Json* assets = cook->expected.find("assets"))
                for (const auto& [path, assetClass] : assets->getObject())
                    classes.insert(assetClass.getString());
            for (const std::string& assetClass : classes)
                checks.push_back(std::string(loadPrefix) + assetClass);
        }
        std::filesystem::path utoc = pak.parent_path() / (name + ".utoc");
        if (std::filesystem::exists(utoc)) {
            std::filesystem::path global = pak.parent_path() / (name + "-global");
            listCook(writer, prefix, version, "iostore", {utoc, pak.parent_path() / (name + ".ucas"), global.string() + ".utoc", global.string() + ".ucas"}, checks);
        }
        checks.push_back("registry");
        listCook(writer, prefix, version, name == version ? "pak" : "inline", {pak}, checks);
    }
}

int listTests(int argc, char** argv) {
    if (argc < 6 || argc % 2 != 0) {
        std::fprintf(stderr, "usage: ue5parse_asset_tests --list <out.cmake> <executable> (<folder> <prefix>)...\n");
        return 2;
    }
    std::string temporary = std::string(argv[2]) + ".tmp";
    Writer writer{std::ofstream(temporary), argv[3]};
    for (int i = 4; i + 1 < argc; i += 2)
        if (std::filesystem::is_directory(argv[i]))
            listFolder(writer, argv[i], argv[i + 1] == std::string_view("-") ? "" : argv[i + 1]);
    writer.out.close();
    std::filesystem::rename(temporary, argv[2]);
    std::printf("%d asset tests\n", writer.count);
    return 0;
}

}

int main(int argc, char** argv) {
    if (argc > 1 && argv[1] == std::string_view("--list"))
        return listTests(argc, argv);
    if (argc < 4) {
        std::fprintf(stderr, "usage: ue5parse_asset_tests <version> <check> <archive>...\n");
        return 2;
    }
    runCheck(argv[1], argv[2], std::vector<std::filesystem::path>(argv + 3, argv + argc));
    std::printf("%s %s %s, %d failed checks\n", test::getFailures() == 0 ? "ok  " : "FAIL", argv[1], argv[2], test::getFailures());
    return test::getFailures() == 0 ? 0 : 1;
}
