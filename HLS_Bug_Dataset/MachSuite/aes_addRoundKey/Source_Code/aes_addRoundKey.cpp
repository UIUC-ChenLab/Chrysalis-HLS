void aes_addRoundKey(uint8_t *buf, uint8_t *key)
{
    register uint8_t i = 16;

    addkey : while (i--) buf[i] ^= key[i];
}