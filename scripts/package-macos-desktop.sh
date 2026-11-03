#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" || "$#" != 2 ]]; then
  echo "Usage (macOS): $0 <built .app> <output directory>" >&2
  exit 2
fi

source_app="$1"
output_directory="$2"

if [[ ! -x "$source_app/Contents/MacOS/occlusion-desktop" ]]; then
  echo "Built desktop executable is missing: $source_app" >&2
  exit 2
fi

mkdir -p "$output_directory"
output_directory="$(cd "$output_directory" && pwd)"
app="$output_directory/Occlusion Lab.app"
ditto "$source_app" "$app"
"$(brew --prefix qtbase)/bin/macdeployqt" "$app" -verbose=1
rm -f "$app/Contents/PlugIns/platforms/libqoffscreen.dylib"
python3 "$(dirname "$0")/relocate-macos-libraries.py" "$app"

# macdeployqt copies native libraries but can retain build-machine search paths.
# Remove those paths before signing and checking the relocated bundle.
while IFS= read -r -d '' item; do
  if ! file -b "$item" | grep -q 'Mach-O'; then
    continue
  fi
  while IFS= read -r dependency; do
    case "$dependency" in
      /opt/homebrew/*|/usr/local/*)
        echo "Unbundled Homebrew dependency in $item: $dependency" >&2
        exit 1
        ;;
    esac
  done < <(otool -L "$item" | awk 'NR > 1 { print $1 }')
  while IFS= read -r search_path; do
    case "$search_path" in
      /opt/homebrew/*|/usr/local/*)
        install_name_tool -delete_rpath "$search_path" "$item"
        ;;
    esac
  done < <(otool -l "$item" | awk '
    $1 == "cmd" && $2 == "LC_RPATH" { rpath = 1; next }
    rpath && $1 == "path" { print $2; rpath = 0 }
  ')
done < <(find "$app" -type f -print0)

while IFS= read -r -d '' item; do
  if [[ "$item" == *.dylib || "$item" == *.so ]] || file -b "$item" | grep -q 'Mach-O'; then
    codesign --force --sign - "$item"
  fi
done < <(find "$app" -type f -print0)
while IFS= read -r -d '' item; do
  resolved="$(realpath "$item")"
  if [[ "$resolved" != "$app"/* ]]; then
    echo "Bundled library resolves outside the app: $item -> $resolved" >&2
    exit 1
  fi
  codesign --force --sign - "$item"
done < <(find "$app" \( -type f -o -type l \) -name '*.dylib' -print0)
vtk_signature_probe=""
for candidate in "$app/Contents/Frameworks"/libvtkFiltersHybrid*.dylib; do
  if [[ -e "$candidate" ]]; then
    vtk_signature_probe="$candidate"
    break
  fi
done
if [[ -n "$vtk_signature_probe" ]]; then
  echo "VTK_SIGNATURE_BEFORE $(shasum -a 256 "$vtk_signature_probe")"
  echo "VTK_SIGNATURE_BEFORE $(codesign --display --verbose=2 "$vtk_signature_probe" 2>&1 | tail -n 2 | tr '\n' ' ')"
fi
codesign --force --deep --sign - "$app"
if [[ -n "$vtk_signature_probe" ]]; then
  echo "VTK_SIGNATURE_AFTER $(shasum -a 256 "$vtk_signature_probe")"
  echo "VTK_SIGNATURE_AFTER $(codesign --display --verbose=2 "$vtk_signature_probe" 2>&1 | tail -n 2 | tr '\n' ' ')"
fi
while IFS= read -r -d '' item; do
  codesign --verify --strict --all-architectures --verbose=2 "$item"
done < <(find "$app" \( -type f -o -type l \) -name '*.dylib' -print0)
codesign --verify --deep --strict --verbose=2 "$app"
ditto -c -k --keepParent "$app" "$output_directory/occlusion-lab-macos.zip"
repository_root="$(cd "$(dirname "$0")/.." && pwd)"
ditto "$repository_root/fixtures/desktop-preview/fixed-box.obj" "$output_directory/fixed-box.obj"
ditto "$repository_root/fixtures/desktop-preview/moving-box.obj" "$output_directory/moving-box.obj"
cat > "$output_directory/README.txt" <<'EOF'
Occlusion Lab macOS development preview

1. Unzip occlusion-lab-macos.zip to get Occlusion Lab.app.
2. Open the app. If macOS blocks this unnotarized preview, use Finder's
   right-click > Open or Privacy & Security > Open Anyway.
3. In the app, select this folder (containing fixed-box.obj and moving-box.obj)
   as the directory, then click Import OBJ pair.
4. Try Evaluate pose, Run sweep, the frame slider, and Export JSON v1.

This preview works with synthetic, closed OBJ meshes only. It has no clinical
validation. Please report your Mac model, macOS version, and any launch or
rendering problem.
EOF
echo "Packaged $output_directory/occlusion-lab-macos.zip"
