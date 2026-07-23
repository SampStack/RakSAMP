# syntax=docker/dockerfile:1.7
FROM ubuntu:24.04 AS build

ARG TARGETARCH
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential ca-certificates cmake curl && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON && \
    cmake --build build --parallel && \
    ctest --test-dir build --output-on-failure

FROM ubuntu:24.04 AS runtime
RUN groupadd --gid 10001 raksamp && \
    useradd --uid 10001 --gid 10001 --create-home --home-dir /work raksamp
WORKDIR /work
USER 10001:10001

FROM runtime AS client
COPY --from=build /src/build/bin/raksamp-client /usr/local/bin/raksamp-client
COPY --chown=10001:10001 client/bin/RakSAMPClient.xml /work/RakSAMPClient.xml
ENTRYPOINT ["raksamp-client"]
CMD ["--config", "/work/RakSAMPClient.xml"]

FROM runtime AS server
COPY --from=build /src/build/bin/raksamp-server /usr/local/bin/raksamp-server
COPY --chown=10001:10001 server/bin/ /work/
EXPOSE 7777/udp
ENTRYPOINT ["raksamp-server"]
CMD ["--config", "/work/RakSAMPServer.xml"]
