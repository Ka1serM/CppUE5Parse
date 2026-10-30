#pragma once

#include <cstdint>
#include <string>

#include "ue5parse/core/FName.h"
#include "ue5parse/package/ZenTypes.h"

namespace ue5parse {

class FileProvider;
class Package;

// What a reference of a package points to, without reading any object: an export of a loaded package, or a native
// script object ("/Script/Engine.StaticMesh"). A value; valid while its FileProvider is.
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
    bool isScriptObject() const { return script != nullptr; }
    const Package* getPackage() const { return package; }
    std::uint32_t getExportIndex() const { return exportIndex; }
    const FScriptObjectEntry* getScriptObject() const { return script; }

    FName getName() const;
    // The outer: another export or script object. Null for a top-level object (its outer is its package).
    ResolvedObject getOuter() const;
    // The class of an export. Null for a script object: the script objects do not store classes.
    ResolvedObject getClass() const;
    // "/Game/Maps/Ascent.Ascent:PersistentLevel.Light_3", or "/Script/Engine.StaticMesh". Empty for null.
    std::string getPathName() const;
    void appendPathName(std::string& out) const;

    bool operator==(const ResolvedObject& other) const {
        return package == other.package && exportIndex == other.exportIndex && script == other.script;
    }

private:
    const Package* package = nullptr;
    std::uint32_t exportIndex = 0;
    const FileProvider* provider = nullptr;
    const FScriptObjectEntry* script = nullptr;
};

}
