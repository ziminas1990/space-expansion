FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        git \
        pkg-config \
        python3 \
        python3-pip \
        python3-venv \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv \
    && /opt/venv/bin/pip install --no-cache-dir conan \
    && /opt/venv/bin/conan profile detect --force
ENV PATH="/opt/venv/bin:$PATH"

WORKDIR /src
COPY conanfile.txt /src/conanfile.txt

# NOTE: creating empty CMakeLists.txt file to force Conan to generate a
# CMakePresets.json file
RUN touch /src/CMakeLists.txt \
    && conan install /src/conanfile.txt --output-folder=/build --build=missing

COPY . /src
RUN cmake -S /src -B /build --preset conan-release \
    && cmake --build /build --config Release -- -j"$(nproc)" \
    && cmake --install /build --prefix /usr

# Build the final image

FROM ubuntu:24.04

COPY --from=builder /usr/bin/space-expansion-server /usr/bin/space-expansion-server
COPY space-expansion.cfg /usr/share/space-expansion/space-expansion.cfg

EXPOSE 6842/udp
EXPOSE 17392/udp
EXPOSE 25000-25200/udp

ENTRYPOINT ["/usr/bin/space-expansion-server"]
CMD ["/etc/space-expansion/space-expansion.cfg"]
