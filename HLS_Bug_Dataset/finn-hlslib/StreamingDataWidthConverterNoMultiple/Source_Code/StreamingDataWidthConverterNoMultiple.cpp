void StreamingDataWidthConverterNoMultiple(
    hls::stream<ap_uint<InWidth> > & in,
    hls::stream<ap_uint<OutWidth> > & out) {
    static_assert((InWidth % 2) == 0, "");
    static_assert((OutWidth % 2) == 0, "");
    static_assert(InWidth != OutWidth, "");
    static unsigned int      offset = 0; 

    if (InWidth > OutWidth){
     
      static ap_uint<OutWidth> remainder = 0;
      ap_uint<InWidth>  valueIn = in.read();
      
      if(offset !=0) {
        ap_uint<OutWidth>   valueOut = 0;
        valueOut = (valueIn(offset-1,0),remainder(OutWidth-offset-1,0));
        valueIn = valueIn(InWidth-1,offset); // leave the next part prepared 
        out.write(valueOut);
      }
      for (; offset <= (InWidth-OutWidth) ; offset+=OutWidth){
        ap_uint<OutWidth>   valueOut = valueIn(OutWidth-1,0);
        valueIn = valueIn(InWidth-1,OutWidth); // leave the next part prepared 
        out.write(valueOut);
      }
      remainder = valueIn;
      if (offset == InWidth)
        offset = 0;
      else
        offset = offset + OutWidth - InWidth;
    }
    else {
      /*OutWidth > InWidth*/
      static ap_uint<InWidth> remainder = 0;
      ap_uint<OutWidth> value = 0;
      if (offset !=0) {
        value(offset-1,0) = remainder(InWidth-1,InWidth-offset);
      }
      for (; offset <= (OutWidth-InWidth); offset+=InWidth){
        ap_uint<InWidth>   aux = in.read();
        value(offset+InWidth-1,offset) = aux;
      }
      if (offset != OutWidth){
        ap_uint<InWidth>   aux = in.read();
        value(OutWidth-1,offset) = aux(OutWidth-offset-1,0);
        remainder                   = aux;
        offset = offset + InWidth - OutWidth;
      }
      else
        offset = 0;
      out.write(value);
    }

}