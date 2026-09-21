# Secure Firmware Update & Automated SBOM Pipeline

**Hardware Root of Trust + DevSecOps pipeline for the Raspberry Pi Pico 2 W (RP2350)**

If it's not signed, it doesn't boot.

![Status](https://img.shields.io/badge/status-working-brightgreen)
![Hardware](https://img.shields.io/badge/hardware-RP2350-blue)

---

## Overview

This project builds an end-to-end secure firmware deployment pipeline for a Raspberry Pi Pico 2 W, aligned with EU Cyber Resilience Act (CRA) supply chain security requirements.

The board **refuses to run any firmware that isn't cryptographically signed**. Every code push is automatically scanned for vulnerabilities, inventoried into a Software Bill of Materials (SBOM), checked against known CVEs, compiled, and signed — before the binary ever reaches the hardware.

## Architecture

```
DEVELOPMENT & WORKSPACE (LOCAL)
1. Developer writes C firmware & embeds public_key.h
                │
                ▼ git push
DEVSECOPS PIPELINE (GITHUB ACTIONS)
2. Static Application Security Testing (SAST / Cppcheck)
3. Automated Software Bill of Materials Generation (Syft)
4. Vulnerability & Dependency CVE Scanning (Grype)
5. Compile C Source into RP2350 Binary Payload (.uf2)
6. Generate SHA-256 Digest & Sign Payload with Private Key
                │
                ▼ Flash Binary (.uf2) via USB
HARDWARE EXECUTION (PICO 2 W)
7. RP2350 Bootloader calculates SHA-256 Hash of incoming binary
8. mbedTLS validates ECDSA Signature against public_key.h
                │
        Is Cryptographic Signature Valid?
                │
        ┌───────┴────────┐
        ▼                ▼
      YES (Pass)      NO (Fail/Tampered)
        │                │
   9A. BOOT SUCCESS   9B. SECURITY ALERT
   Executes main app   Halts system CPU &
   code, outputs        logs violation to
   serial logs.         serial console.
```

## Features

- **Real cryptographic verification** — genuine SHA-256 hashing and ECDSA P-256 signature verification on-device via mbedTLS, not simulated
- **Fail-secure design** — any verification failure halts the board permanently rather than retrying or falling through
- **Automated SAST** — Cppcheck runs on every push, catching memory/safety issues before compilation
- **Automated SBOM** — CycloneDX-format Software Bill of Materials generated via Syft on every build
- **Automated CVE scanning** — Grype cross-references the SBOM against known vulnerability databases
- **Automated signing & release** — GitHub Actions signs the compiled binary using a private key stored in GitHub Secrets, then publishes a GitHub Release with the signed firmware and all compliance artifacts attached

## Tech Stack

| Category | Tools |
| --- | --- |
| Hardware | Raspberry Pi Pico 2 W (RP2350) |
| Firmware | C, Pico SDK 2.3.0, mbedTLS |
| Cryptography | OpenSSL (ECDSA P-256, SHA-256) |
| SAST | Cppcheck |
| SBOM | Syft (CycloneDX) |
| CVE Scanning | Grype |
| CI/CD | GitHub Actions |
| Build System | CMake, Ninja, ARM GCC toolchain |

## Repository Structure

```
.
├── main.c                    # Bootloader: SHA-256 + ECDSA verification logic
├── CMakeLists.txt            # Build configuration (target: secure_boot_app)
├── pico_sdk_import.cmake     # Standard Pico SDK import boilerplate
├── mbedtls_config.h          # mbedTLS feature flags (SHA-256, ECDSA, PK parsing)
├── public_key.h              # Embedded ECDSA public key (DER, generated via xxd -i)
├── payload.h                  # Embedded firmware payload (generated)
├── signature.h                # Embedded ECDSA signature (generated)
├── ec_private_key.pem         # Private signing key (gitignored — never committed)
├── ec_public_key.pem          # Public key (PEM)
├── ec_public_key.der          # Public key (DER)
├── sbom.json                   # Generated SBOM (CycloneDX)
├── sbom_manual_notes.md        # Manually documented vendored dependencies
├── cppcheck_report.txt         # Latest SAST scan output
└── .github/workflows/
    └── sec-pipeline.yml        # SAST + SBOM + CVE scan + build + sign + release
```

## Getting Started

### Prerequisites

- Raspberry Pi Pico 2 W
- [VS Code](https://code.visualstudio.com/) + Raspberry Pi Pico extension (installs Pico SDK 2.x, ARM GCC, CMake, Ninja)
- [Git for Windows](https://git-scm.com/) (includes Git Bash)
- [OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)
- Python 3

### 1. Generate your signing keypair

```bash
openssl ecparam -name prime256v1 -genkey -noout -out ec_private_key.pem
openssl ec -in ec_private_key.pem -pubout -out ec_public_key.pem
```

The private key **never** leaves your machine or CI secrets — it's excluded via `.gitignore`.

### 2. Embed the public key and signed payload

```bash
openssl ec -in ec_private_key.pem -pubout -outform DER | xxd -i > public_key.h
```

The firmware payload and its ECDSA signature are similarly converted into `payload.h` and `signature.h` as C byte arrays for compile-time embedding.

### 3. Build the firmware

```bash
mkdir build && cd build
cmake -G Ninja -DPICO_SDK_FETCH_FROM_GIT=ON -DPICO_NO_PICOTOOL=1 ..
ninja
```

> **Note:** `-DPICO_NO_PICOTOOL=1` works around a known Windows regression in picotool 2.3.0 ([raspberrypi/picotool#355](https://github.com/raspberrypi/picotool/issues/355)) that crashes with an access violation during the `coprodis` post-processing step. If it's skipped, convert the `.elf` to `.uf2` manually with an older picotool build (2.2.0-a4).

### 4. Flash it

Hold **BOOTSEL**, plug in the Pico 2 W, release BOOTSEL, then drag `secure_boot_app.uf2` onto the `RPI-RP2` drive that appears.

### 5. Watch it verify itself

```bash
python -m serial.tools.miniterm <YOUR_COM_PORT> 115200
```

Expected output on a valid, signed build:

```
RP2350 Secure Bootloader Initializing
[SECURE BOOT SUCCESS] Signature Verified!
[SECURE BOOT] Booting verified firmware payload...
Running verified firmware payload logic...
```
![Secure boot success](</images/verification pass.png>)

## The Tamper Test

To prove the board actually rejects invalid firmware — not just accepts good ones — a byte was flipped in the embedded payload, then rebuilt and reflashed:

```
RP2350 Secure Bootloader Initializing
[CRITICAL ERROR] Signature Verification Failed: ECP - The signature is not valid (-0x4e00)
[SECURE BOOT] Boot execution aborted. Device locked.
```

![Secure boot failure](</images/Tamper test.png>)

The board halts permanently rather than falling through to execution. Restoring the original signed payload and reflashing returns it to the passing state above.

## CI/CD Pipeline

Every push to `main` triggers `.github/workflows/sec-pipeline.yml`, running three jobs in sequence:

1. **SAST + SBOM + CVE Scan** — Cppcheck, Syft, Grype
2. **Build Firmware & Sign Binary** — compiles via CMake/Ninja, signs with the private key from GitHub Secrets
3. **Create GitHub Release** — publishes the signed `.uf2`, `sbom.json`, `cppcheck_report.txt`, and `firmware_signature.der` as release assets

See the [Releases](../../releases) page for signed builds and their attached compliance artifacts.

## Known Limitations

- **SBOM auto-detection** — Syft's scanners rely on package manifests (e.g. `package.json`, `requirements.txt`); this project vendors dependencies via the Pico SDK's CMake `FetchContent` mechanism, which isn't auto-detected (0 packages found when scoped to source only). Vendored components (Pico SDK 2.3.0, mbedTLS) are manually documented in `sbom_manual_notes.md` instead.
- **Single-stage verification** — the bootloader verifies an embedded payload rather than chain-loading into a separately flashed application at a distinct flash offset. True two-stage secure boot (flash partitioning + vector table relocation) is a natural next step, not yet implemented.
- **picotool 2.3.0 Windows regression** — `.uf2` generation on Windows requires either `-DPICO_NO_PICOTOOL=1` (skips auto-generation) plus a manual conversion using picotool 2.2.0-a4, or an equivalent workaround, until upstream fixes the crash.

## Author

**Jennifer Batina**
[GitHub](https://github.com/Batina-Jennifer) · [LinkedIn](https://www.linkedin.com/in/batina-jennifer-458359169/)
