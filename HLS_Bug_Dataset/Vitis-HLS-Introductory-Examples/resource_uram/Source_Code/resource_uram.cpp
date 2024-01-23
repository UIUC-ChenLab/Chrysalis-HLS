#define NWORDS 1 << ADDRBITS

void resource_uram(bool wren, bool rden, addr_t addrW, data_t datain,
                   addr_t AddrR, data_t* dataout) {
#pragma HLS PIPELINE II = 1

    static data_t buffer[NWORDS];
#pragma HLS DEPENDENCE variable = buffer inter WAR false
#pragma HLS BIND_STORAGE variable = buffer type = ram_2p impl = uram

    if (rden)
        *dataout = buffer[AddrR];

    if (wren)
        buffer[addrW] = datain;
}