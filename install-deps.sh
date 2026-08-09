#!/bin/bash
# Установка зависимостей для сборки FlylinkDC Hub (Ubuntu 26.04)
set -e

apt-get update
apt-get install -y \
    build-essential cmake ninja-build mold ccache \
    liblua5.4-dev libsqlite3-dev \
    libcivetweb-dev libtinyxml2-dev prometheus-cpp-dev nlohmann-json3-dev libspdlog-dev \
    luacheck cppcheck \
    python3

# Build zlib-ng (SIMD-optimized zlib replacement) from source
if ! pkg-config --exists zlib-ng 2>/dev/null; then
    echo "Building zlib-ng from source..."
    ZLIB_NG_VERSION="2.3.3"
    ZLIB_NG_URL="https://github.com/zlib-ng/zlib-ng/archive/refs/tags/${ZLIB_NG_VERSION}.tar.gz"

    TMPDIR=$(mktemp -d)
    cd "$TMPDIR"
    curl -sL "$ZLIB_NG_URL" | tar xz --strip-components=1
    cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DZLIB_COMPAT=ON \
        -DZLIB_ENABLE_TESTS=OFF \
        -DWITH_GZFILEOP=ON \
        -DWITH_OPTIM=ON \
        .
    cmake --build build -j$(nproc)
    cmake --install build
    ldconfig
    cd /
    rm -rf "$TMPDIR"
    echo "zlib-ng ${ZLIB_NG_VERSION} installed."
fi

echo "OK: All build dependencies installed."
