#include "CppUE5Parse/Package/ResolvedObject.h"

#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"

namespace ue5parse {

FName ResolvedObject::getName() const {
    if (import)
        return package->getImportName(exportIndex);
    if (package != nullptr)
        return package->getExportName(exportIndex);
    if (script != nullptr)
        return provider->getScriptObjects().getName(*script);
    return FName();
}

FName ResolvedObject::getClassName() const {
    if (import)
        return package->getImportClassName(exportIndex);
    return getClass().getName();
}

ResolvedObject ResolvedObject::getOuter() const {
    if (import) {
        std::int32_t outer = package->getImportOuterIndex(exportIndex);
        return outer < 0 ? makeImport(package, static_cast<std::uint32_t>(-(static_cast<std::int64_t>(outer) + 1))) : ResolvedObject();
    }
    if (package != nullptr)
        return package->resolve(package->getExport(exportIndex).outerIndex);
    if (script != nullptr) {
        const FScriptObjectEntry* outer = provider->getScriptObjects().getOuter(*script);
        if (outer != nullptr)
            return makeScript(provider, outer);
    }
    return {};
}

ResolvedObject ResolvedObject::getClass() const {
    if (package != nullptr && !import)
        return package->resolve(package->getExport(exportIndex).classIndex);
    return {};
}

void ResolvedObject::appendPathName(std::string& out) const {
    if (import) {
        ResolvedObject outer = getOuter();
        if (!outer.isNull()) {
            outer.appendPathName(out);
            out.push_back(outer.getOuter().isNull() ? '.' : ':');
        }
        getName().appendTo(out);
        return;
    }
    if (package != nullptr) {
        package->appendExportPathName(exportIndex, out);
        return;
    }
    if (script == nullptr)
        return;
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
