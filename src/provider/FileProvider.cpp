#include <oneapi/tbb/collaborative_call_once.h>


#include "ue5parse/package/Package.h"
#include "ue5parse/provider/FileProvider.h"

namespace ue5parse {

namespace {

// A package load, run once. A thread that asks while another loads helps with the load's parallel work.
struct PackageSlot {
    tbb::collaborative_once_flag once;
    Result<std::unique_ptr<Package>> result;
};

// "/Game/A/B.B:Sub" -> "/Game/A/B"; "/Game/A/B.umap" -> "/Game/A/B".
std::string_view getPackageName(std::string_view path) {
    std::size_t slash = path.rfind('/');
    std::size_t dot = path.find('.', slash == std::string_view::npos ? 0 : slash);
    std::size_t colon = path.find(':', slash == std::string_view::npos ? 0 : slash);
    std::size_t end = dot < colon ? dot : colon;
    return end == std::string_view::npos ? path : path.substr(0, end);
}

}

struct FileProvider::Cache {
    // By the package's dense index in the PackageStore: one array index, no lock (design §7.5).
    std::unique_ptr<PackageSlot[]> packages;
};

Result<MountedGame> mount(MountOptions options, std::stop_token stop) {
    std::unique_ptr<Codecs> codecs = options.codecs != nullptr ? std::move(options.codecs) : std::make_unique<Codecs>();
    Mappings mappings;
    if (options.mappings != nullptr) {
        Result<Mappings> parsed = readMappings(*options.mappings, *codecs);
        if (!parsed) return std::unexpected(parsed.error());
        mappings = std::move(*parsed);
    }
    Result<ArchiveSet> set = readArchives(std::move(options.archives), std::move(options.decryptor), std::move(codecs), stop);
    if (!set) return std::unexpected(set.error());
    FileSystem files(std::move(*set));
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "mount");
    Result<ScriptObjects> scripts = readScriptObjects(files, stop);
    if (!scripts) return std::unexpected(scripts.error());
    Result<PackageStore> store = readPackageStore(files, stop);
    if (!store) return std::unexpected(store.error());
    return MountedGame{std::move(files), std::move(mappings), std::move(*scripts), std::move(*store), options.game};
}

FileProvider::FileProvider(MountedGame game) : game(std::move(game)), cache(std::make_unique<Cache>()) {
    cache->packages = std::make_unique<PackageSlot[]>(this->game.packageStore.getEntryCount());
}

FileProvider::~FileProvider() = default;

Result<const Package*> FileProvider::loadPackage(std::string_view path, std::stop_token stop) const {
    return loadPackage(FPackageId::fromName(getPackageName(path)), stop);
}

Result<const Package*> FileProvider::loadPackage(FPackageId id, std::stop_token stop) const {
    // The stop token is checked before a load starts. A started load runs to the end, because every caller gets its
    // result; it reads only the package header.
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "FileProvider.loadPackage");
    std::optional<ChunkLocation> location = game.files.findChunk(FIoChunkId::create(id.value, 0, EIoChunkType::ExportBundleData));
    if (!location) return fail(ErrorCode::MissingPackage, "FileProvider.loadPackage");
    // As in UE, a package loads only through its container's package store entry.
    std::optional<std::uint32_t> index = game.packageStore.findIndex(location->archive, id);
    if (!index) return fail(ErrorCode::MissingPackage, "FileProvider.loadPackage.store");
    PackageSlot& slot = cache->packages[*index];
    tbb::collaborative_call_once(slot.once, [&] { slot.result = readPackage(id, *location, game.packageStore.getEntry(*index)); });
    if (!slot.result) return std::unexpected(slot.result.error());
    return slot.result->get();
}

Result<std::unique_ptr<Package>> FileProvider::readPackage(FPackageId id, ChunkLocation location, const PackageStoreEntry& entry) const {
    std::uint64_t size = game.files.getChunkSize(location);
    // The header is nearly always in the first block; read more only when it is larger.
    std::vector<std::byte> bytes(size < 65536 ? size : 65536);
    Result<void> read = game.files.readChunk(location, 0, bytes);
    if (!read) return std::unexpected(read.error());
    std::uint32_t headerSize = peekZenHeaderSize(bytes);
    if (headerSize == 0 || headerSize > size) return fail(ErrorCode::InvalidData, "FileProvider.loadPackage.headerSize");
    if (headerSize > bytes.size()) {
        bytes.resize(headerSize);
        read = game.files.readChunk(location, 0, bytes);
        if (!read) return std::unexpected(read.error());
    }
    bytes.resize(headerSize);
    Result<ZenPackageHeader> header = parseZenPackageHeader(std::move(bytes), game.game, &entry);
    if (!header) return std::unexpected(header.error());
    return std::make_unique<Package>(*this, location, id, std::move(*header));
}

Result<std::vector<std::byte>> FileProvider::readFile(std::string_view path, std::stop_token stop) const {
    std::optional<FileEntry> file = game.files.findFile(path);
    if (!file) return fail(ErrorCode::MissingFile, "FileProvider.readFile");
    std::vector<std::byte> bytes(game.files.getFileSize(*file));
    Result<void> read = game.files.readFile(*file, 0, bytes, stop);
    if (!read) return std::unexpected(read.error());
    return bytes;
}

}
