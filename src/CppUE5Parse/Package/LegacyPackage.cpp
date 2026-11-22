#include "CppUE5Parse/Package/LegacyPackage.h"

#include <algorithm>

#include "CppUE5Parse/Core/ByteReader.h"
#include "CppUE5Parse/Core/Text.h"
#include "CppUE5Parse/Provider/FileProvider.h"

namespace ue5parse {

namespace {

constexpr std::uint32_t packageFileTag = 0x9E2A83C1;
//! State of Decay 2 starts its packages with "one" and omits the version fields.
constexpr std::uint32_t stateOfDecay2PackageTag = 0x00656E6F;
//! Ash Echoes replaces the package tag; the rest of the summary is standard.
constexpr std::uint32_t ashEchoesPackageTag = 0x56DE5ECA;
//! Tower of Fantasy XORs the summary counts and offsets with this value.
constexpr std::uint32_t towerOfFantasySummaryMask = 0xEEB2CEC7;
constexpr std::uint32_t packageFilterEditorOnly = 0x80000000;
constexpr std::uint32_t packageUnversionedProperties = 0x2000;
constexpr std::int32_t oldestLegacyFileVersion = -9;
constexpr std::uint32_t dataResourceAddedCookedIndex = 2;

using V4 = EUnrealEngineObjectUE4Version;
using V5 = EUnrealEngineObjectUE5Version;

std::int32_t unpackLegacyFileVersion(std::int32_t legacyFileVersion) {
    if (legacyFileVersion == 0xF7)
        legacyFileVersion = static_cast<std::int32_t>(0xFFFFFF00u | 0xF7u);
    if (legacyFileVersion >= 0 || (static_cast<std::uint32_t>(legacyFileVersion) >> 8) == 0xFFFFFF)
        return legacyFileVersion;
    return static_cast<std::int32_t>(0xFFFFFF00u | (static_cast<std::uint32_t>(legacyFileVersion) & 0xFF));
}

void skipString(ByteReader& reader) {
    bool wide = false;
    reader.readStringBytes(wide);
}

void skipEngineVersion(ByteReader& reader) {
    reader.skip(3 * sizeof(std::uint16_t) + sizeof(std::uint32_t));
    skipString(reader);
}

void skipArray(ByteReader& reader, std::size_t elementSize) {
    std::uint32_t count = reader.readCount(elementSize);
    reader.skip(std::size_t(count) * elementSize);
}

void readCustomVersions(ByteReader& reader, std::int32_t legacyFileVersion, std::vector<FCustomVersion>& out) {
    std::uint32_t count = reader.readCount(legacyFileVersion == -2 ? 8 : 20);
    out.reserve(count);
    for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
        FCustomVersion version;
        if (legacyFileVersion == -2) {
            version.key.d = reader.read<std::uint32_t>();
            version.version = reader.read<std::int32_t>();
        } else {
            version.key = reader.read<FGuid>();
            version.version = reader.read<std::int32_t>();
            if (legacyFileVersion >= -5)
                skipString(reader);
        }
        out.push_back(version);
    }
    std::ranges::sort(out, {}, &FCustomVersion::key);
}

FName readName(ByteReader& reader, std::span<const FName> names) {
    std::uint32_t index = reader.read<std::uint32_t>();
    std::uint32_t number = reader.read<std::uint32_t>();
    return makeName(names, index, number);
}

bool readBool(ByteReader& reader) {
    return reader.read<std::int32_t>() != 0;
}

struct SummaryOffsets {
    std::int32_t nameCount = 0;
    std::int32_t nameOffset = 0;
    std::int32_t exportCount = 0;
    std::int32_t exportOffset = 0;
    std::int32_t importCount = 0;
    std::int32_t importOffset = 0;
    std::int32_t dataResourceOffset = -1;
    std::uint32_t packageFlags = 0;
    //! State of Decay 2 stores its table offsets as if the summary had the standard size.
    std::int64_t offsetShift = 0;
};

std::size_t getTableOffset(const SummaryOffsets& summary, std::int32_t offset) {
    std::int64_t shifted = offset - summary.offsetShift;
    return shifted < 0 ? SIZE_MAX : static_cast<std::size_t>(shifted);
}

Result<SummaryOffsets> readSummary(ByteReader& reader, EGame game, PackageHeader& header) {
    SummaryOffsets summary;
    std::uint32_t tag = reader.read<std::uint32_t>();
    bool stateOfDecay2 = tag == stateOfDecay2PackageTag && game == EGame::GAME_StateOfDecay2;
    if (tag != packageFileTag && tag != ashEchoesPackageTag && !stateOfDecay2)
        return fail(ErrorCode::InvalidData, "parseLegacyPackageHeader.tag");
    std::int32_t legacyFileVersion = 0;
    std::int32_t licenseeVersion = 0;
    if (stateOfDecay2) {
        legacyFileVersion = reader.read<std::int32_t>();
        header.fileVersion = getPackageFileVersion(game);
        summary.packageFlags = packageFilterEditorOnly;
    } else {
        std::int32_t storedLegacyFileVersion = reader.read<std::int32_t>();
        //! Delta Force multiplies the legacy file version by 659.
        if (game == EGame::GAME_DeltaForce)
            storedLegacyFileVersion /= 659;
        legacyFileVersion = unpackLegacyFileVersion(storedLegacyFileVersion);
        if (legacyFileVersion >= 0 || legacyFileVersion < oldestLegacyFileVersion)
            return fail(ErrorCode::UnsupportedVersion, "parseLegacyPackageHeader.legacyFileVersion");
        if (legacyFileVersion != -4)
            reader.skip(sizeof(std::int32_t));
        FPackageFileVersion fileVersion;
        fileVersion.fileVersionUE4 = reader.read<std::int32_t>();
        //! Days Gone stores a wrong UE4 file version.
        if (game == EGame::GAME_DaysGone)
            fileVersion.fileVersionUE4 = 498;
        if (legacyFileVersion <= -8)
            fileVersion.fileVersionUE5 = reader.read<std::int32_t>();
        licenseeVersion = reader.read<std::int32_t>();
        bool unversioned = fileVersion.fileVersionUE4 == 0 && fileVersion.fileVersionUE5 == 0 && licenseeVersion == 0;
        header.fileVersion = unversioned ? getPackageFileVersion(game) : fileVersion;
    }
    const FPackageFileVersion& version = header.fileVersion;
    if (version < V4::OLDEST_LOADABLE_PACKAGE)
        return fail(ErrorCode::UnsupportedVersion, "parseLegacyPackageHeader.fileVersion");

    if (!stateOfDecay2) {
        if (version >= V5::PACKAGE_SAVED_HASH)
            reader.skip(20 + sizeof(std::int32_t));
        if (legacyFileVersion <= -2)
            readCustomVersions(reader, legacyFileVersion, header.customVersions);
        if (version < V5::PACKAGE_SAVED_HASH)
            reader.skip(sizeof(std::int32_t));
        skipString(reader);
        summary.packageFlags = reader.read<std::uint32_t>();
    }
    bool filterEditorOnly = (summary.packageFlags & packageFilterEditorOnly) != 0;
    summary.nameCount = reader.read<std::int32_t>();
    summary.nameOffset = reader.read<std::int32_t>();
    if (version >= V5::ADD_SOFTOBJECTPATH_LIST)
        reader.skip(2 * sizeof(std::int32_t));
    if (!filterEditorOnly && version >= V4::ADDED_PACKAGE_SUMMARY_LOCALIZATION_ID)
        skipString(reader);
    if (version >= V4::SERIALIZE_TEXT_IN_PACKAGES)
        reader.skip(2 * sizeof(std::int32_t));
    summary.exportCount = reader.read<std::int32_t>();
    summary.exportOffset = reader.read<std::int32_t>();
    summary.importCount = reader.read<std::int32_t>();
    summary.importOffset = reader.read<std::int32_t>();
    if (version >= V5::VERSE_CELLS)
        reader.skip(4 * sizeof(std::int32_t));
    if (version >= V5::METADATA_SERIALIZATION_OFFSET)
        reader.skip(sizeof(std::int32_t));
    reader.skip(sizeof(std::int32_t));
    if (version >= V4::ADD_STRING_ASSET_REFERENCES_MAP)
        reader.skip(2 * sizeof(std::int32_t));
    if (version >= V4::ADDED_SEARCHABLE_NAMES)
        reader.skip(sizeof(std::int32_t));
    reader.skip(sizeof(std::int32_t));
    if (version >= V5::IMPORT_TYPE_HIERARCHIES || game == EGame::GAME_DeltaForce)
        reader.skip(2 * sizeof(std::int32_t));
    if (version < V5::PACKAGE_SAVED_HASH)
        reader.skip(sizeof(FGuid));
    if (game == EGame::GAME_Valorant_PRE_11_2 || game == EGame::GAME_HYENAS)
        reader.skip(8);
    if (!filterEditorOnly && version >= V4::ADDED_PACKAGE_OWNER)
        reader.skip(sizeof(FGuid) * (version < V4::NON_OUTER_PACKAGE_IMPORT ? 2 : 1));
    skipArray(reader, 2 * sizeof(std::int32_t));
    if (version >= V4::ENGINE_VERSION_OBJECT)
        skipEngineVersion(reader);
    else
        reader.skip(sizeof(std::int32_t));
    if (version >= V4::PACKAGE_SUMMARY_HAS_COMPATIBLE_ENGINE_VERSION)
        skipEngineVersion(reader);
    reader.skip(sizeof(std::uint32_t));
    if (reader.read<std::int32_t>() != 0)
        return fail(ErrorCode::UnknownCompression, "parseLegacyPackageHeader.compressedChunks");
    reader.skip(sizeof(std::uint32_t));
    if (game == EGame::GAME_ArkSurvivalEvolved && licenseeVersion >= 10)
        reader.skip(8);
    std::uint32_t additionalPackages = reader.readCount(sizeof(std::int32_t));
    for (std::uint32_t i = 0; i < additionalPackages && !reader.hasFailed(); i++)
        skipString(reader);
    if (legacyFileVersion > -7)
        reader.skip(sizeof(std::int32_t));
    reader.skip(sizeof(std::int32_t));
    if (game == EGame::GAME_TowerOfFantasy)
        for (std::int32_t* field : {&summary.nameCount, &summary.nameOffset, &summary.exportCount, &summary.exportOffset, &summary.importCount, &summary.importOffset})
            *field = static_cast<std::int32_t>(static_cast<std::uint32_t>(*field) ^ towerOfFantasySummaryMask);
    if (game == EGame::GAME_SeaOfThieves || game == EGame::GAME_GearsOfWar4)
        reader.skip(6);
    header.bulkDataStartOffset = reader.read<std::int64_t>();
    if (version >= V4::WORLD_LEVEL_INFO)
        reader.skip(sizeof(std::int32_t));
    if (version >= V4::CHANGED_CHUNKID_TO_BE_AN_ARRAY_OF_CHUNKIDS)
        skipArray(reader, sizeof(std::int32_t));
    else if (version >= V4::ADDED_CHUNKID_TO_ASSETDATA_AND_UPACKAGE)
        reader.skip(sizeof(std::int32_t));
    if (version >= V4::PRELOAD_DEPENDENCIES_IN_COOKED_EXPORTS)
        reader.skip(2 * sizeof(std::int32_t));
    if (version >= V5::NAMES_REFERENCED_FROM_EXPORT_DATA)
        reader.skip(sizeof(std::int32_t));
    if (version >= V5::PAYLOAD_TOC)
        reader.skip(sizeof(std::int64_t));
    if (version >= V5::DATA_RESOURCES || game == EGame::GAME_TheFirstDescendant)
        summary.dataResourceOffset = reader.read<std::int32_t>();
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (stateOfDecay2)
        summary.offsetShift = summary.nameOffset - static_cast<std::int64_t>(reader.getPosition());
    header.unversionedProperties = (summary.packageFlags & packageUnversionedProperties) != 0;
    return summary;
}

Result<void> readNames(ByteReader& reader, const SummaryOffsets& summary, EGame game, PackageHeader& header,
                       const std::unordered_map<std::string, std::string>* nameReplacements) {
    if (summary.nameCount < 0 || summary.nameOffset < 0)
        return fail(ErrorCode::InvalidData, "parseLegacyPackageHeader.names");
    reader.seek(getTableOffset(summary, summary.nameOffset));
    if (static_cast<std::size_t>(summary.nameCount) > reader.getRemaining() / 5)
        return fail(ErrorCode::ReadPastEnd, "parseLegacyPackageHeader.nameCount");
    bool hashes = header.fileVersion >= V4::NAME_HASHES_SERIALIZED || game == EGame::GAME_GearsOfWar4 || game == EGame::GAME_DaysGone;
    header.names.reserve(static_cast<std::size_t>(summary.nameCount));
    for (std::int32_t i = 0; i < summary.nameCount && !reader.hasFailed(); i++) {
        bool wide = false;
        std::span<const std::byte> bytes = reader.readStringBytes(wide);
        std::string_view text = header.storage->copy(StringView(bytes, wide).toUtf8(*header.storage));
        if (nameReplacements != nullptr)
            if (auto found = nameReplacements->find(std::string(text)); found != nameReplacements->end())
                text = header.storage->copy(found->second);
        if (hashes)
            reader.skip(2 * sizeof(std::uint16_t));
        header.names.push_back(FName(text, hashName(text), 0));
    }
    return reader.getResult();
}

Result<std::vector<LegacyImport>> readImports(ByteReader& reader, const SummaryOffsets& summary, EGame game, const PackageHeader& header) {
    if (summary.importCount < 0 || summary.importOffset < 0)
        return fail(ErrorCode::InvalidData, "parseLegacyPackageHeader.imports");
    reader.seek(getTableOffset(summary, summary.importOffset));
    if (static_cast<std::size_t>(summary.importCount) > reader.getRemaining() / 28)
        return fail(ErrorCode::ReadPastEnd, "parseLegacyPackageHeader.importCount");
    bool filterEditorOnly = (summary.packageFlags & packageFilterEditorOnly) != 0;
    bool packageName = header.fileVersion >= V4::NON_OUTER_PACKAGE_IMPORT && (!filterEditorOnly || game >= EGame::GAME_UE5_8);
    std::vector<LegacyImport> imports(static_cast<std::size_t>(summary.importCount));
    for (LegacyImport& import : imports) {
        import.classPackage = readName(reader, header.names);
        import.className = readName(reader, header.names);
        import.outerIndex = reader.read<std::int32_t>();
        import.objectName = readName(reader, header.names);
        if (packageName)
            reader.skip(2 * sizeof(std::uint32_t));
        if (game == EGame::GAME_RacingMaster)
            reader.skip(1);
        if (header.fileVersion >= V5::OPTIONAL_RESOURCES)
            reader.skip(sizeof(std::int32_t));
    }
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    return imports;
}

Result<void> readExports(ByteReader& reader, const SummaryOffsets& summary, PackageHeader& header) {
    if (summary.exportCount < 0 || summary.exportOffset < 0)
        return fail(ErrorCode::InvalidData, "parseLegacyPackageHeader.exports");
    reader.seek(getTableOffset(summary, summary.exportOffset));
    if (static_cast<std::size_t>(summary.exportCount) > reader.getRemaining() / 40)
        return fail(ErrorCode::ReadPastEnd, "parseLegacyPackageHeader.exportCount");
    const FPackageFileVersion& version = header.fileVersion;
    header.exports.resize(static_cast<std::size_t>(summary.exportCount));
    for (PackageExport& entry : header.exports) {
        entry.classIndex = reader.read<std::int32_t>();
        entry.superIndex = reader.read<std::int32_t>();
        if (version >= V4::TemplateIndex_IN_COOKED_EXPORTS)
            entry.templateIndex = reader.read<std::int32_t>();
        entry.outerIndex = reader.read<std::int32_t>();
        entry.name = readName(reader, header.names);
        entry.objectFlags = reader.read<std::uint32_t>();
        std::int64_t serialSize = 0;
        std::int64_t serialOffset = 0;
        if (version < V4::e64BIT_EXPORTMAP_SERIALSIZES) {
            serialSize = reader.read<std::int32_t>();
            serialOffset = reader.read<std::int32_t>();
        } else {
            serialSize = reader.read<std::int64_t>();
            serialOffset = reader.read<std::int64_t>();
        }
        if (serialSize < 0 || serialOffset < 0)
            return fail(ErrorCode::InvalidData, "parseLegacyPackageHeader.serialRange");
        entry.serialSize = static_cast<std::uint64_t>(serialSize);
        entry.dataOffset = static_cast<std::uint64_t>(serialOffset);
        entry.cookedOffset = entry.dataOffset;
        reader.skip(3 * sizeof(std::int32_t));
        if (version < V5::REMOVE_OBJECT_EXPORT_PACKAGE_GUID)
            reader.skip(sizeof(FGuid));
        if (version >= V5::TRACK_OBJECT_EXPORT_IS_INHERITED)
            reader.skip(sizeof(std::int32_t));
        reader.skip(sizeof(std::uint32_t));
        if (version >= V4::LOAD_FOR_EDITOR_GAME)
            reader.skip(sizeof(std::int32_t));
        if (version >= V4::COOKED_ASSETS_IN_EDITOR_SUPPORT)
            reader.skip(sizeof(std::int32_t));
        if (version >= V5::OPTIONAL_RESOURCES)
            reader.skip(sizeof(std::int32_t));
        if (version >= V4::PRELOAD_DEPENDENCIES_IN_COOKED_EXPORTS)
            reader.skip(5 * sizeof(std::int32_t));
        if (!header.unversionedProperties && version >= V5::SCRIPT_SERIALIZATION_OFFSET)
            reader.skip(2 * sizeof(std::int64_t));
    }
    return reader.getResult();
}

Result<void> readDataResources(ByteReader& reader, const SummaryOffsets& summary, PackageHeader& header) {
    if (summary.dataResourceOffset <= 0)
        return {};
    reader.seek(getTableOffset(summary, summary.dataResourceOffset));
    std::uint32_t version = reader.read<std::uint32_t>();
    bool hasCookedIndex = version >= dataResourceAddedCookedIndex;
    std::size_t entrySize = hasCookedIndex ? 45 : 44;
    std::uint32_t count = reader.readCount(entrySize);
    header.bulkDataMap.reserve(count);
    for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
        FBulkDataMapEntry entry{};
        reader.skip(sizeof(std::uint32_t));
        std::uint8_t cookedIndex = hasCookedIndex ? reader.read<std::uint8_t>() : 0;
        entry.serialOffset = reader.read<std::int64_t>();
        entry.duplicateSerialOffset = reader.read<std::int64_t>();
        entry.serialSize = reader.read<std::int64_t>();
        reader.skip(sizeof(std::int64_t) + sizeof(std::int32_t));
        entry.flags = reader.read<std::uint32_t>();
        entry.pad = cookedIndex;
        header.bulkDataMap.push_back(entry);
    }
    return reader.getResult();
}

}

std::optional<std::string> getLegacyPackageName(std::string_view path) {
    std::size_t dot = path.rfind('.');
    if (dot == std::string_view::npos)
        return std::nullopt;
    std::string_view extension = path.substr(dot);
    if (extension != ".uasset" && extension != ".umap")
        return std::nullopt;
    std::size_t content = path.find("/Content/");
    if (content == std::string_view::npos)
        return std::nullopt;
    std::string_view root = path.substr(0, content);
    std::string_view mount = root.substr(root.rfind('/') == std::string_view::npos ? 0 : root.rfind('/') + 1);
    std::string_view rest = path.substr(content + 9, dot - content - 9);
    std::string name = "/";
    if (mount == "Engine")
        name += "Engine";
    else if (root.find("/Plugins/") != std::string_view::npos || root.starts_with("Plugins/"))
        name += mount;
    else
        name += "Game";
    name.push_back('/');
    name += rest;
    return name;
}

LegacyPackageFiles findLegacyPackageFiles(const FileSystem& fileSystem, FileEntry header, std::string_view headerPath) {
    LegacyPackageFiles files{header};
    std::string base(headerPath.substr(0, headerPath.rfind('.')));
    auto find = [&](std::string_view extension) { return fileSystem.findFile(base + std::string(extension)); };
    files.exports = find(".uexp");
    files.bulkData = find(".ubulk");
    files.optionalBulkData = find(".uptnl");
    files.memoryMappedBulkData = find(".m.ubulk");
    return files;
}

Result<LegacyPackageHeader> parseLegacyPackageHeader(std::span<const std::byte> bytes, std::string_view packageName, EGame game,
                                                     const std::unordered_map<std::string, std::string>* nameReplacements) {
    LegacyPackageHeader header;
    PackageHeader& common = header.common;
    ByteReader reader{bytes};
    Result<SummaryOffsets> summary = readSummary(reader, game, common);
    if (!summary)
        return std::unexpected(summary.error());
    common.name = FName(common.storage->copy(packageName));
    Result<void> names = readNames(reader, *summary, game, common, nameReplacements);
    if (!names)
        return std::unexpected(names.error());
    Result<std::vector<LegacyImport>> imports = readImports(reader, *summary, game, common);
    if (!imports)
        return std::unexpected(imports.error());
    header.imports = std::move(*imports);
    common.importCount = header.imports.size();
    Result<void> exports = readExports(reader, *summary, common);
    if (!exports)
        return std::unexpected(exports.error());
    Result<void> resources = readDataResources(reader, *summary, common);
    if (!resources)
        return std::unexpected(resources.error());
    return header;
}

std::string_view getAceCombat7FileName(std::string_view path) {
    std::string_view name = path.substr(0, path.rfind('.'));
    std::size_t slash = name.rfind('/');
    return slash == std::string_view::npos ? name : name.substr(slash + 1);
}

void decryptAceCombat7Header(std::span<std::byte> bytes, std::uint64_t offset, AceCombat7Stream stream, std::span<const std::byte> key) {
    constexpr std::uint64_t tagSize = sizeof(packageFileTag);
    for (std::uint64_t i = offset; i < tagSize && i - offset < bytes.size(); i++)
        bytes[i - offset] = static_cast<std::byte>(packageFileTag >> (i * 8));
    const std::uint64_t skip = offset < tagSize ? tagSize - offset : 0;
    if (skip < bytes.size())
        decryptAceCombat7(bytes.subspan(skip), stream, offset + skip - tagSize, key.first<aceCombat7KeySize>());
}

void decryptAceCombat7Exports(std::span<std::byte> bytes, std::uint64_t offset, std::uint64_t headerSize, std::uint64_t exportsSize, AceCombat7Stream stream,
                              std::span<const std::byte> key) {
    constexpr std::uint64_t tagSize = sizeof(packageFileTag);
    decryptAceCombat7(bytes, stream, headerSize - tagSize + offset, key.first<aceCombat7KeySize>());
    const std::uint64_t tagStart = exportsSize >= tagSize ? exportsSize - tagSize : 0;
    for (std::uint64_t i = std::max(offset, tagStart); i < offset + bytes.size(); i++)
        bytes[i - offset] = static_cast<std::byte>(packageFileTag >> ((i - tagStart) * 8));
}

LegacyPackage::LegacyPackage(const FileProvider& provider, LegacyPackageFiles files, std::uint64_t headerFileSize, LegacyPackageHeader header,
                             std::optional<AceCombat7Stream> aceCombat7)
    : Package(provider, std::move(header.common)), files(files), headerFileSize(headerFileSize), aceCombat7(aceCombat7), imports(std::move(header.imports)),
      loaded(std::make_unique<std::atomic<const Package*>[]>(imports.size())) {}

LegacyPackage::~LegacyPackage() = default;

FName LegacyPackage::getImportName(std::size_t importIndex) const {
    return importIndex < imports.size() ? imports[importIndex].objectName : FName();
}

std::int32_t LegacyPackage::getImportOuterIndex(std::size_t importIndex) const {
    return importIndex < imports.size() ? imports[importIndex].outerIndex : 0;
}

FName LegacyPackage::getImportClassName(std::size_t importIndex) const {
    return importIndex < imports.size() ? imports[importIndex].className : FName();
}

std::optional<std::size_t> LegacyPackage::findTopImport(std::size_t importIndex) const {
    std::size_t current = importIndex;
    for (std::size_t depth = 0; depth < imports.size(); depth++) {
        std::int32_t outer = imports[current].outerIndex;
        if (outer == 0)
            return current;
        if (outer > 0)
            return std::nullopt;
        std::size_t next = static_cast<std::size_t>(-(static_cast<std::int64_t>(outer) + 1));
        if (next >= imports.size())
            return std::nullopt;
        current = next;
    }
    return std::nullopt;
}

bool LegacyPackage::isNativeImport(std::size_t topImport) const {
    return imports[topImport].objectName.getText().starts_with("/Script/");
}

Result<const Package*> LegacyPackage::loadImportedPackage(std::size_t topImport, std::stop_token stop) const {
    const Package* other = loaded[topImport].load(std::memory_order_acquire);
    if (other != nullptr)
        return other;
    Result<const Package*> result = getProvider().loadPackage(imports[topImport].objectName.toString(), stop);
    if (!result)
        return result;
    loaded[topImport].store(*result, std::memory_order_release);
    return *result;
}

std::string LegacyPackage::getImportPathBelow(std::size_t topImport, std::size_t importIndex) const {
    std::size_t chain[64];
    std::size_t depth = 0;
    for (std::size_t current = importIndex; current != topImport && depth < 64;) {
        chain[depth++] = current;
        current = static_cast<std::size_t>(-(static_cast<std::int64_t>(imports[current].outerIndex) + 1));
    }
    std::string path;
    for (std::size_t i = depth; i-- > 0;) {
        imports[chain[i]].objectName.appendTo(path);
        if (i > 0)
            path.push_back(i == depth - 1 ? ':' : '.');
    }
    return path;
}

ResolvedObject LegacyPackage::resolve(std::int32_t packageIndex, std::stop_token stop) const {
    if (packageIndex > 0) {
        if (static_cast<std::size_t>(packageIndex) > getExportCount())
            return {};
        return ResolvedObject::makeExport(this, static_cast<std::uint32_t>(packageIndex - 1));
    }
    if (packageIndex == 0)
        return {};
    std::size_t importIndex = static_cast<std::size_t>(-(static_cast<std::int64_t>(packageIndex) + 1));
    if (importIndex >= imports.size())
        return {};
    std::optional<std::size_t> top = findTopImport(importIndex);
    if (!top)
        return {};
    ResolvedObject import = ResolvedObject::makeImport(this, static_cast<std::uint32_t>(importIndex));
    if (isNativeImport(*top) || *top == importIndex)
        return import;
    Result<const Package*> other = loadImportedPackage(*top, stop);
    if (!other)
        return other.error().code == ErrorCode::MissingPackage ? ResolvedObject::makeMissingImport(this, static_cast<std::uint32_t>(importIndex)) : import;
    std::int64_t exportIndex = (*other)->findExportByPath(getImportPathBelow(*top, importIndex));
    if (exportIndex < 0)
        return import;
    return ResolvedObject::makeExport(*other, static_cast<std::uint32_t>(exportIndex));
}

bool LegacyPackage::resolveLoads(std::int32_t packageIndex) const {
    if (packageIndex >= 0)
        return false;
    std::size_t importIndex = static_cast<std::size_t>(-(static_cast<std::int64_t>(packageIndex) + 1));
    if (importIndex >= imports.size())
        return false;
    std::optional<std::size_t> top = findTopImport(importIndex);
    if (!top || *top == importIndex || isNativeImport(*top))
        return false;
    return loaded[*top].load(std::memory_order_acquire) == nullptr;
}

std::uint64_t LegacyPackage::getExportDataSize() const {
    std::uint64_t size = headerFileSize;
    if (files.exports)
        size += getProvider().getFiles().getFileSize(*files.exports);
    return size;
}

Result<void> LegacyPackage::readExportData(std::uint64_t offset, std::span<std::byte> out, std::stop_token stop) const {
    std::uint64_t dataSize = getExportDataSize();
    if (offset > dataSize || out.size() > dataSize - offset)
        return fail(ErrorCode::InvalidData, "LegacyPackage.readExportData.range");
    const FileSystem& fileSystem = getProvider().getFiles();
    if (offset < headerFileSize) {
        std::size_t inHeader = static_cast<std::size_t>(std::min<std::uint64_t>(out.size(), headerFileSize - offset));
        Result<void> read = fileSystem.readFile(files.header, offset, out.first(inHeader), stop);
        if (!read)
            return read;
        if (aceCombat7)
            decryptAceCombat7Header(out.first(inHeader), offset, *aceCombat7, getProvider().getGameData().aceCombat7Key);
        out = out.subspan(inHeader);
        offset = headerFileSize;
    }
    if (out.empty())
        return {};
    Result<void> read = fileSystem.readFile(*files.exports, offset - headerFileSize, out, stop);
    if (read && aceCombat7)
        decryptAceCombat7Exports(out, offset - headerFileSize, headerFileSize, fileSystem.getFileSize(*files.exports), *aceCombat7,
                                 getProvider().getGameData().aceCombat7Key);
    return read;
}

std::optional<FileEntry> LegacyPackage::findBulkData(EIoChunkType type, std::uint8_t cookedIndex) const {
    if (cookedIndex != 0)
        return std::nullopt;
    if (type == EIoChunkType::OptionalBulkData)
        return files.optionalBulkData;
    if (type == EIoChunkType::MemoryMappedBulkData)
        return files.memoryMappedBulkData;
    return files.bulkData;
}

std::optional<std::uint64_t> LegacyPackage::getBulkDataSize(EIoChunkType type, std::uint8_t cookedIndex) const {
    std::optional<FileEntry> file = findBulkData(type, cookedIndex);
    if (!file)
        return std::nullopt;
    return getProvider().getFiles().getFileSize(*file);
}

Result<void> LegacyPackage::readBulkData(EIoChunkType type, std::uint8_t cookedIndex, std::uint64_t offset, std::span<std::byte> out) const {
    std::optional<FileEntry> file = findBulkData(type, cookedIndex);
    if (!file)
        return fail(ErrorCode::MissingChunk, "LegacyPackage.readBulkData");
    std::uint64_t size = getProvider().getFiles().getFileSize(*file);
    if (offset > size || out.size() > size - offset)
        return fail(ErrorCode::InvalidData, "LegacyPackage.readBulkData.range");
    return getProvider().getFiles().readFile(*file, offset, out);
}

}
