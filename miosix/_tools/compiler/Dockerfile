# ==========================================
# Stage 1: build toolchain
# ==========================================
FROM ubuntu:20.04 AS builder

ARG COMPILER
ENV COMPILER=${COMPILER}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt -y install \
    gcc g++ make libncurses5-dev byacc flex texinfo patch tar unzip lzip \
    libelf-dev perl libexpat1-dev curl xz-utils sudo

# Copy only the selected subdirectory
COPY ${COMPILER} /project/${COMPILER}

WORKDIR /project/${COMPILER}

RUN ./download.sh && ./install-script.sh -j$(nproc)

WORKDIR /opt
RUN tar -zcvf arm-miosix-eabi-${COMPILER}.tar.gz arm-miosix-eabi

# ==========================================
# Stage 2: export artifact
# ==========================================
FROM scratch AS export
ARG COMPILER
COPY --from=builder /opt/arm-miosix-eabi-${COMPILER}.tar.gz /
