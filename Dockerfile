FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

FROM ubuntu:22.04 AS runtime

COPY --from=builder /app/build/tests/riscv_tests /usr/local/bin/riscv_tests
COPY --from=builder /app/build/examples/riscv_demo /usr/local/bin/riscv_demo

CMD ["sh", "-c", "riscv_tests && riscv_demo"]
