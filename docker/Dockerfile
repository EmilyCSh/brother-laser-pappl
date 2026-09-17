FROM alpine:edge

ARG LLVM_VERSION
ARG CUPS_VERSION
ARG PAPPL_VERSION

RUN apk add --no-cache \
    alpine-sdk \
    autoconf \
    automake \
    libtool \
    gperf \
    cmake \
    ninja-is-really-ninja \
    pkgconfig \
    clang${LLVM_VERSION} \
    lld${LLVM_VERSION} \
    clang${LLVM_VERSION}-extra-tools \
    eudev \
    eudev-dev \
    libusb-dev \
    jpeg-dev \
    libjpeg-turbo-static \
    libpng-dev \
    libpng-static \
    zlib-dev \
    zlib-static \
    openssl-dev \
    openssl-libs-static \
    linux-pam-dev \
    linux-headers \
    wget \
    tar \
    xz \
    libc++-dev \
    libc++-static \
    mimalloc3-dev

ENV CC=clang-${LLVM_VERSION}
ENV CXX=clang++-${LLVM_VERSION}
ENV CFLAGS="-static"
ENV CXXFLAGS="-static"
ENV LDFLAGS="-static -fuse-ld=lld -Wl,--whole-archive -lmimalloc -Wl,--no-whole-archive"

RUN wget https://github.com/OpenPrinting/cups/releases/download/v${CUPS_VERSION}/cups-${CUPS_VERSION}-source.tar.gz && \
    tar -xzf cups-${CUPS_VERSION}-source.tar.gz && \
    rm cups-${CUPS_VERSION}-source.tar.gz && \
    cd cups-${CUPS_VERSION} && \
    ./configure --prefix=/usr --disable-shared --enable-static --with-tls=openssl --disable-gssapi --disable-dbus --with-dnssd=no --disable-pam && \
    make -j"$(nproc)" && \
    make install && \
    cd .. && \
    rm -rf cups-${CUPS_VERSION}

RUN wget https://github.com/michaelrsweet/pappl/releases/download/v${PAPPL_VERSION}/pappl-${PAPPL_VERSION}.tar.gz && \
    tar -xzf pappl-${PAPPL_VERSION}.tar.gz && \
    rm pappl-${PAPPL_VERSION}.tar.gz && \
    cd pappl-${PAPPL_VERSION} && \
    ./configure --prefix=/usr --disable-shared --enable-static --disable-libpam --enable-libusb --enable-libjpeg --enable-libpng && \
    make -C pappl -j"$(nproc)" && \
    make -C pappl install && \
    cd .. && \
    rm -rf pappl-${PAPPL_VERSION}


