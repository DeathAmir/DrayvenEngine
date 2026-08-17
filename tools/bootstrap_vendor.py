#!/usr/bin/env python3
import argparse
import pathlib
import shutil
import subprocess
import sys

COCOS_URL = "https://github.com/cocos2d/cocos2d-x.git"
COCOS_SHA = "7a5282a301a13e467bdcc3466d3cd3883862aadf"
FAIRYGUI_URL = "https://github.com/fairygui/FairyGUI-cocos2dx.git"
FAIRYGUI_SHA = "94fa2231db6814ce26631823ea6f2fafc442eb0c"


def run(args, cwd=None):
    print("+", " ".join(str(x) for x in args), flush=True)
    subprocess.check_call([str(x) for x in args], cwd=cwd)


def checkout(url, sha, dst, clean=False):
    if clean and dst.exists():
        shutil.rmtree(dst)
    if not (dst / ".git").exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        run(["git", "clone", "--filter=blob:none", "--no-checkout", url, dst])
    run(["git", "fetch", "--depth", "1", "origin", sha], cwd=dst)
    run(["git", "checkout", "--detach", "FETCH_HEAD"], cwd=dst)


def main():
    parser = argparse.ArgumentParser(description="Fetch pinned Drayven third-party engine sources")
    parser.add_argument("--root", default="vendor")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--skip-cocos-deps", action="store_true")
    args = parser.parse_args()

    root = pathlib.Path(args.root).resolve()
    cocos = root / "cocos2d-x"
    fairygui = root / "FairyGUI-cocos2dx"

    checkout(COCOS_URL, COCOS_SHA, cocos, args.clean)
    checkout(FAIRYGUI_URL, FAIRYGUI_SHA, fairygui, args.clean)

    if not args.skip_cocos_deps:
        # Cocos2d-x v4's legacy downloader prompts after extraction unless
        # --remove-download is explicitly supplied. Keep CI deterministic.
        run([sys.executable, "download-deps.py", "--remove-download", "yes"], cwd=cocos)

    marker = root / ".drayven-vendor-lock"
    marker.write_text(
        f"cocos2d-x={COCOS_SHA}\nFairyGUI-cocos2dx={FAIRYGUI_SHA}\n",
        encoding="utf-8",
    )
    print(f"Vendor sources ready in {root}")


if __name__ == "__main__":
    main()
