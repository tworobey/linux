# ---------- BUILD ----------
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
        python3-pip \
        cmake \
    && pip3 install conan==1.*

WORKDIR /app

COPY conanfile.txt .
COPY CMakeLists.txt .
COPY ./src ./src
COPY ./tests ./tests

RUN mkdir build && cd build && \
    conan install .. \
        --build=missing \
        -s build_type=Release \
        -s compiler.libcxx=libstdc++11 && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build .

# ---------- RUN ----------
FROM ubuntu:22.04 as run
RUN apt update && apt install -y libpq5 && rm -rf /var/lib/apt/lists/*

RUN groupadd -r www && useradd -r -g www www

WORKDIR /app

# копируем бинарник
COPY --from=build /app/build/bin/game_server .

# копируем данные
COPY ./data ./data
COPY ./static ./static

USER www

ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static", "--tick-period", "50"]
