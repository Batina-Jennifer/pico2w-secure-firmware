# RP2350 Secure Firmware Update & Automated DevSecOps Pipeline

An automated, CRA-compliant supply chain security pipeline and application-level secure bootloader for the **Raspberry Pi Pico 2 W (RP2350)**.


![Status](https://img.shields.io/badge/status-working-brightgreen)
![Hardware](https://img.shields.io/badge/hardware-RP2350-blue)

---

## Overview

This project builds an end-to-end secure firmware deployment pipeline for a Raspberry Pi Pico 2 W, aligned with EU Cyber Resilience Act (CRA) supply chain security requirements.

The board verifies incoming firmware against embedded cryptographic signatures before execution. Every code push is automatically scanned for secrets and code vulnerabilities, inventoried into a Software Bill of Materials (SBOM), audited against known CVEs, compiled, and signed — before the binary ever reaches the hardware.

## Architecture

```
DEVELOPMENT & WORKSPACE (LOCAL)
1. Developer writes C firmware & embeds public_key.h
                │
                ▼ git push
DEVSECOPS PIPELINE (GITHUB ACTIONS)
2. Secret Scanning (Gitleaks) & Static Analysis (Cppcheck SAST)
3. Enriched Software Bill of Materials Audit (Syft / CycloneDX)
4. Vulnerability & Dependency CVE Scanning (Grype & .grype.yaml)
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
- **Fail-secure design** — any verification failure halts the board permanently in a hardware lock loop
- **Automated Secret Scanning** — Gitleaks audits full repository history on every push to prevent secret leaks
- **Automated SAST** — Cppcheck enforces C code safety and style standards prior to compilation
- **Enriched SBOM & CRA Compliance** — Machine-readable CycloneDX `sbom.json` containing explicit PURL/CPE metadata for bare-metal dependencies.
- **Automated CVE Scanning & Risk Triage** — Grype cross-references the SBOM against vulnerability databases while applying formal risk decisions documented in .grype.yaml.
- **Automated signing & release** — GitHub Actions signs compiled binaries using OpenSSL keys stored in GitHub Secrets, publishing a GitHub Release with all compliance artifacts attached.

## Tech Stack

| Category | Tools |
| --- | --- |
| Hardware | Raspberry Pi Pico 2 W (RP2350) |
| Firmware | C, Pico SDK 2.3.0, mbedTLS |
| Cryptography | OpenSSL (ECDSA P-256, SHA-256) |
| Secret Scanning | Gitleaks |
| SAST | Cppcheck |
| SBOM | Syft (CycloneDX 1.7) |
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

## Security & Supply Chain Audit

### 1. Secret Isolation & Key Management
- **Zero Local Key Leaks:** Verified via `git log --all -- *.pem` that `ec_private_key.pem` was never tracked or committed to Git history. Only `.gitignore`, `ec_public_key.pem`, and `public_key.h` exist in historical commits.
- **CI/CD Signing:** Private signing keys are injected strictly at build time via GitHub Secrets (`ECC_PRIVATE_KEY`) and immediately purged from container memory post-signing.

### 2. Software Bill of Materials (SBOM) & Vulnerability Management
- **Manual Component Enrichment:** Initial automated Syft scans returned empty package sets due to bare-metal C vendoring (Pico SDK and Mbed TLS).
- **CRA Alignment:** Manually enriched `sbom.json` with explicit Package URLs (`pkg:github/raspberrypi/pico-sdk@2.3.0` and `pkg:github/mbed-tls/mbedtls@3.6.6`) and CPE identifiers (`cpe:2.3:a:arm:mbed_tls:3.6.6:*:*:*:*:*:*:*`) to enable accurate upstream CVE tracking via Grype.

### 3. Vulnerability Triage & Impact Analysis
- **Identified Finding:** `CVE-2025-66442` (Medium Severity) — LLVM `select-optimize` timing side channel in Mbed TLS RSA and CBC/ECB decryption routines.
![CVE Identified](</images/new vuln.png>)
- **Triage Result:** **Not Affected (Unreachable Code / False Positive)**.
- **Justification:**
  1. *Primitive Scope:* Firmware uses ECDSA P-256 and SHA-256; RSA and CBC/ECB routines are omitted at compile time.
  2. *Toolchain:* Build uses `arm-none-eabi-gcc`, avoiding the vulnerable LLVM optimization pass entirely.
- **Action:** Formally documented risk decision in `.grype.yaml`.
![CVE Fixed](</images/fixed vuln.png>)

### 4. Architecture Terminology & Verification Boundary
- **Verification Boundary:** Firmware payload authenticity is validated via application-level C code executing Mbed TLS ECDSA (NIST P-256).
- **Hardware Root of Trust Note:** This implementation performs software-based signature verification. Achieving a true hardware-enforced Root of Trust on the RP2350 requires provisioning public key hashes directly into One-Time Programmable (OTP) memory to engage the internal boot ROM secure boot flow.

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

Every push to `main` triggers `.github/workflows/sec-pipeline.yml`, executing three gated jobs under a least-privilege security model (`permissions: contents: read`):

1. **SAST + Secret Scan + SBOM Triage:**
   - **Gitleaks:** Scans full repository history to prevent accidental secret or key exposure.
   - **Cppcheck:** Enforces C code quality and style standards (`--error-exitcode=1`).
   - **Syft & Grype:** Evaluates the enriched `sbom.json` against NVD vulnerability databases and enforces the build gate (`--fail-on medium`) while respecting triage rules in `.grype.yaml`.
2. **Build Firmware & Sign Binary:**
   - Compiles the application via CMake/Ninja using `-DPICO_NO_PICOTOOL=1 -DSKIP_PICOTOOL_BUILD=1` to bypass Windows toolchain regressions.
   - Injects `ECC_PRIVATE_KEY` from GitHub Secrets to derive header signatures (`signature.der`) and immediately purges the private key from container disk memory.
3. **Create GitHub Release (`contents: write` scoped):**
   - Publishes the compiled `.uf2` binary, `signature.der`, enriched `sbom.json`, `.grype.yaml` triage record, and `cppcheck_report.txt` directly to the GitHub Releases page.

See the [Releases](../../releases) page for signed builds and their attached compliance artifacts.

## Known Limitations

- **Bare-Metal C SBOM Enrichment** — Automated Syft scans on bare-metal C repositories return empty component sets due to the absence of standard package manifests. Dependencies (Pico SDK v2.3.0 and Mbed TLS v3.6.6) are explicitly enriched with Package URLs (`purl`) and CPEs directly in `sbom.json` to enable accurate upstream vulnerability tracking.
- **Application-Level Firmware Verification** — The bootloader performs cryptographic signature verification on an embedded payload within C code. True hardware-enforced secure boot on the RP2350 requires provisioning public key hashes into One-Time Programmable (OTP) memory to engage the internal boot ROM.
- **picotool Windows Build Regression** — The build pipeline configures CMake with `-DPICO_NO_PICOTOOL=1 -DSKIP_PICOTOOL_BUILD=1` to bypass `picotool.exe coprodis` segmentation faults (`0xC0000005`) observed on Windows host toolchains.

## Author

**Jennifer Batina**
[GitHub](https://github.com/Batina-Jennifer) · [LinkedIn](https://www.linkedin.com/in/batina-jennifer-458359169/)
