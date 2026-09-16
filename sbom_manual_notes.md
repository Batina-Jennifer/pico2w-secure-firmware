# Manually-Supplemented SBOM Components

Automated scanning (Syft, CycloneDX output) found 0 auto-detectable
packages when scoped to source only (excluding build/). This is a known
limitation: Syft's scanners rely on package manifests (package.json,
requirements.txt, etc.), and this project vendors its dependencies via the
Pico SDK's CMake FetchContent mechanism rather than a package manager
Syft recognizes.

The following components are manually declared based on the actual vendored
source used in this build:

| Component | Version / Commit | Source |
|---|---|---|
| Raspberry Pi Pico SDK | 2.3.0 | https://github.com/raspberrypi/pico-sdk |
| mbedTLS (via Pico SDK) | 82418f42a1bc7db94ea81f603e05f0061b553c40 | lib/mbedtls submodule |

This gap and the manual supplementation is a realistic reflection of
current SBOM tooling limitations for embedded/vendored C dependencies,
distinct from ecosystems with native package managers (npm, pip, cargo).