"""egs_fetch.py <manifest.json> <install dir> <chunk cache dir> [threads]

Install an old (JSON manifest) Epic launcher build straight from the public CDN, for the UE 4.3-4.8 manifests the
new launcher rejects (their ChunkShaList is all zeros). Every assembled file is verified against its FileHash
(SHA1). Skips debug symbols, samples, docs and non-Win64 binaries: enough to build C++ projects and run the editor."""
import concurrent.futures as cf
import hashlib
import json
import os
import re
import struct
import sys
import threading
import time
import urllib.request
import zlib

BASES = ["https://egdownload.fastly-edge.com/Builds/Rocket/Automated/Engine/CloudDir",
         "https://epicgames-download1.akamaized.net/Builds/Rocket/Automated/Engine/CloudDir",
         "https://egs-cloudfront-chunks.epicgamescdn.com/Builds/Rocket/Automated/Engine/CloudDir"]
# Engine/Source stays whole: UBT reads every platform module's rules, even for a Win64-only build
SKIP = re.compile(r"(\.pdb$|^Samples/|^FeaturePacks/|^Engine/Documentation/|^Engine/Extras/(?!Redist/)"
                  r"|/Binaries/(.*/)?(Android|IOS|HTML5|Linux|Mac|Win32|PS4|XboxOne|WinRT|TVOS)/)", re.I)


def blob(s):
    return int.from_bytes(bytes(int(s[i:i + 3]) for i in range(0, len(s), 3)), "little")


def blob_bytes(s):
    return bytes(int(s[i:i + 3]) for i in range(0, len(s), 3))


def chunk_dir(version):
    return "ChunksV4" if version >= 15 else "ChunksV3" if version >= 6 else "ChunksV2" if version >= 3 else "Chunks"


def parse_chunk(raw):
    magic, version, header_size, _compressed = struct.unpack_from("<IIII", raw, 0)
    if magic != 0xB1FE3AA2:
        raise ValueError("bad chunk magic")
    stored_as = raw[40]
    data = raw[header_size:]
    return zlib.decompress(data) if stored_as & 1 else data


def main():
    manifest, dest, cache = sys.argv[1], sys.argv[2], sys.argv[3]
    threads = int(sys.argv[4]) if len(sys.argv) > 4 else 16
    m = json.load(open(manifest, encoding="utf-8"))
    cdir = chunk_dir(blob(m["ManifestFileVersion"]))
    files = [f for f in m["FileManifestList"] if not SKIP.search(f["Filename"])]

    def present(f):  # assembled (and verified) by an earlier run
        out = os.path.join(dest, f["Filename"].replace("/", os.sep))
        return os.path.exists(out) and os.path.getsize(out) == sum(blob(p["Size"]) for p in f["FileChunkParts"])

    files = [f for f in files if not present(f)]
    need = sorted({p["Guid"] for f in files for p in f["FileChunkParts"]})
    sizes = {g: blob(m["ChunkFilesizeList"][g]) for g in need}
    print(f"{len(files)} of {len(m['FileManifestList'])} files, {len(need)} chunks, "
          f"{sum(sizes.values()) / 1e9:.2f} GB to download", flush=True)
    os.makedirs(cache, exist_ok=True)

    def url(g, base):
        return f"{base}/{cdir}/{blob(m['DataGroupList'][g]):02d}/{blob(m['ChunkHashList'][g]):016X}_{g}.chunk"

    done, lock, t0 = [0, 0], threading.Lock(), time.time()

    def fetch(g):
        path = os.path.join(cache, g + ".chunk")
        if not (os.path.exists(path) and os.path.getsize(path) == sizes[g]):
            for attempt in range(12):
                base = BASES[attempt % len(BASES)]
                try:
                    with urllib.request.urlopen(url(g, base), timeout=60) as r:
                        raw = r.read()
                    parse_chunk(raw)  # must decompress
                    with open(path + ".part", "wb") as f:
                        f.write(raw)
                    os.replace(path + ".part", path)
                    break
                except Exception as e:  # noqa: BLE001
                    err = e
                    time.sleep(min(30, 2 ** attempt))
            else:
                raise RuntimeError(f"chunk {g}: {err}")
        with lock:
            done[0] += 1
            done[1] += sizes[g]
            if done[0] % 200 == 0:
                rate = done[1] / max(1, time.time() - t0) / 1e6
                print(f"  {done[0]}/{len(need)} chunks, {done[1] / 1e9:.2f} GB, {rate:.1f} MB/s", flush=True)

    with cf.ThreadPoolExecutor(threads) as ex:
        list(ex.map(fetch, need))

    cached = {}

    def chunk(g):
        if g not in cached:
            if len(cached) > 64:
                cached.clear()
            cached[g] = parse_chunk(open(os.path.join(cache, g + ".chunk"), "rb").read())
        return cached[g]

    bad = []
    for f in files:
        out = os.path.join(dest, f["Filename"].replace("/", os.sep))
        os.makedirs(os.path.dirname(out), exist_ok=True)
        h = hashlib.sha1()
        with open(out, "wb") as o:
            for p in f["FileChunkParts"]:
                off, size = blob(p["Offset"]), blob(p["Size"])
                piece = chunk(p["Guid"])[off:off + size]
                h.update(piece)
                o.write(piece)
        if h.digest() != blob_bytes(f["FileHash"]):
            bad.append(f["Filename"])
    print(f"assembled {len(files)} files, {len(bad)} hash mismatches", flush=True)
    for b in bad[:20]:
        print("  BAD", b)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
