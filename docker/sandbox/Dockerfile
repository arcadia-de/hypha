ARG UBUNTU_VERSION=26.04
ARG HYPHA_BUILDENV_VERSION=latest
FROM ghcr.io/arcadia-de/hypha/build-env:${HYPHA_BUILDENV_VERSION} AS builder
ARG GO_VERSION=1.26.4

ENV DEBIAN_FRONTEND=noninteractive
ENV GOPATH=/go
ENV CGO_ENABLED=1
ENV PATH=$PATH:/usr/local/go/bin:$GOPATH/bin

COPY . /hypha

WORKDIR /hypha

RUN ldconfig \
 && meson setup build --reconfigure \
 && ninja -C build \
 && PKG_CONFIG_PATH="$(pwd)/build/meson-uninstalled:$PKG_CONFIG_PATH" \
    go build -ldflags="-X main.Version=0.0.0 -linkmode=external -s -w -extldflags=-Wl,--gc-sections" \
      -o hypha \
      cmd/hypha/main.go

FROM ubuntu:${UBUNTU_VERSION}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
    ca-certificates \
    libssl3 \
    zlib1g \
 && rm -rf /var/lib/apt/lists/* 

COPY --from=builder /usr/local/ /usr/local/
COPY --from=builder /usr/lib/libjansson.so* /usr/lib/
COPY --from=builder /hypha/hypha /bin/hypha

RUN ldconfig \
 && mkdir -p /root/.config/hypha

WORKDIR /root/.config/hypha
CMD [ "/bin/hypha", "info" ]
