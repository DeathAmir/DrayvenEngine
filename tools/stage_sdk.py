#!/usr/bin/env python3
import argparse
import pathlib
import shutil

BINARY_EXTENSIONS = {".a", ".lib", ".so", ".dll", ".exe", ".dylib"}
HEADER_EXTENSIONS = {".h", ".hpp", ".hh", ".inl"}


def copy_file(src: pathlib.Path, dst: pathlib.Path):
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def copy_tree_filtered(src: pathlib.Path, dst: pathlib.Path, extensions):
    if not src.exists():
        return
    for path in src.rglob("*"):
        if path.is_file() and path.suffix.lower() in extensions:
            copy_file(path, dst / path.relative_to(src))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--platform", required=True)
    args = parser.parse_args()

    root = pathlib.Path(__file__).resolve().parents[1]
    build = pathlib.Path(args.build).resolve()
    out = pathlib.Path(args.output).resolve()
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    copy_tree_filtered(root / "include", out / "include", HEADER_EXTENSIONS)
    copy_tree_filtered(root / "vendor" / "FairyGUI-cocos2dx" / "libfairygui" / "Classes", out / "third_party" / "fairygui" / "include", HEADER_EXTENSIONS)
    copy_tree_filtered(root / "vendor" / "cocos2d-x" / "cocos", out / "third_party" / "cocos2d-x" / "include" / "cocos", HEADER_EXTENSIONS)
    copy_tree_filtered(root / "vendor" / "cocos2d-x" / "external", out / "third_party" / "cocos2d-x" / "include" / "external", HEADER_EXTENSIONS)

    for path in build.rglob("*"):
        if path.is_file() and path.suffix.lower() in BINARY_EXTENSIONS:
            copy_file(path, out / "libraries" / path.relative_to(build))

    for name in ["LICENSE", "README.md", "THIRD_PARTY_NOTICES.md"]:
        src = root / name
        if src.exists():
            copy_file(src, out / name)

    (out / "SDK_INFO.txt").write_text(
        "DrayvenEngine 0.3.0\n"
        f"platform={args.platform}\n"
        "engine=Cocos2d-x v4\n"
        "ui=FairyGUI-cocos2dx\n"
        "script=DRYS\n"
        "noesis=not redistributed; optional user-supplied licensed SDK only\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
