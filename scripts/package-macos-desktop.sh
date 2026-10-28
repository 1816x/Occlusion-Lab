#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" || "$#" != 2 ]]; then
  echo "Usage (macOS): $0 <built .app> <output directory>" >&2
  exit 2
fi

source_app="$1"
output_directory="$2"
app="$output_directory/Occlusion Lab.app"
executable="$app/Contents/MacOS/occlusion-desktop"

if [[ ! -x "$source_app/Contents/MacOS/occlusion-desktop" ]]; then
  echo "Built desktop executable is missing: $source_app" >&2
  exit 2
fi

mkdir -p "$output_directory"
ditto "$source_app" "$app"
"$(brew --prefix qtbase)/bin/macdeployqt" "$app" -verbose=1
dylibbundler -cd -b -ns -x "$executable" \
  -d "$app/Contents/Frameworks" -p '@executable_path/../Frameworks/'

# Check every bundled Mach-O object, including Qt plugins and copied libraries.
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
done < <(find "$app" -type f -print0)

codesign --force --deep --sign - "$app"
ditto -c -k --keepParent "$app" "$output_directory/occlusion-lab-macos.zip"
echo "Packaged $output_directory/occlusion-lab-macos.zip"
