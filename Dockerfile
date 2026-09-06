# syntax=docker/dockerfile:1
# Base image version is selectable via --build-arg UBUNTU_VERSION.
# NOTE: ubuntu:26.04 amd64 image is currently broken in the registry
# (empty /bin/dash -> "exec format error"), so the default is 24.04 (noble) LTS.
# Switch back to 26.04 once Canonical fixes the base image:
#   docker compose build --build-arg UBUNTU_VERSION=26.04 ptokax
ARG UBUNTU_VERSION=24.04
FROM ubuntu:${UBUNTU_VERSION} AS builder

ARG UBUNTU_VERSION
ARG BUILD_TYPE=Release

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    mold \
    pkgconf \
    liblua5.4-dev \
    lua5.4 \
    libsqlite3-dev \
    libcivetweb-dev \
    libtinyxml2-dev \
    prometheus-cpp-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    && rm -rf /var/lib/apt/lists/*

# Build zlib-ng (SIMD-optimized zlib replacement, vendored in deps/)
COPY deps/zlib-ng-2.3.3.tar.gz /tmp/zlib-ng.tar.gz
RUN tar xzf /tmp/zlib-ng.tar.gz -C /tmp && \
    rm /tmp/zlib-ng.tar.gz && \
    cmake -B /tmp/zlib-ng-2.3.3/build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DZLIB_COMPAT=ON \
        -DZLIB_ENABLE_TESTS=OFF \
        -DWITH_GZFILEOP=ON \
        -DWITH_OPTIM=ON \
        /tmp/zlib-ng-2.3.3 && \
    cmake --build /tmp/zlib-ng-2.3.3/build -j$(nproc) && \
    cmake --install /tmp/zlib-ng-2.3.3/build && \
    rm -rf /tmp/zlib-ng-2.3.3

WORKDIR /app

COPY CMakeLists.txt ./
# Build number must be pre-generated: bash gen-build-number.sh
# (test-hub.sh --docker does this automatically before docker build)
COPY build_number.txt ./
COPY core/ core/
COPY skein/ skein/
COPY sqlite/ sqlite/
COPY fly-server-test-port/ fly-server-test-port/
COPY benches/ benches/

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX=/usr/local -DBUILD_TESTING=OFF . && \
    cmake --build build -j$(nproc) && \
    cmake --install build

FROM ubuntu:${UBUNTU_VERSION}

ARG UBUNTU_VERSION
ARG BUILD_TYPE=Release

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Moscow
ENV LANG=ru_RU.CP1251
ENV LC_ALL=ru_RU.CP1251

# Versioned runtime libs differ between Ubuntu releases:
#   24.04: tinyxml2-10 / spdlog 1.12 / fmt 9
#   26.04: tinyxml2-11 / spdlog 1.15 / fmt 10
RUN apt-get update && apt-get install -y --no-install-recommends \
    liblua5.4-0 \
    libsqlite3-0 \
    libcivetweb1 \
    $(if [ "$UBUNTU_VERSION" = "26.04" ]; then echo "libtinyxml2-11 libspdlog1.15 libfmt10"; else echo "libtinyxml2-10 libspdlog1.12 libfmt9"; fi) \
    libprometheus-cpp-core1.0 \
    libprometheus-cpp-pull1.0 \
    iproute2 \
    binutils \
    tzdata \
    locales \
    $(if [ "$BUILD_TYPE" = "Debug" ]; then echo "libasan8 libubsan1"; fi) \
    && rm -rf /var/lib/apt/lists/* \
    && sed -i 's/# ru_RU.CP1251/ru_RU.CP1251/' /etc/locale.gen \
    && locale-gen ru_RU.CP1251 \
    && ln -sf /usr/share/zoneinfo/Europe/Moscow /etc/localtime \
    && dpkg-reconfigure -f noninteractive tzdata

COPY --from=builder /usr/local/bin/PtokaX /usr/local/bin/PtokaX
COPY --from=builder /usr/local/lib/libz.so* /usr/local/lib/
RUN echo "/usr/local/lib" > /etc/ld.so.conf.d/zlib-ng.conf && ldconfig

COPY asan.supp /app/asan.supp

EXPOSE 411 4111 37015

WORKDIR /app
CMD ["PtokaX"]
