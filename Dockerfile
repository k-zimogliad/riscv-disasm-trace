FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/tests/riscv_tests /usr/local/bin/riscv_tests
COPY --from=builder /src/build/examples/riscv_demo /usr/local/bin/riscv_demo

CMD echo "=== Running Tests ===" && riscv_tests && echo "" && echo "=== Running Demo ===" && riscv_demo
