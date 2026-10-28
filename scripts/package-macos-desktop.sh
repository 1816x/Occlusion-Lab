#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" || "$#" != 2 ]]; then
  echo "Usage (macOS): $0 <built .app> <output directory>" >&2
  exit 2
fi

source_app="$1"
output_directory="$2"
app="$output_directory/Occlusion Lab.app"

if [[ ! -x "$source_app/Contents/MacOS/occlusion-desktop" ]]; then
  echo "Built desktop executable is missing: $source_app" >&2
  exit 2
fi

mkdir -p "$output_directory"
ditto "$source_app" "$app"
"$(brew --prefix qtbase)/bin/macdeployqt" "$app" -verbose=1
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
  if file -b "$item" | grep -q 'Mach-O'; then
    codesign --force --sign - "$item"
  fi
done < <(find "$app" -type f -print0)
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict --verbose=2 "$app"
ditto -c -k --keepParent "$app" "$output_directory/occlusion-lab-macos.zip"
echo "Packaged $output_directory/occlusion-lab-macos.zip"
