#include "ue5parse/registry/AssetRegistry.h"

#include <algorithm>

#include "ue5parse/core/ByteReader.h"
#include "ue5parse/core/FGuid.h"
#include "ue5parse/core/Text.h"
#include "ue5parse/package/ZenTypes.h"
#include "ue5parse/provider/FileProvider.h"

namespace ue5parse {

namespace {

// FAssetRegistryVersion::Type.
enum Version : std::int32_t {
    RemovedMD5Hash = 4,
    FixedTags = 8,
    ClassPaths = 14,
    RemoveAssetPathFNames = 15,
    AddedHeader = 16,
    MarshalledTextAsUTF8String = 19,
    MemoryMappedTagDataStore = 24,
};

constexpr FGuid registryVersionGuid{0x717F9EE7, 0xE9B0493A, 0x88B39132, 0x1B388107};
constexpr std::uint32_t numberedNameBit = 0x80000000u;
constexpr std::uint32_t storeOldBeginMagic = 0x12345678u;
constexpr std::uint32_t storeBeginMagic = 0x12345679u;
constexpr std::uint32_t storeEndMagic = 0x87654321u;

enum class ValueType : std::uint8_t { AnsiString, WideString, NumberlessName, Name, NumberlessExportPath, ExportPath, LocalizedText };

struct ValueId {
    ValueType type;
    std::uint32_t index;
};

ValueId readValueId(ByteReader& reader) {
    std::uint32_t id = reader.read<std::uint32_t>();
    return ValueId{static_cast<ValueType>(id & 7), id >> 3};
}

class RegistryReader {
public:
    RegistryReader(ByteReader& reader, std::int32_t version, std::span<const FName> names) : reader(reader), version(version), names(names) {}

    FName readName() {
        std::uint32_t index = reader.read<std::uint32_t>();
        std::uint32_t number = 0;
        if ((index & numberedNameBit) != 0) {
            index -= numberedNameBit;
            number = reader.read<std::uint32_t>();
        }
        if (reader.hasFailed()) return FName();
        if (index >= names.size()) {
            reader.fail(ErrorCode::InvalidData, "AssetRegistry.name");
            return FName();
        }
        return makeName(names, index, number);
    }

    std::string readString() {
        if (version >= MarshalledTextAsUTF8String) {
            std::uint32_t size = reader.readCount(1);
            std::span<const std::byte> text = reader.readBytes(size);
            return std::string(reinterpret_cast<const char*>(text.data()), text.size());
        }
        bool wide = false;
        std::span<const std::byte> text = reader.readStringBytes(wide);
        return StringView(text, wide).toString();
    }

    // FTopLevelAssetPath: the class's short name is the asset name.
    FName readClassName() {
        if (version >= ClassPaths) {
            readName();   // package
            return readName();
        }
        return readName();
    }

    void align() {
        if (version < MemoryMappedTagDataStore) return;
        std::size_t aligned = (reader.getPosition() + 15) / 16 * 16;
        reader.seek(aligned);
    }

    ByteReader& reader;
    std::int32_t version;
    std::span<const FName> names;
};

std::string exportPathText(FName className, FName objectName, FName packageName) {
    std::string out;
    if (!className.isNone()) {
        className.appendTo(out);
        out.push_back('\'');
    }
    packageName.appendTo(out);
    if (!objectName.isNone()) {
        out.push_back('.');
        objectName.appendTo(out);
    }
    if (!className.isNone()) out.push_back('\'');
    return out;
}

// FStore: the tag values of all assets, read into strings.
struct Store {
    std::vector<std::string> texts;
    std::vector<std::uint32_t> numberlessNames;
    std::vector<FName> names;
    std::vector<std::string> numberlessExportPaths;
    std::vector<std::string> exportPaths;
    std::vector<std::uint32_t> ansiOffsets;
    std::vector<std::uint32_t> wideOffsets;
    std::span<const std::byte> ansiStrings;
    std::span<const std::byte> wideStrings;
    std::vector<std::pair<std::uint32_t, ValueId>> numberlessPairs;
    std::vector<std::pair<FName, ValueId>> pairs;

    std::string getString(ValueId id, std::span<const FName> nameMap) const {
        switch (id.type) {
        case ValueType::AnsiString: {
            if (id.index >= ansiOffsets.size()) return {};
            std::size_t offset = ansiOffsets[id.index];
            std::size_t end = offset;
            while (end < ansiStrings.size() && ansiStrings[end] != std::byte{0}) end++;
            return std::string(reinterpret_cast<const char*>(ansiStrings.data()) + std::min(offset, end), end - std::min(offset, end));
        }
        case ValueType::WideString: {
            if (id.index >= wideOffsets.size()) return {};
            std::size_t offset = wideOffsets[id.index];
            std::size_t end = offset;
            while (end + 1 < wideStrings.size() && (wideStrings[end] != std::byte{0} || wideStrings[end + 1] != std::byte{0})) end += 2;
            if (offset > end) return {};
            return StringView(wideStrings.subspan(offset, end - offset), true).toString();
        }
        case ValueType::NumberlessName:
            if (id.index >= numberlessNames.size() || numberlessNames[id.index] >= nameMap.size()) return {};
            return std::string(nameMap[numberlessNames[id.index]].getText());
        case ValueType::Name:
            return id.index < names.size() ? names[id.index].toString() : std::string();
        case ValueType::NumberlessExportPath:
            return id.index < numberlessExportPaths.size() ? numberlessExportPaths[id.index] : std::string();
        case ValueType::ExportPath:
            return id.index < exportPaths.size() ? exportPaths[id.index] : std::string();
        case ValueType::LocalizedText:
            return id.index < texts.size() ? texts[id.index] : std::string();
        }
        return {};
    }
};

Result<void> readStore(RegistryReader& registry, Store& store) {
    ByteReader& reader = registry.reader;
    registry.align();
    std::uint32_t magic = reader.read<std::uint32_t>();
    bool textFirst = magic == storeBeginMagic;
    if (!textFirst && magic != storeOldBeginMagic) return fail(ErrorCode::BadMagic, "AssetRegistry.store");
    std::int32_t nums[11];
    for (std::int32_t& num : nums) num = reader.read<std::int32_t>();
    for (std::int32_t num : nums)
        if (num < 0) return fail(ErrorCode::InvalidData, "AssetRegistry.store.counts");
    auto alignIfTextFirst = [&] {
        if (textFirst) registry.align();
    };
    if (textFirst) {
        reader.skip(4);   // the text data size
        registry.align();
        for (std::int32_t i = 0; i < nums[4] && !reader.hasFailed(); i++) store.texts.push_back(registry.readString());
    }
    alignIfTextFirst();
    store.numberlessNames.resize(static_cast<std::size_t>(nums[0]));
    reader.readInto(std::span<std::uint32_t>(store.numberlessNames));
    alignIfTextFirst();
    for (std::int32_t i = 0; i < nums[1] && !reader.hasFailed(); i++) store.names.push_back(registry.readName());
    alignIfTextFirst();
    for (std::int32_t i = 0; i < nums[2] && !reader.hasFailed(); i++) {
        if (registry.version >= ClassPaths) reader.skip(4);   // class package
        std::uint32_t classObject = reader.read<std::uint32_t>();
        std::uint32_t object = reader.read<std::uint32_t>();
        std::uint32_t package = reader.read<std::uint32_t>();
        auto name = [&](std::uint32_t index) { return index < registry.names.size() ? registry.names[index] : FName(); };
        store.numberlessExportPaths.push_back(exportPathText(name(classObject), name(object), name(package)));
    }
    alignIfTextFirst();
    for (std::int32_t i = 0; i < nums[3] && !reader.hasFailed(); i++) {
        FName className = registry.readClassName();
        FName object = registry.readName();
        FName package = registry.readName();
        store.exportPaths.push_back(exportPathText(className, object, package));
    }
    if (!textFirst)
        for (std::int32_t i = 0; i < nums[4] && !reader.hasFailed(); i++) store.texts.push_back(registry.readString());
    alignIfTextFirst();
    store.ansiOffsets.resize(static_cast<std::size_t>(nums[5]));
    reader.readInto(std::span<std::uint32_t>(store.ansiOffsets));
    alignIfTextFirst();
    store.wideOffsets.resize(static_cast<std::size_t>(nums[6]));
    reader.readInto(std::span<std::uint32_t>(store.wideOffsets));
    alignIfTextFirst();
    store.ansiStrings = reader.readBytes(static_cast<std::size_t>(nums[7]));
    alignIfTextFirst();
    store.wideStrings = reader.readBytes(static_cast<std::size_t>(nums[8]) * 2);
    alignIfTextFirst();
    for (std::int32_t i = 0; i < nums[9] && !reader.hasFailed(); i++) {
        std::uint32_t key = reader.read<std::uint32_t>();
        store.numberlessPairs.emplace_back(key, readValueId(reader));
    }
    alignIfTextFirst();
    for (std::int32_t i = 0; i < nums[10] && !reader.hasFailed(); i++) {
        FName key = registry.readName();
        store.pairs.emplace_back(key, readValueId(reader));
    }
    if (reader.read<std::uint32_t>() != storeEndMagic && !reader.hasFailed()) return fail(ErrorCode::BadMagic, "AssetRegistry.store.end");
    return reader.getResult();
}

}

std::string FAssetData::getObjectPath() const {
    std::string out;
    packageName.appendTo(out);
    out.push_back('.');
    assetName.appendTo(out);
    return out;
}

const std::string* FAssetData::findTag(FName key) const {
    for (const auto& [name, value] : tagsAndValues)
        if (name == key) return &value;
    return nullptr;
}

FAssetRegistryState::FAssetRegistryState() : storage(std::make_unique<Arena>()) {}
FAssetRegistryState::FAssetRegistryState(FAssetRegistryState&&) noexcept = default;
FAssetRegistryState& FAssetRegistryState::operator=(FAssetRegistryState&&) noexcept = default;
FAssetRegistryState::~FAssetRegistryState() = default;

Result<FAssetRegistryState> parseAssetRegistry(std::span<const std::byte> file) {
    FAssetRegistryState state;
    state.bytes.assign(file.begin(), file.end());
    ByteReader reader(state.bytes);
    FGuid guid = reader.read<FGuid>();
    if (!(guid == registryVersionGuid)) return fail(ErrorCode::UnsupportedVersion, "AssetRegistry.version");
    state.version = reader.read<std::int32_t>();
    if (state.version < FixedTags) return fail(ErrorCode::UnsupportedVersion, "AssetRegistry.version");
    bool filterEditorOnly = state.version >= AddedHeader && reader.read<std::uint32_t>() != 0;

    Result<std::vector<FName>> names = readNameBatch(reader, *state.storage);
    if (!names) return std::unexpected(names.error());
    RegistryReader registry(reader, state.version, *names);
    Store store;
    Result<void> storeRead = readStore(registry, store);
    if (!storeRead) return std::unexpected(storeRead.error());

    std::uint32_t count = reader.readCount(24);
    state.assets.reserve(count);
    for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
        FAssetData asset;
        if (state.version < RemoveAssetPathFNames) registry.readName();   // ObjectPath
        asset.packagePath = registry.readName();
        asset.assetClass = registry.readClassName();
        if (state.version < RemovedMD5Hash) registry.readName();   // GroupNames
        asset.packageName = registry.readName();
        asset.assetName = registry.readName();
        if (state.version >= RemoveAssetPathFNames && !filterEditorOnly) registry.readName();   // OptionalOuterPath
        // Tags: a handle into the store's pairs.
        std::uint64_t handle = reader.read<std::uint64_t>();
        bool numberless = (handle >> 63) != 0;
        std::uint32_t pairCount = static_cast<std::uint16_t>(handle >> 32);
        std::uint32_t pairBegin = static_cast<std::uint32_t>(handle);
        for (std::uint32_t p = 0; p < pairCount; p++) {
            std::size_t at = std::size_t(pairBegin) + p;
            if (numberless) {
                if (at >= store.numberlessPairs.size()) break;
                const auto& [key, value] = store.numberlessPairs[at];
                FName name = key < names->size() ? (*names)[key] : FName();
                asset.tagsAndValues.emplace_back(name, store.getString(value, *names));
            } else {
                if (at >= store.pairs.size()) break;
                const auto& [key, value] = store.pairs[at];
                asset.tagsAndValues.emplace_back(key, store.getString(value, *names));
            }
        }
        // Asset bundles.
        std::uint32_t bundles = reader.readCount(12);
        for (std::uint32_t b = 0; b < bundles && !reader.hasFailed(); b++) {
            registry.readName();
            std::uint32_t assets = reader.readCount(8);
            for (std::uint32_t a = 0; a < assets && !reader.hasFailed(); a++) {
                registry.readName();
                if (state.version >= ClassPaths) registry.readName();
                registry.readString();
            }
        }
        std::uint32_t chunkCount = reader.readCount(4);
        asset.chunkIds.resize(chunkCount);
        reader.readInto(std::span<std::int32_t>(asset.chunkIds));
        asset.packageFlags = reader.read<std::uint32_t>();
        state.assets.push_back(std::move(asset));
    }
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    // ASCII names are views into the bytes, which the state keeps.
    return state;
}

Result<FAssetRegistryState> readAssetRegistry(const FileProvider& provider, std::string_view path) {
    Result<std::vector<std::byte>> bytes = provider.readFile(path);
    if (!bytes) return std::unexpected(bytes.error());
    return parseAssetRegistry(*bytes);
}

}
