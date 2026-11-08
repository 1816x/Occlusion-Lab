#!/usr/bin/env python3
"""Bundle the UCRT64 desktop viewer and audit its transitive PE imports."""

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess


def imported_dlls(binary, objdump):
    result = subprocess.run([str(objdump), '-p', str(binary)], check=True,
                            capture_output=True, text=True)
    if 'file format pei-x86-64' not in result.stdout:
        raise RuntimeError(f'Expected Windows x64 binary: {binary}')
    return re.findall(r'DLL Name:\s*(\S+)', result.stdout)


def bundle_dependencies(bundle, source_bin, system_bin, scan):
    """Resolve each import from the bundle, toolchain, or Windows system files."""
    available = {p.name.lower(): p for p in source_bin.glob('*.dll')}
    system = {p.name.lower() for p in system_bin.glob('*.dll')}
    pending = list(bundle.rglob('*.dll')) + list(bundle.glob('*.exe'))
    visited = set()
    manifest = {}
    while pending:
        binary = pending.pop()
        if binary in visited:
            continue
        visited.add(binary)
        dependencies = scan(binary)
        manifest[str(binary.relative_to(bundle))] = dependencies
        for name in dependencies:
            key = name.lower()
            local = bundle / name
            if local.is_file():
                pending.append(local)
            elif key in available:
                shutil.copy2(available[key], local)
                pending.append(local)
            elif key in system or key.startswith(('api-ms-win-', 'ext-ms-win-')):
                continue
            else:
                raise RuntimeError(f'Missing DLL {name} required by {binary.name}')
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('prefix', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--collision-prefix', type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('Packaging requires Windows and the UCRT64 toolchain')
    bundle = args.output.resolve()
    bundle.mkdir(parents=True, exist_ok=False)
    executable = bundle / 'occlusion-desktop.exe'
    shutil.copy2(args.executable, executable)
    source_bin = args.prefix / 'bin'
    for binary in (args.collision_prefix / 'bin').glob('*.dll'):
        shutil.copy2(binary, bundle / binary.name)
    subprocess.run([str(source_bin / 'windeployqt6.exe') if (source_bin / 'windeployqt6.exe').exists()
                    else str(source_bin / 'windeployqt.exe'), '--release', '--no-translations',
                    '--no-system-d3d-compiler', '--no-opengl-sw', str(executable)], check=True)
    # Supply software OpenGL for machines/runners without a usable graphics driver.
    shutil.copy2(source_bin / 'opengl32.dll', bundle / 'opengl32.dll')
    (bundle / 'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=.\n', encoding='utf-8')
    if not (bundle / 'platforms/qwindows.dll').is_file():
        raise RuntimeError('Qt Windows platform plugin was not deployed')
    manifest = bundle_dependencies(bundle, source_bin, Path(os.environ['SystemRoot']) / 'System32',
                                   lambda binary: imported_dlls(binary, source_bin / 'objdump.exe'))
    (bundle / 'dependency-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    repo = Path(__file__).resolve().parents[1]
    for name in ('fixed-box.obj', 'moving-box.obj'):
        shutil.copy2(repo / 'fixtures/desktop-preview' / name, bundle / name)
    shutil.copytree(args.prefix / 'share/licenses', bundle / 'third-party-licenses')
    shutil.copytree(args.collision_prefix / 'share/licenses', bundle / 'third-party-licenses',
                    dirs_exist_ok=True)
    shutil.copy2(repo / 'LICENSE', bundle / 'LICENSE.txt')
    (bundle / 'README.txt').write_text('''Occlusion Lab Windows x64 beta preview

Extract the whole ZIP before launching occlusion-desktop.exe.
Keep the executable, DLLs, and platforms folder together. No Qt or MSYS2 installation is needed.
Select this extracted folder in the app, then click Import OBJ pair to load the sample meshes.
Try Evaluate pose, Run sweep, the frame slider, camera controls, and Export JSON v1.

This unsigned development preview is for Windows 10/11 x64 and synthetic meshes only.
If Windows displays an unknown-publisher warning, verify that you downloaded this artifact
from the project's GitHub Actions run before choosing to run it.
It has no clinical validation. Report your Windows version, graphics hardware, and any
launch, import, render, sweep, or export issue. See the Windows beta checklist in the repository.
''', encoding='utf-8')
    print(f'Packaged {bundle}: audited {len(manifest)} PE binaries')


if __name__ == '__main__':
    main()
