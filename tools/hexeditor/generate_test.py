# generate_test_bin.py
data = bytearray()

# 1. Signature (Hex Editor Test)
data.extend(b"HEX_EDITOR_V1.0") 

# 2. Sequential bytes (0x00 to 0xFF)
for i in range(256):
    data.append(i)

# 3. Patterned noise (Repeat 0xAA 0x55)
for _ in range(250):
    data.extend([0xAA, 0x55])

# 4. Fill the rest to 1KB
data.extend(b"\x00" * (1024 - len(data)))

with open("test.bin", "wb") as f:
    f.write(data)

print("Created test.bin (1024 bytes)")
