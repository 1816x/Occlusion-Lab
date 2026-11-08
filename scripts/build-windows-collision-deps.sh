#!/usr/bin/env bash
# Run in an MSYS2 UCRT64 shell after installing the desktop toolchain.
set -euo pipefail
: "${MINGW_PREFIX:?Use the MSYS2 UCRT64 shell}"
root="$(pwd)/build/windows-deps"
mkdir -p "$root"
fetch_source() {
  local name="$1" url="$2" checksum="$3"
  curl --fail --location --retry 3 "$url" -o "$root/$name.tar.gz"
  echo "$checksum  $root/$name.tar.gz" | sha256sum --check
  mkdir -p "$root/$name"
  tar -xzf "$root/$name.tar.gz" --strip-components=1 -C "$root/$name"
}
fetch_source libccd https://github.com/danfis/libccd/archive/refs/tags/v2.1.tar.gz 542b6c47f522d581fbf39e51df32c7d1256ac0c626e7c2b41f1040d4b9d50d1e
fetch_source fcl https://github.com/flexible-collision-library/fcl/archive/refs/tags/0.7.0.tar.gz 90409e940b24045987506a6b239424a4222e2daf648c86dd146cbcb692ebdcbc
cmake -S "$root/libccd" -B "$root/libccd-build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_SHARED_LIBS=ON \
  -DENABLE_DOUBLE_PRECISION=ON -DBUILD_TESTING=OFF
cmake --build "$root/libccd-build" --parallel 2
cmake --install "$root/libccd-build"
cmake -S "$root/fcl" -B "$root/fcl-build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$MINGW_PREFIX" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_SHARED_LIBS=ON \
  -DFCL_WITH_OCTOMAP=OFF -DFCL_BUILD_TESTS=OFF -DFCL_BUILD_EXAMPLES=OFF
cmake --build "$root/fcl-build" --parallel 2
cmake --install "$root/fcl-build"
mkdir -p "$MINGW_PREFIX/share/licenses/occlusion-fcl" "$MINGW_PREFIX/share/licenses/occlusion-libccd"
cp "$root/fcl/LICENSE" "$MINGW_PREFIX/share/licenses/occlusion-fcl/"
cp "$root/libccd/BSD-LICENSE" "$MINGW_PREFIX/share/licenses/occlusion-libccd/"
