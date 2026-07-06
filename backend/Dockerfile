FROM debian:bookworm AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential cmake git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSMARTTRAFFIC_BUILD_TESTS=OFF \
    && cmake --build build --target smarttraffic_server --parallel

FROM debian:bookworm-slim AS runtime

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /app/build/backend/smarttraffic_server /app/smarttraffic_server

ENV PORT=18080
EXPOSE 18080

CMD ["/app/smarttraffic_server"]
