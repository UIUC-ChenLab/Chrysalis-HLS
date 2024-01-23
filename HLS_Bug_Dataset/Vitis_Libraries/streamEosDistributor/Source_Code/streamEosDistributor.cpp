void streamEosDistributor(hls::stream<bool>& inStream, hls::stream<bool> outStream[SLAVES]) {
    do {
        bool i = inStream.read();
        for (int n = 0; n < SLAVES; n++) outStream[n] << i;
        if (i == 1) break;
    } while (1);
}