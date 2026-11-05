#!/usr/bin/env bash
set -euo pipefail
# Requires git, a C compiler, CMake >=3.25 and make/ninja.
# Installs SDL locally; never modifies system packages.
project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
deps_dir="${GAME_DEPS_DIR:-$project_dir/.deps}"
install_dir="$deps_dir/install"
mkdir -p "$deps_dir"
fetch_dependency() {
    local name="$1" repo="$2" tag="$3" revision="$4"
    if [[ ! -d "$deps_dir/$name/.git" ]]; then
        git clone --depth 1 --branch "$tag" "$repo" "$deps_dir/$name"
    fi
    [[ "$(git -C "$deps_dir/$name" rev-parse HEAD)" == "$revision" ]] || {
        echo "Unexpected revision for $name" >&2; exit 1;
    }
}
fetch_dependency SDL https://github.com/libsdl-org/SDL.git release-3.2.28 7f3ae3d57459e59943a4ecfefc8f6277ec6bf540
fetch_dependency SDL_image https://github.com/libsdl-org/SDL_image.git release-3.2.4 11154afb7855293159588b245b446a4ef09e574f
cmake -S "$deps_dir/SDL" -B "$deps_dir/sdl-build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$install_dir" \
    -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_UNIX_CONSOLE_BUILD=ON
cmake --build "$deps_dir/sdl-build" --parallel 4
cmake --install "$deps_dir/sdl-build"
cmake -S "$deps_dir/SDL_image" -B "$deps_dir/image-build" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$install_dir" \
    -DCMAKE_PREFIX_PATH="$install_dir" -DSDLIMAGE_SAMPLES=OFF \
    -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF
cmake --build "$deps_dir/image-build" --parallel 4
cmake --install "$deps_dir/image-build"
printf 'Configure the game with: cmake --preset dev -DCMAKE_PREFIX_PATH=%q\n' "$install_dir"
