# Mirrors the image and system deps the CI workflow builds with
# (.github/workflows/bazel-build-test.yml), so a local devcontainer build is
# the same environment CI already validates -- not a separate, unverified one.
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential python3 python3-dev git curl ca-certificates patch unzip \
    && rm -rf /var/lib/apt/lists/*

# bazelisk reads .bazelversion itself; pick the binary matching the
# container's actual architecture (arm64 on Apple Silicon hosts under
# Docker Desktop/Colima, amd64 elsewhere) rather than CI's amd64-only asset.
RUN ARCH="$(dpkg --print-architecture)" \
    && curl -fsSL -o /usr/local/bin/bazel \
        "https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-${ARCH}" \
    && chmod +x /usr/local/bin/bazel
