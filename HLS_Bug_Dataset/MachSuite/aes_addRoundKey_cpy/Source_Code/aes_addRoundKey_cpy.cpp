void aes_addRoundKey_cpy(uint8_t *buf, uint8_t *key, uint8_t *cpk)
{
    register uint8_t i = 16;

    cpkey : while (i--)  buf[i] ^= (cpk[i] = key[i]), cpk[16+i] = key[16 + i];
}