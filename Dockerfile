ARG UBUNTU_VERSION=26.04
FROM ubuntu:${UBUNTU_VERSION} AS builder
ARG GO_VERSION=1.26.4
ARG GRAPHVIZ_VERSION=16.0.0
ARG JANSSON_VERSION=2.15.1
ARG LIBARCHIVE_VERSION=3.8.8
ARG LIBGIT2_VERSION=1.9.7
ARG LIBSODIUM_VERSION=1.0.22
ARG LIBUV_VERSION=1.52.1
ARG UTIL_LINUX_VERSION=2.42.2
ARG WAMR_VERSION=2.4.5
ARG XXHASH_VERSION=0.8.3
ARG LUA_VERSION=5.4.8

ENV GRAPHVIZ_VERSION=${GRAPHVIZ_VERSION}
ENV UTIL_LINUX_VERSION=${UTIL_LINUX_VERSION}
ENV LIBGIT2_VERSION=${LIBGIT2_VERSION}
ENV LIBUV_VERSION=${LIBUV_VERSION}
ENV XXHASH_VERSION=${XXHASH_VERSION}
ENV LIBARCHIVE_VERSION=${LIBARCHIVE_VERSION}
ENV WAMR_VERSION=${WAMR_VERSION}
ENV LIBSODIUM_VERSION=${LIBSODIUM_VERSION}
ENV GO_VERSION=${GO_VERSION}
ENV JANSSON_VERSION=${JANSSON_VERSION}
ENV LUA_VERSION=${LUA_VERSION}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    curl \
    ninja-build \
    python3 \
    python3-pip \
    build-essential \
    pkg-config \
    ca-certificates \
    curl \
    git \
    autoconf \
    cmake \
    automake \
    autopoint \
    libssl-dev \
    libtool \
    bison \
    flex \
    bzip2 \
    gettext \
    libltdl-dev \
 && rm -rf /var/lib/apt/lists/* \
 && pip3 install --break-system-packages \
    meson

RUN curl -sSL "https://golang.org/dl/go${GO_VERSION}.linux-amd64.tar.gz" -o go-${GO_VERSION}.tar.gz \
 && tar -C /usr/local -xzf go-${GO_VERSION}.tar.gz

ENV GOPATH=/go
ENV PATH=$PATH:/usr/local/go/bin:$GOPATH/bin

# ╭─────────╮
# │ Jansson │
# ╰─────────╯
RUN curl -SsfL https://github.com/akheron/jansson/releases/download/v${JANSSON_VERSION}/jansson-${JANSSON_VERSION}.tar.bz2 -o jansson.tar.bz2 \
 && tar -xf jansson.tar.bz2 \
 && cd jansson-${JANSSON_VERSION} \
 && ./configure --prefix=/usr --disable-static \
 && make -j$(nproc) \
 && make install \
 && cd .. 

# ╭───────────╮
# │ libsodium │
# ╰───────────╯
RUN curl -L https://github.com/jedisct1/libsodium/archive/refs/tags/${LIBSODIUM_VERSION}-RELEASE.tar.gz -o libsodium-${LIBSODIUM_VERSION}.tar.gz \
 && tar -xvf libsodium-${LIBSODIUM_VERSION}.tar.gz \
 && cd libsodium-${LIBSODIUM_VERSION}-RELEASE \
 && ./configure \
 && make -j$(nproc) \
 && make check \
 && make install \
 && cd ..

# ╭────────────╮
# │ util-linux │
# ╰────────────╯
RUN curl -L https://github.com/util-linux/util-linux/archive/refs/tags/v${UTIL_LINUX_VERSION}.tar.gz -o util-linux-${UTIL_LINUX_VERSION}.tar.gz\
 && tar -xf util-linux-${UTIL_LINUX_VERSION}.tar.gz \
 && cd util-linux-${UTIL_LINUX_VERSION} \
 && ./autogen.sh \
 && ./configure --disable-all-programs --enable-libuuid \
 && make -j$(nproc) \
 && make install \
 && cd ..

# ╭────────────╮
# │ libarchive │
# ╰────────────╯
RUN curl -L https://github.com/libarchive/libarchive/archive/refs/tags/v${LIBARCHIVE_VERSION}.tar.gz -o libarchive-${LIBARCHIVE_VERSION}.tar.gz \
 && tar -xf libarchive-${LIBARCHIVE_VERSION}.tar.gz \
 && cd libarchive-${LIBARCHIVE_VERSION} \
 && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
 && cmake --build build -j$(nproc) \
 && cmake --install build \
 && cd ..

# ╭─────────╮
# │ libgit2 │
# ╰─────────╯
RUN curl -L https://github.com/libgit2/libgit2/archive/refs/tags/v${LIBGIT2_VERSION}.tar.gz -o libgit2-${LIBGIT2_VERSION}.tar.gz \
 && tar -xf libgit2-${LIBGIT2_VERSION}.tar.gz \
 && cd libgit2-${LIBGIT2_VERSION} \
 && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
 && cmake --build build -j$(nproc) \
 && cmake --install build \
 && cd ..

# ╭──────────╮
# │ graphviz │
# ╰──────────╯
RUN curl -L https://gitlab.com/graphviz/graphviz/-/archive/${GRAPHVIZ_VERSION}/graphviz-${GRAPHVIZ_VERSION}.tar.gz -o graphviz-${GRAPHVIZ_VERSION}.tar.gz \
 && tar -xf graphviz-${GRAPHVIZ_VERSION}.tar.gz \
 && cd graphviz-${GRAPHVIZ_VERSION} \
 && ./autogen.sh \
 && ./configure --prefix=/usr/local \
 && make -j$(nproc) \
 && make install \
 && cd ..

# ╭──────╮
# │ WAMR │
# ╰──────╯
RUN curl -L https://github.com/wasm-micro-runtime/wasm-micro-runtime/archive/refs/tags/WAMR-${WAMR_VERSION}.tar.gz -o wamr-${WAMR_VERSION}.tar.gz \
 && tar -xf wamr-${WAMR_VERSION}.tar.gz \
 && cd wasm-micro-runtime-WAMR-${WAMR_VERSION} \
 && cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
 && cmake --build build -j$(nproc) \
 && cmake --install build \
 && mkdir -p /usr/local/lib/pkgconfig \
 && echo "prefix=/usr/local" > /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "exec_prefix=\${prefix}" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "libdir=\${exec_prefix}/lib" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "includedir=\${prefix}/include" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "Name: libiwasm" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "Description: WebAssembly Micro Runtime" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "Version: ${WAMR_VERSION}" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "Libs: -L\${libdir} -liwasm -lpthread -lm" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "Cflags: -I\${includedir}" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && cd ..

# ╭────────╮
# │ xxhash │
# ╰────────╯
RUN curl -L https://github.com/Cyan4973/xxHash/archive/refs/tags/v${XXHASH_VERSION}.tar.gz -o xxhash-${XXHASH_VERSION}.tar.gz \
 && tar -xf xxhash-${XXHASH_VERSION}.tar.gz \
 && cd xxHash-${XXHASH_VERSION} \
 && make -j$(nproc) \
 && make install \
 && cd ..

# ╭───────╮
# │ libuv │
# ╰───────╯
RUN curl -L https://github.com/libuv/libuv/archive/refs/tags/v${LIBUV_VERSION}.tar.gz -o libuv-${LIBUV_VERSION}.tar.gz \
 && tar -xf libuv-${LIBUV_VERSION}.tar.gz \
 && cd libuv-${LIBUV_VERSION} \
 && sh autogen.sh \
 && ./configure \
 && make -j$(nproc) \
 && make install \
 && cd ..

RUN curl -L -R -O https://www.lua.org/ftp/lua-${LUA_VERSION}.tar.gz -o lua-${LUA_VERSION}.tar.gz \
 && tar zxf lua-${LUA_VERSION}.tar.gz \
 && cd lua-${LUA_VERSION} \
 && make -j$(nproc) \
 && make install \
 && mkdir -p /usr/local/lib/pkgconfig \
 && echo "prefix=/usr/local" > /usr/local/lib/pkgconfig/liblua54.pc \
 && echo "exec_prefix=\${prefix}" >> /usr/local/lib/pkgconfig/liblua54.pc \
 && echo "libdir=\${exec_prefix}/lib" >> /usr/local/lib/pkgconfig/liblua54.pc \
 && echo "includedir=\${prefix}/include" >> /usr/local/lib/pkgconfig/liblua54.pc \
 && echo "" >> /usr/local/lib/pkgconfig/liblua54.pc \
 && echo "Name: liblua54" >> /usr/local/lib/pkgconfig/libiwasm.pc \
 && echo "Description: An Extensible Embedded Language" >> /usr/local/lib/pkgconfig/liblua54.pc \
 && echo "Version: ${LUA_VERSION}" >> /usr/local/lib/pkgconfig/liblua54.pc \
 && echo "Libs: -L\${libdir} -llua -lm" >> /usr/local/lib/pkgconfig/liblua54.pc \
 && echo "Cflags: -I\${includedir}" >> /usr/local/lib/pkgconfig/liblua54.pc \
 && cd ..

#TODO(@s0cks):
# FROM ubuntu:${UBUNTU_VERSION}
# COPY --from=builder /usr/local/ /usr/local/
# RUN ldconfig

