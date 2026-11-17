#include "CppUE5Parse/Package/Package.h"

#include <ankerl/unordered_dense.h>
#include <oneapi/tbb/collaborative_call_once.h>

namespace ue5parse {

namespace {

char toLowerAscii(char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

struct IdHash {
    using is_avalanching = void;
    std::uint64_t operator()(std::uint64_t value) const noexcept { return value; }
};

void appendLowerAscii(std::string_view text, std::string& out) {
    for (char c : text)
        out.push_back(toLowerAscii(c));
}

std::uint64_t hashLowerPath(std::string_view lowerPath) {
    return ankerl::unordered_dense::hash<std::string_view>{}(lowerPath);
}

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); i++)
        if (toLowerAscii(a[i]) != toLowerAscii(b[i]))
            return false;
    return true;
}

}

struct Package::PathLookup {
    tbb::collaborative_once_flag once;
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, IdHash> byPath;
};

Package::Package(const FileProvider& provider, PackageHeader header)
    : provider(&provider), header(std::move(header)), pathLookup(std::make_unique<PathLookup>()) {}

Package::~Package() = default;

void Package::appendExportPathName(std::size_t index, std::string& out) const {
    std::size_t chain[64];
    std::size_t depth = 0;
    std::size_t current = index;
    for (;;) {
        chain[depth++] = current;
        const std::int32_t outer = header.exports[current].outerIndex;
        if (outer <= 0 || static_cast<std::size_t>(outer) > getExportCount() || depth == 64)
            break;
        current = static_cast<std::size_t>(outer - 1);
    }
    getName().appendTo(out);
    out.push_back('.');
    for (std::size_t i = depth; i-- > 0;) {
        getExportName(chain[i]).appendTo(out);
        if (i > 0)
            out.push_back(i == depth - 1 ? ':' : '.');
    }
}

std::string Package::getExportPathName(std::size_t index) const {
    std::string out;
    appendExportPathName(index, out);
    return out;
}

const Package::PathLookup& Package::getPathLookup() const {
    tbb::collaborative_call_once(pathLookup->once, [&] {
        pathLookup->byPath.reserve(getExportCount());
        std::string path;
        std::string lowerPath;
        for (std::uint32_t i = 0; i < getExportCount(); i++) {
            path.clear();
            appendExportPathName(i, path);
            lowerPath.clear();
            appendLowerAscii(path, lowerPath);
            pathLookup->byPath.try_emplace(hashLowerPath(lowerPath), i);
        }
    });
    return *pathLookup;
}

std::int64_t Package::findExportByPath(std::string_view objectPath) const {
    std::string lowerPath;
    appendLowerAscii(getName().toString(), lowerPath);
    lowerPath.push_back('.');
    appendLowerAscii(objectPath, lowerPath);
    const PathLookup& lookup = getPathLookup();
    auto found = lookup.byPath.find(hashLowerPath(lowerPath));
    if (found == lookup.byPath.end())
        return -1;
    if (!equalsIgnoreCase(getExportPathName(found->second), lowerPath))
        return -1;
    return found->second;
}

Result<std::vector<std::byte>> Package::readExportBytes(std::size_t index, std::stop_token stop) const {
    if (index >= getExportCount())
        return fail(ErrorCode::MissingExport, "Package.readExportBytes");
    if (stop.stop_requested())
        return fail(ErrorCode::Cancelled, "Package.readExportBytes");
    const PackageExport& entry = header.exports[index];
    std::uint64_t dataSize = getExportDataSize();
    if (entry.dataOffset > dataSize || entry.serialSize > dataSize - entry.dataOffset)
        return fail(ErrorCode::InvalidData, "Package.readExportBytes.range");
    std::vector<std::byte> bytes(entry.serialSize);
    Result<void> read = readExportData(entry.dataOffset, bytes, stop);
    if (!read)
        return std::unexpected(read.error());
    return bytes;
}

}
