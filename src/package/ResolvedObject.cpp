#include "ue5parse/package/ResolvedObject.h"

#include "ue5parse/package/Package.h"
#include "ue5parse/provider/FileProvider.h"

namespace ue5parse {

FName ResolvedObject::getName() const {
    if (package != nullptr) return package->getExportName(exportIndex);
    if (script != nullptr) return provider->getScriptObjects().getName(*script);
    return FName();
}

ResolvedObject ResolvedObject::getOuter() const {
    if (package != nullptr) return package->resolve(package->getExportMapEntry(exportIndex).outerIndex);
    if (script != nullptr) {
        const FScriptObjectEntry* outer = provider->getScriptObjects().getOuter(*script);
        if (outer != nullptr) return makeScript(provider, outer);
    }
    return {};
}

ResolvedObject ResolvedObject::getClass() const {
    if (package != nullptr) return package->resolve(package->getExportMapEntry(exportIndex).classIndex);
    return {};
}

void ResolvedObject::appendPathName(std::string& out) const {
    if (package != nullptr) {
        package->appendExportPathName(exportIndex, out);
        return;
    }
    if (script == nullptr) return;
    // "/Script/Engine" is a package; its objects are "/Script/Engine.StaticMesh".
    const FScriptObjectEntry* outer = provider->getScriptObjects().getOuter(*script);
    if (outer != nullptr) {
        makeScript(provider, outer).appendPathName(out);
        bool outerIsTopLevel = provider->getScriptObjects().getOuter(*outer) == nullptr;
        out.push_back(outerIsTopLevel ? '.' : ':');
    }
    getName().appendTo(out);
}

std::string ResolvedObject::getPathName() const {
    std::string out;
    appendPathName(out);
    return out;
}

}
