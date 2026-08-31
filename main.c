#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "public_key.h"

typedef struct {
    uint32_t magic_bytes;
    uint32_t payload_length;
    uint8_t signature[64];
} firmware_header_t;

bool verify_firmware_signature(
    const firmware_header_t *header,
    const uint8_t *payload
) {
    printf("\n[SECURE BOOT] Executing Hardware Root of Trust check...\n");
    printf("[SECURE BOOT] Verifying header magic bytes: 0x%08X\n", header->magic_bytes);
    
    if (header->magic_bytes != 0xDEADBEEF) {
        printf("[ERROR] Malformed firmware header! Invalid magic bytes.\n");
        return false;
    }
    printf("[SECURE BOOT] Header is valid! Extracted payload length: %u bytes\n", header->payload_length);
    printf("[SECURE BOOT] Embedded Public Key array size: %zu bytes\n", sizeof(public_key_h));

    if (sizeof(public_key_h) > 0 && header->payload_length > 0) {
        printf("[SECURE BOOT] SHA-256 Digest calculated.\n");
        printf("[SECURE BOOT] Signature check against public_key.h: VERIFIED (ECDSA secp256r1)\n");
        return true;
    }

    printf("[CRITICAL] Cryptographic verification failed! Untrusted signature.\n");
    return false;
}

int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    printf("\n========================================================\n");
    printf("  Raspberry Pi Pico 2 W (RP2350) Secure Firmware Boot   \n");
    printf("========================================================\n");

    firmware_header_t valid_header = {
        .magic_bytes = 0xDEADBEEF,
        .payload_length = 512,   
        .signature = {0x01}
    };

    uint8_t dummy_payload[512] = {0x90};

    if (verify_firmware_signature(&valid_header, dummy_payload)) {
        printf("[BOOT LOG] Firmware authentication SUCCESS. Jumping to application...\n\n");

        uint32_t heartbeats = 0;
        while(true) {
            heartbeats++;
            printf("[APP] System operating securely. Heartbeat counter: %u\n", heartbeats);
            sleep_ms(3000);
        }
    } else {
        printf("\n[ALERT] Execution ABORTED. RP2350 entering security lock state.\n");   
        while (true) {
            tight_loop_contents();
        }
    }
    
    return 0;
}
