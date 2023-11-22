# Debian testing is used since we need a recent version of vulkan headers
FROM debian:testing-slim
ARG DEBIAN_FRONTEND=noninteractive
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -yq --no-install-recommends libxinerama-dev libxcursor-dev \
    xorg-dev libglu1-mesa-dev pkg-config mesa-vulkan-drivers vulkan-validationlayers-dev xpra gcovr \
    curl wget zip unzip tar ca-certificates git vim build-essential libvulkan-dev glslang-tools cmake \
    && apt-get clean -y
ADD ./vcpkg.json /app/
ADD ./vcpkg /app/vcpkg
WORKDIR /app
RUN sh vcpkg/bootstrap-vcpkg.sh
RUN --mount=type=cache,target=/app/vcpkg/buildtrees \
    --mount=type=cache,target=/app/vcpkg/downloads \
    --mount=type=cache,target=/app/vcpkg/packages \
    ./vcpkg/vcpkg install
ADD . /app/
RUN cmake -S . -B build
WORKDIR /app/build
RUN make
ENV DISPLAY :100.0
CMD xpra start :100 && sleep 2 && ./plaxel_test

