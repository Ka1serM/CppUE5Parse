#!/usr/bin/env python3
import argparse
import collections
import os
import re
import subprocess
import xml.etree.ElementTree as ElementTree


def main():
    parser = argparse.ArgumentParser(description="Runs the asset tests and prints the versions each check fails on. The JUnit results stay in <build>/asset-tests.xml.")
    parser.add_argument("build")
    parser.add_argument("regex", nargs="?", default="")
    parser.add_argument("--cook", default="iostore|pak|inline")
    parser.add_argument("--prefix", default="", help="the cook folder's test prefix: '' (D3D) or vulkan/")
    args = parser.parse_args()
    junit = os.path.join(os.path.abspath(args.build), "asset-tests.xml")
    subprocess.run(["ctest", "-j", str(os.cpu_count()), "-Q", "-R", args.regex, "--output-junit", junit], cwd=args.build, check=False)
    name = re.compile(r"^%s(\d+\.\d+\.\d+)/(%s)/(.+)$" % (re.escape(args.prefix), args.cook))
    results = collections.defaultdict(dict)
    for case in ElementTree.parse(junit).getroot().iter("testcase"):
        match = name.match(case.get("name"))
        if match:
            version, cook, check = match.groups()
            results[check][version + ("" if "|" not in args.cook else "/" + cook)] = case.get("status") == "run" and case.find("failure") is None

    def key(version):
        return [int(n) if n.isdigit() else n for n in re.split(r"[./]", version)]

    width = max((len(c) for c in results), default=0)
    passed = sum(ok for r in results.values() for ok in r.values())
    total = sum(len(r) for r in results.values())
    for check in sorted(results, key=lambda c: (not c.startswith(("mount", "files", "packages", "objects", "registry")), c)):
        failed = sorted((v for v, ok in results[check].items() if not ok), key=key)
        line = "FAIL " + " ".join(failed) if failed else "ok   (%d cooks)" % len(results[check])
        print("%-*s  %s" % (width, check, line))
    print("%d of %d tests passed" % (passed, total))


if __name__ == "__main__":
    main()
