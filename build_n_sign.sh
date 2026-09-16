#!/bin/bash
set -e

#!/bin/bash
set -e

echo "=== [1/4] Generating Key Formats & Hashing Payload ==="
openssl pkey -pubin -in ec_public_key.pem -outform DER -out ec_public_key.der
openssl dgst -sha256 -binary -out payload_hash.bin firmware_payload.bin
openssl dgst -sha256 -sign ec_private_key.pem -out signature.der firmware_payload.bin

echo "=== [2/4] Generating C Header Files ==="
echo -e "#ifndef PUBLIC_KEY_H\n#define PUBLIC_KEY_H\n#include <stddef.h>\n#include <stdint.h>" > public_key.h
xxd -i ec_public_key.der | sed 's/ec_public_key_der/public_key_h/g' >> public_key.h
echo "#endif" >> public_key.h

echo -e "#ifndef PAYLOAD_H\n#define PAYLOAD_H\n#include <stddef.h>\n#include <stdint.h>" > payload.h
xxd -i payload_hash.bin | sed 's/payload_hash_bin/payload_hash_bin/g' >> payload.h
echo "#endif" >> payload.h

echo -e "#ifndef SIGNATURE_H\n#define SIGNATURE_H\n#include <stddef.h>\n#include <stdint.h>" > signature.h
xxd -i signature.der | sed 's/signature_der/signature_der/g' >> signature.h
echo "#endif" >> signature.h

echo "=== [3/4] Compiling Firmware with Ninja ==="
cd build
ninja

echo "=== [4/4] Build & Sign Complete ==="
echo "Output binary: build/secure_boot_app.uf2"