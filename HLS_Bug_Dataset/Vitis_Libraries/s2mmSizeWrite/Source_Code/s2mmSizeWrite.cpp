void s2mmSizeWrite(hls::stream<OUTSIZE_DT>& inSizeStream,
                   OUTSIZE_DT* compressedSize,
                   OUTSIZE_DT numItr,
                   OUTSIZE_DT inputSize,
                   OUTSIZE_DT blckSize) {
    OUTSIZE_DT blckNum = (inputSize - 1) / blckSize + 1;
    OUTSIZE_DT idx = 0;
sizewritedummy:
    for (auto z = 0; z < (numItr - 1) * blckNum; z++) {
#pragma HLS PIPELINE
        inSizeStream.read();
        if (TUSR_DWIDTH != 0) inSizeStream.read();
    }
sizewrite:
    for (auto z = 0; z < blckNum; z++) {
#pragma HLS PIPELINE
        OUTSIZE_DT sizeVal = inSizeStream.read();
        compressedSize[idx] = sizeVal;
        if (TUSR_DWIDTH != 0) compressedSize[idx + 1] = inSizeStream.read();
        idx += 2;
    }
}