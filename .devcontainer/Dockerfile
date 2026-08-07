FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

# Exclude documentation files from /usr/share/doc/ to reduce the Docker image size
RUN echo 'path-exclude=/usr/share/doc/*' >> /etc/dpkg/dpkg.cfg.d/00_exclude-docs

# TARGETARCH is set by BuildKit/buildx (--platform).
ARG TARGETARCH

# Install build dependencies
RUN : \
    && ARCH="${TARGETARCH:-$(dpkg --print-architecture)}" \
    && apt-get update \
    && apt-get install -y -q --fix-missing --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        gcc \
        clang \
        clang-tidy \
        clang-format \
        git \
        kconfig-frontends \
        libdbus-1-dev \
        libdbus-glib-1-dev \
        libgcrypt-dev \
        libusb-1.0-0 \
        libusb-1.0-0-dev \
        libsdl2-dev \
        libslirp-dev \
        libpixman-1-dev \
        minicom \
        ninja-build \
        openssh-client \
        cppcheck \
        pkg-config \
        python-is-python3 \
        python3 \
        python3-dev \
        python3-pip \
        python3-venv \
        tar \
        wget \
        xz-utils \
    && if [ "${ARCH}" = "amd64" ]; then \
         apt-get install -y -q --no-install-recommends gcc-multilib; \
       fi \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* \
    && :

# Install QEMU for RISC-V
ARG QEMU_VER_SUFFIX=develop-8.2.0-20240122
ARG QEMU_VER=esp-${QEMU_VER_SUFFIX}

ARG QEMU_RISCV32_PREFIX=qemu-riscv32-softmmu
RUN : \
    && ARCH="${TARGETARCH:-$(dpkg --print-architecture)}" \
    && case "${ARCH}" in \
         amd64)  QEMU_HOST=x86_64-linux-gnu; \
                 QEMU_RISCV32_SHA256=95ac86d7b53bf98b5ff19c33aa926189b849f5a0daf8f41e160bc86c5e31abd4 ;; \
         arm64)  QEMU_HOST=aarch64-linux-gnu; \
                 QEMU_RISCV32_SHA256=4089f7958f753779e5b4c93fe2469d62850a1f209b0bda8b75d55fe4a61ca39b ;; \
         *) echo "Unsupported architecture: ${ARCH}"; exit 1 ;; \
       esac \
    && QEMU_RISCV32_DIST=${QEMU_RISCV32_PREFIX}-esp_develop_8.2.0_20240122-${QEMU_HOST}.tar.xz \
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
ENV IDF_VERSION=v6.0

RUN git clone -b $IDF_VERSION --recursive --shallow-submodules $IDF_URL_PATH $IDF_PATH

# Install IDF.
# Note: --enable-test-specific replaces the pre-v6 --enable-pytest flag
# (run install.sh --help to see the current feature set).
RUN : \
    && update-ca-certificates --fresh \
    && $IDF_PATH/install.sh --enable-test-specific --enable-ci \
    && $IDF_PATH/tools/idf_tools.py --non-interactive install required --targets=all \
    && $IDF_PATH/tools/idf_tools.py --non-interactive install cmake \
    && $IDF_PATH/tools/idf_tools.py --non-interactive install-python-env \
    && $IDF_PATH/tools/idf_tools.py --non-interactive export --prefer-system \
    && rm -rf $IDF_TOOLS_PATH/dist \
    && :

# Clone MCUboot (branch with IDF v6.0 HAL support, no separate IDF version needed)
ARG MCUBOOT_URL="https://github.com/almir-okato/mcuboot.git"
ARG MCUBOOT_BRANCH=esp32h4_initial_support
ENV MCUBOOT_PATH=/opt/mcuboot
RUN git clone -b $MCUBOOT_BRANCH --depth 1 --recursive --shallow-submodules $MCUBOOT_URL $MCUBOOT_PATH

# Install PIP packages inside the IDF environment
ENV PIP_PACKAGES="\
    kconfiglib \
    imgtool==2.3.0 \
    pre-commit \
"
RUN . /opt/esp/python_env/idf6.0_py3.10_env/bin/activate \
    && python3 -m pip install $PIP_PACKAGES

RUN echo '. $IDF_PATH/export.sh' >> /root/.profile
