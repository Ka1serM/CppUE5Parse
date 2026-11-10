#pragma once

#include <cstdint>
#include <string>

#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Package/ZenTypes.h"

namespace ue5parse {

class FileProvider;
class Package;

class ResolvedObject {
public:
    ResolvedObject() = default;
    static ResolvedObject makeExport(const Package* package, std::uint32_t exportIndex) {
        ResolvedObject object;
        object.package = package;
        object.exportIndex = exportIndex;
        return object;
    }
    static ResolvedObject makeScript(const FileProvider* provider, const FScriptObjectEntry* entry) {
        ResolvedObject object;
        object.provider = provider;
        object.script = entry;
        return object;
    }

    bool isNull() const { return package == nullptr && script == nullptr; }
    bool isExport() const { return package != nullptr; }
    const Package* getPackage() const { return package; }
    std::uint32_t getExportIndex() const { return exportIndex; }

    FName getName() const;
    ResolvedObject getOuter() const;
    ResolvedObject getClass() const;
    std::string getPathName() const;
    void appendPathName(std::string& out) const;

    bool operator==(const ResolvedObject& other) const { return package == other.package && exportIndex == other.exportIndex && script == other.script; }

private:
    const Package* package = nullptr;
    std::uint32_t exportIndex = 0;
    const FileProvider* provider = nullptr;
    const FScriptObjectEntry* script = nullptr;
};

}
