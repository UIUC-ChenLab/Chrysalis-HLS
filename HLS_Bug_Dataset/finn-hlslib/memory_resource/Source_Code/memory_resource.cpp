void memory_resource(T inputBuf, ap_resource_dflt const&){
#pragma HLS BIND_STORAGE variable=inputBuf type=RAM_2P
}