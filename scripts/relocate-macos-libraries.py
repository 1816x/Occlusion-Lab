#!/usr/bin/env python3
"""Finish macdeployqt's relocation of non-Qt Homebrew libraries."""

from pathlib import Path
import shutil
import subprocess
import sys


def output(*command: str) -> str:
    return subprocess.run(command, check=True, capture_output=True, text=True).stdout


def is_homebrew(path: str) -> bool:
    return path.startswith(("/opt/homebrew/", "/usr/local/"))


def is_macho(path: Path) -> bool:
    return "Mach-O" in output("file", "-b", str(path))


def install_name(path: Path) -> str:
    result = subprocess.run(["otool", "-D", str(path)], capture_output=True, text=True)
    lines = result.stdout.splitlines()
    return lines[1].strip() if result.returncode == 0 and len(lines) > 1 else ""


def dependencies(path: Path) -> list[str]:
    lines = output("otool", "-L", str(path)).splitlines()[1:]
    return [line.strip().split(" (", 1)[0] for line in lines]


def relocate(app: Path) -> None:
    frameworks = app / "Contents" / "Frameworks"
    frameworks.mkdir(exist_ok=True)
    changes = 0
    for _ in range(64):
        pass_changes = 0
        for item in app.rglob("*"):
            if item.is_symlink() or not item.is_file() or not is_macho(item):
                continue
            identity = install_name(item)
            if is_homebrew(identity):
                subprocess.run(
                    ["install_name_tool", "-id",
                     f"@executable_path/../Frameworks/{item.name}", str(item)],
                    check=True,
                )
                pass_changes += 1
            for dependency in dependencies(item):
                if dependency == identity or not is_homebrew(dependency):
                    continue
                if ".framework/" in dependency:
                    raise RuntimeError(f"Qt framework was not deployed: {dependency}")
                destination = frameworks / Path(dependency).name
                if not destination.exists():
                    shutil.copy2(Path(dependency).resolve(strict=True), destination)
                subprocess.run(
                    ["install_name_tool", "-change", dependency,
                     f"@executable_path/../Frameworks/{destination.name}", str(item)],
                    check=True,
                )
                pass_changes += 1
        changes += pass_changes
        if pass_changes == 0:
            print(f"Relocated {changes} Homebrew install names and dependencies")
            return
    raise RuntimeError("Homebrew library relocation did not converge")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: relocate-macos-libraries.py <app bundle>")
    relocate(Path(sys.argv[1]))
