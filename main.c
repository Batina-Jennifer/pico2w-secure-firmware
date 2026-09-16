#include <stdio.h>
#include "pico/stdlib.h"

/* Mbed TLS Headers */
#include "mbedtls/pk.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/error.h"

/* Local Header files containing keys, signatures, and payloads */
#include "public_key.h"
#include "signature.h"
#include "payload.h"

static void boot_failed_halt(const char *reason, int err_code) {
    char err_buf[128];
    mbedtls_strerror(err_code, err_buf, sizeof(err_buf));
    printf("\n[CRITICAL ERROR] %s: %s (-0x%04x)\n", reason, err_buf, -err_code);
    printf("[SECURE BOOT] Boot execution aborted. Device locked.\n");
    
    /* Hardware Execution Trap: Infinite Halt Loop */
    while (1) {
        tight_loop_contents();
    }
}

int main() {
    stdio_init_all();
    
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    
    sleep_ms(1000);

    printf("=========================================\n");
    printf("  RP2350 Secure Bootloader Initializing  \n");
    printf("=========================================\n");

    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    /* 1. Parse DER-encoded Public Key */
    int ret = mbedtls_pk_parse_public_key(&pk, public_key_h, public_key_h_len);
    if (ret != 0) {
        boot_failed_halt("Public Key Parsing Failed", ret);
    }

    /* 2. Extract ECP/ECDSA context pointer using mbedtls_pk_ec macro */
    mbedtls_ecdsa_context *ecdsa = mbedtls_pk_ec(pk);
    if (ecdsa == NULL) {
        printf("[CRITICAL ERROR] Failed to extract ECDSA context.\n");
        while (1) { tight_loop_contents(); }
    }

    /* 3. Verify Signature against Payload Hash */
    ret = mbedtls_ecdsa_read_signature(
        ecdsa,
        payload_hash_bin, sizeof(payload_hash_bin),
        signature_der, signature_der_len
    );

    if (ret != 0) {
        mbedtls_pk_free(&pk);
        boot_failed_halt("Signature Verification Failed", ret);
    }

    /* 4. Verification Passed -> Launch Payload */
    mbedtls_pk_free(&pk);
    printf("[SECURE BOOT SUCCESS] Signature Verified!\n");
    printf("[SECURE BOOT] Booting verified firmware payload...\n");

    /* Simulated Firmware Entry Point */
    while (1) {
        printf("Running verified firmware payload logic...\n");
        sleep_ms(2000);
    }
    return 0;
}