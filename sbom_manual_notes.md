# Manually-Supplemented SBOM Components & Supply Chain Audit Trail

## 1. Automated Detection Limitation
Automated scanning via Syft (`cyclonedx-json`) initially found 0 auto-detectable 
packages when scoped to source files. This is a known industry limitation for 
embedded systems: Syft relies on high-level package manifests (`package.json`, 
`requirements.txt`), whereas this project vendors bare-metal C dependencies 
via the Pico SDK's CMake FetchContent mechanism.

## 2. Enriched Component Inventory
To achieve Cyber Resilience Act (CRA) compliance and enable automated CVE tracking via Grype, 
the primary `sbom.json` was manually enriched with explicit Package URLs (PURLs) and CPE identifiers:

| Component | Version / Commit | PURL / CPE Identifier |
|---|---|---|
| Raspberry Pi Pico SDK | 2.3.0 | `pkg:github/raspberrypi/pico-sdk@2.3.0` |
| Mbed TLS (via Pico SDK) | 3.6.6 (`82418f4`) | `pkg:github/mbed-tls/mbedtls@3.6.6`<br>`cpe:2.3:a:arm:mbed_tls:3.6.6:*:*:*:*:*:*:*` |

## 3. Vulnerability Triage
With CPE identifiers mapped, automated scans using Grype evaluate upstream security advisories on every CI/CD run. Identified findings (such as `CVE-2025-66442`) are formally assessed and triaged in `.grype.yaml`.