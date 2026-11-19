#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"

namespace ue5parse::runtime {

class FConfigFile {
public:
    void ProcessInputFileContents(std::string_view Contents);
    bool GetString(std::string_view Section, std::string_view Key, FString& Value) const;

private:
    using FConfigSection = std::vector<std::pair<std::string, std::string>>;

    std::unordered_map<std::string, FConfigSection> Sections;
};

const FConfigFile& GetEngineIni();

}
