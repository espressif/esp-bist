FROM espressif/idf:latest

ARG DEBIAN_FRONTEND=noninteractive

# Exclude documentation files from /usr/share/doc/ to reduce the Docker image size
RUN echo 'path-exclude=/usr/share/doc/*' >> /etc/dpkg/dpkg.cfg.d/00_exclude-docs

# Install build dependencies
RUN : \
    && apt-get update \
    && apt-get install -y -q --fix-missing --no-install-recommends \
        kconfig-frontends \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* \
    && :

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

WORKDIR /

CMD ["/bin/bash"]
