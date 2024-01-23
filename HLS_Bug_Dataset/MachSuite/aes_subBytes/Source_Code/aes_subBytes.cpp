void aes_subBytes(uint8_t *buf)
{
    register uint8_t i = 16;

    sub : while (i--) buf[i] = rj_sbox(buf[i]);
}

// Content of called function
uint8_t rj_sbox(uint8_t x)
{
    uint8_t y, sb;

    sb = y = gf_mulinv(x);
    y = (y<<1)|(y>>7); sb ^= y;  y = (y<<1)|(y>>7); sb ^= y;
    y = (y<<1)|(y>>7); sb ^= y;  y = (y<<1)|(y>>7); sb ^= y;

    return (sb ^ 0x63);
}