FROM ubuntu:rolling as vcpkg
ARG DEBIAN_FRONTEND=noninteractive
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -yq --no-install-recommends wget curl zip unzip tar ca-certificates git \
    build-essential libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev pkg-config libvulkan-dev cmake
ADD ./vcpkg.json /app/
ADD ./vcpkg /app/vcpkg
WORKDIR /app
RUN sh vcpkg/bootstrap-vcpkg.sh
RUN --mount=type=cache,target=/app/vcpkg/buildtrees \
    --mount=type=cache,target=/app/vcpkg/downloads \
    --mount=type=cache,target=/app/vcpkg/packages \
    ./vcpkg/vcpkg install

FROM ubuntu:rolling as cmake
ARG DEBIAN_FRONTEND=noninteractive
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -yq --no-install-recommends wget curl zip unzip tar ca-certificates git \
    build-essential libxinerama-dev libxcursor-dev xorg-dev libglu1-mesa-dev pkg-config libvulkan-dev cmake
# TODO: replace by system version when compatible
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc > /etc/apt/trusted.gpg.d/lunarg.asc && \
    wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list &&\
    apt-get update && apt-get install -yq --no-install-recommends vulkan-sdk libglu1-mesa-dev
ADD src /app/src
ADD shaders /app/shaders
ADD textures /app/textures
ADD fonts /app/fonts
ADD CMakeLists.txt CMakePresets.json shaderCompiling-CMakeLists.txt vcpkg.json vk_layer_settings.txt main.cpp /app/
ADD vcpkg /app/vcpkg
ADD test /app/test
WORKDIR /app
COPY --from=vcpkg /app/vcpkg_installed /app/build/debug/vcpkg_installed
RUN cmake --workflow --preset build

FROM ubuntu:rolling as run
ARG DEBIAN_FRONTEND=noninteractive
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    apt-get update && apt-get install -yq --no-install-recommends wget curl zip unzip tar ca-certificates
# TODO: replace by system version when compatible
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
    wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc > /etc/apt/trusted.gpg.d/lunarg.asc && \
    wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list http://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list &&\
    apt-get update && apt-get install -yq --no-install-recommends xpra mesa-vulkan-drivers vulkan-sdk
COPY --from=cmake /app/build/debug/plaxel_test /app/plaxel_test
COPY --from=cmake /app/build/debug/test /app/test
WORKDIR /app
ENV DISPLAY :100.0
CMD xpra start :100 && sleep 2 && ./plaxel_test