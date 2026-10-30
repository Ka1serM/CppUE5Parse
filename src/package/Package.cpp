#include <ankerl/unordered_dense.h>
#include <oneapi/tbb/collaborative_call_once.h>

#include "ue5parse/package/Package.h"
#include "ue5parse/provider/FileProvider.h"

namespace ue5parse {

namespace {

char toLowerAscii(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; }

struct IdHash {
    using is_avalanching = void;
    std::uint64_t operator()(std::uint64_t value) const noexcept { return value; }   // a CityHash64 already
};

struct NameHash {
    using is_avalanching = void;
    std::uint64_t operator()(const FName& name) const noexcept { return name.getHash() ^ (std::uint64_t(name.getNumber()) << 47); }
};

void appendLowerAscii(std::string_view text, std::string& out) {
    for (char c : text) out.push_back(toLowerAscii(c));
}

std::uint64_t hashLowerPath(std::string_view lowerPath) { return ankerl::unordered_dense::hash<std::string_view>{}(lowerPath); }

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); i++)
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    return true;
}

}

struct Package::Lazy {
    tbb::collaborative_once_flag lookupOnce;
    ankerl::unordered_dense::map<FName, std::uint32_t, NameHash> byName;
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, IdHash> byPublicHash;

    // Apart from the other lookups: only a load by path needs it, and it builds every export's path.
    tbb::collaborative_once_flag pathOnce;
    // By the hash of the lowercase full path, "/game/a/b.b:sub". The first export of a hash wins.
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, IdHash> byPath;
};

Package::Package(const FileProvider& provider, ChunkLocation chunk, FPackageId id, ZenPackageHeader header)
    : provider(&provider), chunk(chunk), id(id), header(std::move(header)), lazy(std::make_unique<Lazy>()) {}

Package::~Package() = default;

FName Package::getExportName(std::size_t index) const {
    return header.getName(header.getExportMap()[index].objectName, provider->getScriptObjects().getNames());
}

void Package::appendExportPathName(std::size_t index, std::string& out) const {
    // An outer chain longer than this is corrupt data.
    std::size_t chain[64];
    std::size_t depth = 0;
    std::size_t current = index;
    for (;;) {
        chain[depth++] = current;
        const FPackageObjectIndex& outer = header.getExportMap()[current].outerIndex;
        if (!outer.isExport() || outer.getIndex() >= getExportCount() || depth == 64) break;
        current = static_cast<std::size_t>(outer.getIndex());
    }
    getName().appendTo(out);
    out.push_back('.');
    // The first object below the top-level one takes ':', deeper ones '.' (SUBOBJECT_DELIMITER).
    for (std::size_t i = depth; i-- > 0;) {
        getExportName(chain[i]).appendTo(out);
        if (i > 0) out.push_back(i == depth - 1 ? ':' : '.');
    }
}

std::string Package::getExportPathName(std::size_t index) const {
    std::string out;
    appendExportPathName(index, out);
    return out;
}

const Package::Lazy& Package::getLookup() const {
    tbb::collaborative_call_once(lazy->lookupOnce, [&] {
        lazy->byName.reserve(getExportCount());
        lazy->byPublicHash.reserve(getExportCount());
        for (std::uint32_t i = 0; i < getExportCount(); i++) {
            // The first export of a name, unless a later one is top-level and the first is not.
            auto [found, inserted] = lazy->byName.try_emplace(getExportName(i), i);
            if (!inserted && getExportMapEntry(i).outerIndex.isNull() && !getExportMapEntry(found->second).outerIndex.isNull())
                found->second = i;
            lazy->byPublicHash.try_emplace(getExportMapEntry(i).publicExportHash, i);
        }
    });
    return *lazy;
}

std::int64_t Package::findExport(FName name) const {
    const Lazy& lookup = getLookup();
    auto found = lookup.byName.find(name);
    return found != lookup.byName.end() ? found->second : -1;
}

const Package::Lazy& Package::getPathLookup() const {
    tbb::collaborative_call_once(lazy->pathOnce, [&] {
        lazy->byPath.reserve(getExportCount());
        std::string path;
        std::string lowerPath;
        for (std::uint32_t i = 0; i < getExportCount(); i++) {
            path.clear();
            appendExportPathName(i, path);
            lowerPath.clear();
            appendLowerAscii(path, lowerPath);
            lazy->byPath.try_emplace(hashLowerPath(lowerPath), i);
        }
    });
    return *lazy;
}

std::int64_t Package::findExportByPath(std::string_view objectPath) const {
    std::string lowerPath;
    appendLowerAscii(getName().toString(), lowerPath);
    lowerPath.push_back('.');
    appendLowerAscii(objectPath, lowerPath);
    const Lazy& lookup = getPathLookup();
    auto found = lookup.byPath.find(hashLowerPath(lowerPath));
    if (found == lookup.byPath.end()) return -1;
    // Another path with the same hash is no match.
    if (!equalsIgnoreCase(getExportPathName(found->second), lowerPath)) return -1;
    return found->second;
}

std::int64_t Package::findExportByPublicHash(std::uint64_t publicExportHash) const {
    const Lazy& lookup = getLookup();
    auto found = lookup.byPublicHash.find(publicExportHash);
    return found != lookup.byPublicHash.end() ? found->second : -1;
}

ResolvedObject Package::resolve(FPackageObjectIndex index, std::stop_token stop) const {
    if (index.isExport()) {
        if (index.getIndex() >= getExportCount()) return {};
        return ResolvedObject::makeExport(this, static_cast<std::uint32_t>(index.getIndex()));
    }
    if (index.isScriptImport()) {
        const FScriptObjectEntry* entry = provider->getScriptObjects().find(index);
        if (entry == nullptr) return {};
        return ResolvedObject::makeScript(provider, entry);
    }
    if (!index.isPackageImport()) return {};
    std::uint32_t packageIndex = index.getImportedPackageIndex();
    std::uint32_t hashIndex = index.getImportedPublicExportHashIndex();
    std::span<const FPackageId> imported = header.getImportedPackages();
    std::span<const std::uint64_t> hashes = header.getImportedPublicExportHashes();
    if (packageIndex >= imported.size() || hashIndex >= hashes.size()) return {};
    Result<const Package*> other = provider->loadPackage(imported[packageIndex], stop);
    if (!other) return {};
    std::int64_t exportIndex = (*other)->findExportByPublicHash(hashes[hashIndex]);
    if (exportIndex < 0) return {};
    return ResolvedObject::makeExport(*other, static_cast<std::uint32_t>(exportIndex));
}

ResolvedObject Package::resolve(std::int32_t packageIndex, std::stop_token stop) const {
    if (packageIndex > 0) {
        if (static_cast<std::size_t>(packageIndex) > getExportCount()) return {};
        return ResolvedObject::makeExport(this, static_cast<std::uint32_t>(packageIndex - 1));
    }
    if (packageIndex == 0) return {};
    std::size_t import = static_cast<std::size_t>(-(static_cast<std::int64_t>(packageIndex) + 1));
    if (import >= header.getImportMap().size()) return {};
    return resolve(header.getImportMap()[import], stop);
}

Result<std::vector<std::byte>> Package::readExportBytes(std::size_t index, std::stop_token stop) const {
    if (index >= getExportCount()) return fail(ErrorCode::MissingExport, "Package.readExportBytes");
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "Package.readExportBytes");
    std::uint64_t begin = header.getExportChunkOffset(index);
    std::uint64_t size = getExportMapEntry(index).cookedSerialSize;
    std::uint64_t chunkSize = provider->getFiles().getChunkSize(chunk);
    if (begin > chunkSize || size > chunkSize - begin) return fail(ErrorCode::InvalidData, "Package.readExportBytes.range");
    std::vector<std::byte> bytes(size);
    Result<void> read = provider->getFiles().readChunk(chunk, begin, bytes, stop);
    if (!read) return std::unexpected(read.error());
    return bytes;
}

Result<std::vector<std::byte>> Package::readBulkData(EIoChunkType type, std::uint64_t offset, std::uint64_t size,
                                                     std::stop_token stop) const {
    std::optional<ChunkLocation> location = provider->getFiles().findChunk(FIoChunkId::create(id.value, 0, type));
    if (!location) return fail(ErrorCode::MissingChunk, "Package.readBulkData");
    std::uint64_t chunkSize = provider->getFiles().getChunkSize(*location);
    if (offset > chunkSize || size > chunkSize - offset) return fail(ErrorCode::InvalidData, "Package.readBulkData.range");
    std::vector<std::byte> out(size);
    Result<void> read = provider->getFiles().readChunk(*location, offset, out, stop);
    if (!read) return std::unexpected(read.error());
    return out;
}

}
