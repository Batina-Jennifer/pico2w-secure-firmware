#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* Enable SHA-256 */
#define MBEDTLS_SHA256_C

/* Enable Public Key Parsing & ECDSA */
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_OID_C

/* Enable Error Reporting */
#define MBEDTLS_ERROR_C

#endif /* MBEDTLS_CONFIG_H */