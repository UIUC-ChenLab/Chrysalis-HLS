void MultiChanDataWidthConverter_Batch(
	hls::stream<MultiChanData<NumVecs, InWidth> > & in,
	hls::stream<MultiChanData<NumVecs, OutWidth> > & out,
	const unsigned int numReps) {
	static_assert((InWidth % OutWidth == 0) || (OutWidth % InWidth == 0), "");
	if (InWidth > OutWidth) {
		// emit multiple output words per input word read
		const unsigned int outPerIn = InWidth / OutWidth;
		const unsigned int totalIters = NumInWords * outPerIn * numReps;
		unsigned int o = 0;
		MultiChanData<NumVecs, InWidth> ei;
		for (unsigned int t = 0; t < totalIters; t++) {
#pragma HLS pipeline style=flp II=1
			// read new input word if current out count is zero
			if (o == 0)
				ei = in.read();
			// pick output word from the rightmost position
			MultiChanData<NumVecs, OutWidth> eo;
			for(unsigned int v = 0; v < NumVecs; v++) {
#pragma HLS UNROLL
				eo.data[v] = (ei.data[v])(OutWidth - 1, 0);
				// shift input to get new output word for next iteration
				ei.data[v] = ei.data[v] >> OutWidth;
			}
			out.write(eo);
			// increment written output count
			o++;
			// wraparound indices to recreate the nested loop structure
			if (o == outPerIn) {
				o = 0;
			}
		}
	} else if (InWidth == OutWidth) {
		// straight-through copy
		for (unsigned int i = 0; i < NumInWords * numReps; i++) {
#pragma HLS pipeline style=flp II=1
			MultiChanData<NumVecs, InWidth> e = in.read();
			MultiChanData<NumVecs, OutWidth> eo;
			// we don't support typecasting between templated types, so explicitly
			// transfer vector-by-vector here
			for(unsigned int v=0; v < NumVecs; v++) {
#pragma HLS UNROLL
				eo.data[v] = e.data[v];
			}
			out.write(eo);
		}
	} else { // InWidth < OutWidth
		// read multiple input words per output word emitted
		const unsigned int inPerOut = OutWidth / InWidth;
		const unsigned int totalIters = NumInWords * numReps;
		unsigned int i = 0;
		MultiChanData<NumVecs, OutWidth> eo;
		for (unsigned int t = 0; t < totalIters; t++) {
#pragma HLS pipeline style=flp II=1
			// read input and shift into output buffer
			MultiChanData<NumVecs, InWidth> ei = in.read();
			for(unsigned int v = 0; v < NumVecs; v++) {
#pragma HLS UNROLL
				eo.data[v] = eo.data[v] >> InWidth;
				(eo.data[v])(OutWidth - 1, OutWidth - InWidth) = ei.data[v];
			}
			// increment read input count
			i++;
			// wraparound logic to recreate nested loop functionality
			if (i == inPerOut) {
				i = 0;
				out.write(eo);
			}
		}
	}
}