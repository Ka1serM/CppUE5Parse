#include <string>

#include "AssetSuite.h"
#include "CppUE5Parse/Package/Package.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/JsonUtilities/JsonObjectConverter.h"

namespace test::assets {

namespace {

void ignoreProblem(const char*, const rt::FString&) {}

std::string exportPackage(const ue5parse::Package& package) {
    rt::FLinkerLoad* linker = rt::FLinkerLoad::FindOrCreate(package);
    std::string output;
    rt::FJsonWriter writer(output);
    writer.WriteArrayStart();
    for (std::size_t index = 0; index < package.getExportCount(); index++)
        if (const rt::UObject* object = linker->CreateExport(static_cast<rt::int32>(index)))
            rt::FJsonObjectConverter::UObjectToJsonObject(object, writer);
    writer.WriteArrayEnd();
    return output;
}

void checkJsonExport(const Cook& cook) {
    withProvider(cook, [](const ue5parse::FileProvider& provider) {
        rt::FRuntime runtime(provider);
        rt::SetLoadProblemHandler(ignoreProblem);
        rt::FLinkerLoad::SetKeepUnknownProperties(true);
        for (const std::string& packagePath : listPackages(provider.getFiles())) {
            ue5parse::Result<const ue5parse::Package*> package = provider.loadPackage(packagePath);
            if (!package)
                continue;
            std::optional<Json> json = Json::parse(exportPackage(**package));
            if (!json || !json->isArray())
                fail(packagePath, "exports no valid JSON");
            else if (json->getArray().empty())
                fail(packagePath, "exports no object");
        }
        rt::FLinkerLoad::SetKeepUnknownProperties(false);
        rt::SetLoadProblemHandler(nullptr);
    });
}

}

void addJsonChecks(Checks& checks) {
    checks.cook.emplace("json", checkJsonExport);
}

}
