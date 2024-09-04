FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive

# Exclude documentation files from /usr/share/doc/ to reduce the Docker image size
RUN echo 'path-exclude=/usr/share/doc/*' >> /etc/dpkg/dpkg.cfg.d/00_exclude-docs

# Install build dependencies
RUN : \
    && apt-get update \
    && apt-get install -y -q --fix-missing --no-install-recommends \
        apt-transport-https \
        apt-utils \
        arptables \
        autoconf \
        automake \
        avahi-daemon \
        avahi-utils \
        bison \
        build-essential \
        ca-certificates \
        check \
        cmake \
        curl \
        dhcpcd5 \
        dnsutils \
        dos2unix \
        doxygen \
        file \
        flex \
        g++ \
        g++-multilib \
        gawk \
        gcc \
        gcc-multilib \
        genromfs \
        gettext-base \
        gir1.2-gtk-3.0 \
        git \
        gperf \
        iperf \
        iproute2 \
        iputils-ping \
        iw \
        lcov \
        libbsd-dev \
        libcairo2-dev \
        libdbus-1-dev \
        libdbus-glib-1-dev \
        libffi-dev \
        libgcrypt-dev \
        libgirepository1.0-dev \
        libglib2.0-dev \
        libical-dev \
        libjpeg-dev \
        libjson-c-dev \
        liblzma-dev \
        libncurses-dev \
        libncurses5-dev \
        libnet1-dev \
        libpcap-dev \
        libpython2.7 \
        libreadline-dev \
        libsqlite3-dev \
        libssl-dev \
        libtool \
        libudev-dev \
        libusb-1.0-0 \
        libusb-1.0-0-dev \
        libxml2-dev \
        libxslt-dev \
        libyaml-dev \
        libsdl2-dev \
        libslirp-dev \
        libpixman-1-dev \
        libnuma1 \
        lzip \
        lzma \
        make \
        minicom \
        net-tools \
        network-manager \
        ninja-build \
        openssh-client \
        patch \
        ppp \
        psmisc \
        python-is-python3 \
        python3 \
        python3-dev \
        python3-pip \
        python3-venv \
        rsync \
        ruby \
        rustc \
        socat \
        software-properties-common \
        sudo \
        telnet \
        udev \
        uncrustify \
        unrar \
        unzip \
        vim-common \
        wget \
        wireless-tools \
        wpasupplicant \
        xxd \
        xz-utils \
        zip \
        zlib1g-dev \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* \
    && :


ENV PIP_PACKAGES="\
    cmake-format \
    CodeChecker \
    cvt2utf \
    cxxfilt \
    imgtool \
    kconfiglib \
    pexpect==4.8.0 \
    pyelftools \
    pyserial==3.5 \
    setuptools \
    wheel \
    pytest \
"

# install PIP packages for Python3
RUN python3 -m pip install $PIP_PACKAGES

# Install QEMU for RISC-V
ARG QEMU_VER_SUFFIX=develop-8.2.0-20240122
ARG QEMU_VER=esp-${QEMU_VER_SUFFIX}
ARG QEMU_DIST_SUFFIX=x86_64-linux-gnu.tar.xz

ARG QEMU_RISCV32_PREFIX=qemu-riscv32-softmmu
ARG QEMU_RISCV32_DIST=${QEMU_RISCV32_PREFIX}-esp_develop_8.2.0_20240122-x86_64-linux-gnu.tar.xz
ARG QEMU_RISCV32_SHA256=95ac86d7b53bf98b5ff19c33aa926189b849f5a0daf8f41e160bc86c5e31abd4
RUN : \
    && wget --no-verbose https://github.com/espressif/qemu/releases/download/${QEMU_VER}/${QEMU_RISCV32_DIST} \
    && echo "${QEMU_RISCV32_SHA256} ${QEMU_RISCV32_DIST}" | sha256sum --check --strict - \
    && mkdir -p /opt/${QEMU_RISCV32_PREFIX} \
    && tar -xf ${QEMU_RISCV32_DIST} -C /opt/${QEMU_RISCV32_PREFIX} \
    && rm ${QEMU_RISCV32_DIST} \
    && :
ENV PATH=/opt/${QEMU_RISCV32_PREFIX}/qemu/bin:${PATH}

# Install IDF tools
ARG IDF_URL_PATH="https://github.com/espressif/esp-idf.git"

ENV IDF_PATH=/opt/esp/idf
ENV IDF_TOOLS_PATH=/opt/esp
ENV IDF_VERSION=v5.2.2

RUN git clone -b $IDF_VERSION --recursive $IDF_URL_PATH $IDF_PATH

# Install IDF
RUN : \
    && update-ca-certificates --fresh \
    && $IDF_PATH/install.sh --enable-pytest --enable-ci \
    && $IDF_PATH/tools/idf_tools.py --non-interactive install required --targets=all \
    && $IDF_PATH/tools/idf_tools.py --non-interactive install cmake \
    && $IDF_PATH/tools/idf_tools.py --non-interactive install-python-env \
    && $IDF_PATH/tools/idf_tools.py --non-interactive export --prefer-system \
    && rm -rf $IDF_TOOLS_PATH/dist \
    && :

# RUN . ./$IDF_PATH/export.sh

WORKDIR /

CMD ["/bin/bash"]
