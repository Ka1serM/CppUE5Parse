"""egs_restore.py <install dir> <since YYYY-MM-DD[THH:MM] | auto> [--dry-run]

Restores the files of an Epic launcher engine install that were modified after <since> (e.g. engine packages an editor
resaved) to their launcher build; "auto" takes the end of the install (the newest non-package file, + 1 min). Reads the install's manifests (<install>\\.egstore\\*.manifest, binary or JSON), checks the
SHA1 of every file written after <since>, and rebuilds each mismatching one from its chunks (downloaded from the CDN
into <install>\\.egstore\\restore_chunks, removed afterwards; run one at a time per install). Prints one line per
restored file. The binary (UE 5.1+) builds' chunks need signed CDN URLs (HTTP 403): repair those with the launcher's
Install API instead ({"type": "Repair", ...})."""
import datetime
import glob
import hashlib
import io
import json
import os
import struct
import sys
import urllib.request
import zlib

BASES = ["https://egdownload.fastly-edge.com/Builds/Rocket/Automated/Engine/CloudDir",
         "https://epicgames-download1.akamaized.net/Builds/Rocket/Automated/Engine/CloudDir",
         "https://egs-cloudfront-chunks.epicgamescdn.com/Builds/Rocket/Automated/Engine/CloudDir"]


class Reader:
    def __init__(self, data):
        self.f = io.BytesIO(data)

    def u8(self):
        return self.f.read(1)[0]

    def u32(self):
        return struct.unpack("<I", self.f.read(4))[0]

    def i32(self):
        return struct.unpack("<i", self.f.read(4))[0]

    def u64(self):
        return struct.unpack("<Q", self.f.read(8))[0]

    def raw(self, n):
        return self.f.read(n)

    def string(self):
        n = self.i32()
        if n == 0:
            return ""
        if n < 0:
            return self.f.read(-n * 2).decode("utf-16-le").rstrip("\0")
        return self.f.read(n).decode("latin-1").rstrip("\0")

    def guid(self):
        return "%08X%08X%08X%08X" % struct.unpack("<IIII", self.f.read(16))

    def tell(self):
        return self.f.tell()

    def seek(self, pos):
        self.f.seek(pos)


def blob_int(text):  # the JSON manifests' numbers: little-endian bytes as 3-digit decimal groups
    return int.from_bytes(bytes(int(text[i:i + 3]) for i in range(0, len(text), 3)), "little")


def chunk_dir(version):
    return "ChunksV4" if version >= 15 else "ChunksV3" if version >= 6 else "ChunksV2" if version >= 3 else "Chunks"


def parse_json_manifest(blob):
    m = json.loads(blob.decode("utf-8-sig"))
    folder = chunk_dir(blob_int(m["ManifestFileVersion"]))
    chunks = {g: (blob_int(h), blob_int(m["DataGroupList"][g]), 0, folder)
              for g, h in m["ChunkHashList"].items()}
    files = {}
    for f in m["FileManifestList"]:
        parts = [(p["Guid"], blob_int(p["Offset"]), blob_int(p["Size"])) for p in f["FileChunkParts"]]
        files[f["Filename"]] = (bytes(int(f["FileHash"][i:i + 3]) for i in range(0, len(f["FileHash"]), 3)), parts)
    return chunks, files


def parse_manifest(path):
    blob = open(path, "rb").read()
    if blob.lstrip()[:1] == b"{":
        return parse_json_manifest(blob)
    magic, header_size, size, compressed_size = struct.unpack_from("<IIII", blob, 0)
    if magic != 0x44BEC00C:
        raise ValueError("not a binary manifest: %s" % path)
    stored_as = blob[36]
    data = blob[header_size:header_size + compressed_size]
    if stored_as & 1:
        data = zlib.decompress(data)
    r = Reader(data)
    # FManifestMeta: skipped by its size
    start = r.tell()
    r.seek(start + r.u32())
    # FChunkDataList: columns of Count entries
    start = r.tell()
    end = start + r.u32()
    r.u8()
    count = r.i32()
    guids = [r.guid() for _ in range(count)]
    hashes = [r.u64() for _ in range(count)]
    [r.raw(20) for _ in range(count)]  # chunk SHA1s
    groups = [r.u8() for _ in range(count)]
    [r.u32() for _ in range(count)]  # window sizes
    sizes = [struct.unpack("<q", r.raw(8))[0] for _ in range(count)]
    chunks = {g: (h, grp, s, "ChunksV4") for g, h, grp, s in zip(guids, hashes, groups, sizes)}
    r.seek(end)
    # FFileManifestList
    start = r.tell()
    r.u32()
    r.u8()
    count = r.i32()
    names = [r.string() for _ in range(count)]
    [r.string() for _ in range(count)]  # symlink targets
    file_hashes = [r.raw(20) for _ in range(count)]
    [r.u8() for _ in range(count)]  # meta flags
    for _ in range(count):  # install tags
        [r.string() for _ in range(r.i32())]
    parts = []
    for _ in range(count):
        file_parts = []
        for _ in range(r.i32()):
            part = r.tell()
            part_size = r.u32()
            file_parts.append((r.guid(), r.u32(), r.u32()))
            r.seek(part + part_size)
        parts.append(file_parts)
    return chunks, {n: (h, p) for n, h, p in zip(names, file_hashes, parts)}


def parse_chunk(raw):
    magic, _version, header_size, _compressed = struct.unpack_from("<IIII", raw, 0)
    if magic != 0xB1FE3AA2:
        raise ValueError("bad chunk magic")
    data = raw[header_size:]
    return zlib.decompress(data) if raw[40] & 1 else data


def sha1(path):
    h = hashlib.sha1()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.digest()


def main():
    install = sys.argv[1]
    dry = "--dry-run" in sys.argv
    # every manifest of the install, oldest first: a tag change (e.g. adding "templates") adds a small one
    manifests = sorted(glob.glob(os.path.join(install, ".egstore", "*.manifest")), key=os.path.getmtime)
    if not manifests:
        print("%s: no launcher manifest (installed by egs_fetch.py?), skipped" % install, flush=True)
        return 0
    chunks, files = {}, {}
    for manifest in manifests:
        more_chunks, more_files = parse_manifest(manifest)
        chunks.update(more_chunks)
        files.update(more_files)
    if sys.argv[2] == "auto":
        # the end of the install: the newest of its files that no editor writes (everything but packages)
        since = max(os.path.getmtime(p) for p in (os.path.join(install, n.replace("/", os.sep)) for n in files
                                                  if not n.endswith((".uasset", ".umap"))) if os.path.exists(p)) + 60
        print("since %s" % datetime.datetime.fromtimestamp(since).isoformat(timespec="minutes"), flush=True)
    else:
        since = datetime.datetime.fromisoformat(sys.argv[2]).timestamp()
    changed = []
    for name, (expected, parts) in files.items():
        path = os.path.join(install, name.replace("/", os.sep))
        if os.path.exists(path) and os.path.getmtime(path) > since and sha1(path) != expected:
            changed.append((name, path, expected, parts))
    print("%s: %d of %d files changed after %s" % (install, len(changed), len(files), sys.argv[2]), flush=True)
    if dry or not changed:
        for name, *_ in changed:
            print("  " + name)
        return 0
    cache = os.path.join(install, ".egstore", "restore_chunks")
    os.makedirs(cache, exist_ok=True)
    loaded = {}

    def chunk(guid):
        if guid not in loaded:
            path = os.path.join(cache, guid + ".chunk")
            if not os.path.exists(path):
                rolling, group, _size, folder = chunks[guid]
                err = None
                for base in BASES * 3:
                    try:
                        url = "%s/%s/%02d/%016X_%s.chunk" % (base, folder, group, rolling, guid)
                        with urllib.request.urlopen(url, timeout=60) as response:
                            raw = response.read()
                        parse_chunk(raw)
                        open(path, "wb").write(raw)
                        break
                    except Exception as e:  # noqa: BLE001
                        err = e
                else:
                    raise RuntimeError("chunk %s: %s" % (guid, err))
            loaded[guid] = parse_chunk(open(path, "rb").read())
        return loaded[guid]

    bad = 0
    for name, path, expected, parts in changed:
        data = b"".join(chunk(g)[offset:offset + size] for g, offset, size in parts)
        if hashlib.sha1(data).digest() != expected:
            print("  HASH MISMATCH after rebuild: " + name, flush=True)
            bad += 1
            continue
        readonly = not os.access(path, os.W_OK)
        if readonly:
            os.chmod(path, 0o666)
        with open(path, "wb") as f:
            f.write(data)
        print("  restored " + name, flush=True)
    for f in glob.glob(os.path.join(cache, "*.chunk")):
        os.remove(f)
    os.rmdir(cache)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
