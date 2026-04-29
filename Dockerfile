FROM debian:bookworm-slim

RUN apt update && apt install -y \
    build-essential \
    cmake \
    gdb \
    pkg-config \
    linux-libc-dev \
    can-utils