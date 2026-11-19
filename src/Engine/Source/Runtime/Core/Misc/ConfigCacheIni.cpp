#include "Engine/Source/Runtime/Core/Misc/ConfigCacheIni.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <mutex>

#include "CppUE5Parse/Core/Check.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

namespace {

std::string_view Trim(std::string_view Text) {
    const auto IsSpace = [](char Character) { return std::isspace(static_cast<unsigned char>(Character)) != 0; };
    while (!Text.empty() && IsSpace(Text.front()))
        Text.remove_prefix(1);
    while (!Text.empty() && IsSpace(Text.back()))
        Text.remove_suffix(1);
    return Text;
}

std::string ToLower(std::string_view Text) {
    std::string Lower(Text);
    std::ranges::transform(Lower, Lower.begin(), [](unsigned char Character) { return static_cast<char>(std::tolower(Character)); });
    return Lower;
}

std::string ParseValue(std::string_view Value) {
    if (Value.size() < 2 || Value.front() != '"' || Value.back() != '"')
        return std::string(Value);
    std::string Unquoted;
    for (std::size_t Index = 1; Index + 1 < Value.size(); Index++) {
        if (Value[Index] == '\\' && Index + 2 < Value.size())
            Index++;
        Unquoted += Value[Index];
    }
    return Unquoted;
}

std::string FindProjectName(const FileSystem& Files) {
    std::array<char, 1024> Buffer;
    for (const FileEntry& File : Files.getFiles()) {
        const std::string_view Path = Files.writePath(File, Buffer);
        const std::size_t Slash = Path.find('/');
        if (Slash == std::string_view::npos || Path.find('/', Slash + 1) != std::string_view::npos)
            continue;
        const std::string_view Folder = Path.substr(0, Slash);
        if (Path.substr(Slash + 1) == std::string(Folder) + ".uproject")
            return std::string(Folder);
    }
    UE5PARSE_CHECK(false);
    return {};
}

void CombineIniFile(FConfigFile& Config, const FileProvider& Provider, const std::string& Path) {
    Result<std::vector<std::byte>> Bytes = Provider.readFile(Path);
    if (!Bytes && Bytes.error().code == ErrorCode::MissingFile)
        return;
    UE5PARSE_CHECK(Bytes.has_value());
    Config.ProcessInputFileContents(std::string_view(reinterpret_cast<const char*>(Bytes->data()), Bytes->size()));
}

std::unique_ptr<FConfigFile> LoadGlobalIniFile(const FileProvider& Provider, std::string_view BaseIniName) {
    //! The game runs on Windows, so its Windows platform layers apply.
    constexpr std::string_view Platform = "Windows";
    const std::string Project = FindProjectName(Provider.getFiles());
    const std::string Name(BaseIniName);
    const std::string PlatformName = std::string(Platform) + "/" + std::string(Platform) + Name;
    auto Config = std::make_unique<FConfigFile>();
    CombineIniFile(*Config, Provider, "Engine/Config/Base.ini");
    CombineIniFile(*Config, Provider, "Engine/Config/Base" + Name + ".ini");
    CombineIniFile(*Config, Provider, "Engine/Config/" + std::string(Platform) + "/Base" + std::string(Platform) + Name + ".ini");
    CombineIniFile(*Config, Provider, Project + "/Config/Default" + Name + ".ini");
    CombineIniFile(*Config, Provider, "Engine/Config/" + PlatformName + ".ini");
    CombineIniFile(*Config, Provider, Project + "/Config/" + PlatformName + ".ini");
    return Config;
}

}

void FConfigFile::ProcessInputFileContents(std::string_view Contents) {
    FConfigSection* Section = nullptr;
    while (!Contents.empty()) {
        const std::size_t End = Contents.find('\n');
        const std::string_view Line = Trim(Contents.substr(0, End));
        Contents.remove_prefix(End == std::string_view::npos ? Contents.size() : End + 1);
        if (Line.empty() || Line.front() == ';' || Line.front() == '#')
            continue;
        if (Line.front() == '[' && Line.back() == ']') {
            Section = &Sections[ToLower(Line.substr(1, Line.size() - 2))];
            continue;
        }
        const std::size_t Equals = Line.find('=');
        if (Section == nullptr || Equals == std::string_view::npos)
            continue;
        std::string_view Key = Trim(Line.substr(0, Equals));
        const std::string Value = ParseValue(Trim(Line.substr(Equals + 1)));
        const char Command = Key.empty() ? '\0' : Key.front();
        const bool bHasCommand = Command == '+' || Command == '.' || Command == '-' || Command == '!' || Command == '@' || Command == '*';
        if (bHasCommand)
            Key.remove_prefix(1);
        const std::string LowerKey = ToLower(Key);
        const auto MatchesKey = [&](const std::pair<std::string, std::string>& Entry) { return Entry.first == LowerKey; };
        const auto MatchesEntry = [&](const std::pair<std::string, std::string>& Entry) { return Entry.first == LowerKey && Entry.second == Value; };
        switch (Command) {
        case '+':
            if (std::ranges::none_of(*Section, MatchesEntry))
                Section->emplace_back(LowerKey, Value);
            break;
        case '.':
            Section->emplace_back(LowerKey, Value);
            break;
        case '-':
            if (const auto Found = std::ranges::find_if(*Section, MatchesEntry); Found != Section->end())
                Section->erase(Found);
            break;
        case '!':
            std::erase_if(*Section, MatchesKey);
            break;
        case '@':
        case '*':
            break;
        default:
            std::erase_if(*Section, MatchesKey);
            Section->emplace_back(LowerKey, Value);
            break;
        }
    }
}

bool FConfigFile::GetString(std::string_view Section, std::string_view Key, FString& Value) const {
    const auto FoundSection = Sections.find(ToLower(Section));
    if (FoundSection == Sections.end())
        return false;
    const std::string LowerKey = ToLower(Key);
    const auto Found = std::ranges::find(FoundSection->second, LowerKey, &std::pair<std::string, std::string>::first);
    if (Found == FoundSection->second.end())
        return false;
    Value = FString(std::string_view(Found->second));
    return true;
}

const FConfigFile& GetEngineIni() {
    static std::mutex Mutex;
    static const FileProvider* LoadedProvider = nullptr;
    static std::unique_ptr<FConfigFile> EngineIni;
    std::lock_guard Lock(Mutex);
    const FileProvider& Provider = GetRuntimeProvider();
    if (EngineIni == nullptr || LoadedProvider != &Provider) {
        EngineIni = LoadGlobalIniFile(Provider, "Engine");
        LoadedProvider = &Provider;
    }
    return *EngineIni;
}

}
