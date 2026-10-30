#include <ankerl/unordered_dense.h>
#include <oneapi/tbb/collaborative_call_once.h>

#include <atomic>

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

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); i++)
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
    return true;
}

}

struct Package::Lazy {
    tbb::collaborative_once_flag dataOnce;
    Result<void> dataResult;
    std::vector<std::byte> data;   // the whole package chunk

    tbb::collaborative_once_flag lookupOnce;
    ankerl::unordered_dense::map<FName, std::uint32_t, NameHash> byName;
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, IdHash> byPublicHash;
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

std::int64_t Package::findExportByPath(std::string_view objectPath) const {
    std::string prefix;
    getName().appendTo(prefix);
    prefix.push_back('.');
    std::string path;
    for (std::size_t i = 0; i < getExportCount(); i++) {
        path.clear();
        appendExportPathName(i, path);
        if (equalsIgnoreCase(std::string_view(path).substr(prefix.size()), objectPath)) return static_cast<std::int64_t>(i);
    }
    return -1;
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

Result<void> Package::loadData() const {
    tbb::collaborative_call_once(lazy->dataOnce, [&] {
        lazy->data.resize(provider->getFiles().getChunkSize(chunk));
        lazy->dataResult = provider->getFiles().readChunk(chunk, 0, lazy->data);
        if (!lazy->dataResult) lazy->data.clear();
    });
    return lazy->dataResult;
}

Result<std::span<const std::byte>> Package::getExportBytes(std::size_t index, std::stop_token stop) const {
    if (index >= getExportCount()) return fail(ErrorCode::MissingExport, "Package.getExportBytes");
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "Package.getExportBytes");
    Result<void> loaded = loadData();
    if (!loaded) return std::unexpected(loaded.error());
    std::uint64_t begin = header.getExportChunkOffset(index);
    std::uint64_t size = getExportMapEntry(index).cookedSerialSize;
    if (begin > lazy->data.size() || size > lazy->data.size() - begin) return fail(ErrorCode::InvalidData, "Package.getExportBytes.range");
    return std::span<const std::byte>(lazy->data).subspan(begin, size);
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
