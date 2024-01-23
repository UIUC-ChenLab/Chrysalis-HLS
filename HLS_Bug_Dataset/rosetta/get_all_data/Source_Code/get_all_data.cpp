void get_all_data(uint18_t output[12], uint10_t addr_list[12], uint18_t aa[ROWS*COLS])
{
  
  #pragma HLS inline
  #pragma HLS interface ap_stable port=aa

  uint5_t bank[12];
  #pragma HLS array_partition variable=bank complete dim=0
  uint5_t offset[12];
  #pragma HLS array_partition variable=offset complete dim=0
  uint5_t offset_for_banks[28];
  #pragma HLS array_partition variable=offset_for_banks complete dim=0
  uint18_t data_from_banks[28];
  #pragma HLS array_partition variable=data_from_banks complete dim=0
  
COMPUTE_BANK_AND_OFFSET:
  for (int i = 0; i < 12; i ++ )
  {
    #pragma HLS unroll   
    bank[i] = get_bank(addr_list[i]);
    offset[i] = get_offset(addr_list[i]);
  }

ASSIGN_OFFSET_FOR_BANKS:
  for (int i = 0; i < 28; i ++ )
  {
    #pragma HLS unroll
    offset_for_banks[i] = (bank[ 0] == i) ? offset[ 0] :
                          (bank[ 1] == i) ? offset[ 1] :
                          (bank[ 2] == i) ? offset[ 2] :
                          (bank[ 3] == i) ? offset[ 3] :
                          (bank[ 4] == i) ? offset[ 4] :
                          (bank[ 5] == i) ? offset[ 5] :
                          (bank[ 6] == i) ? offset[ 6] :
                          (bank[ 7] == i) ? offset[ 7] :
                          (bank[ 8] == i) ? offset[ 8] :
                          (bank[ 9] == i) ? offset[ 9] :
                          (bank[10] == i) ? offset[10] :
                          (bank[11] == i) ? offset[11] :
                          uint5_t("0",10);

  }

READ_ALL_BANKS:
  data_from_banks[ 0] =  get_data0 (offset_for_banks[ 0], aa);
  data_from_banks[ 1] =  get_data1 (offset_for_banks[ 1], aa);
  data_from_banks[ 2] =  get_data2 (offset_for_banks[ 2], aa);
  data_from_banks[ 3] =  get_data3 (offset_for_banks[ 3], aa);
  data_from_banks[ 4] =  get_data4 (offset_for_banks[ 4], aa);
  data_from_banks[ 5] =  get_data5 (offset_for_banks[ 5], aa);
  data_from_banks[ 6] =  get_data6 (offset_for_banks[ 6], aa);
  data_from_banks[ 7] =  get_data7 (offset_for_banks[ 7], aa);
  data_from_banks[ 8] =  get_data8 (offset_for_banks[ 8], aa);
  data_from_banks[ 9] =  get_data9 (offset_for_banks[ 9], aa);
  data_from_banks[10] =  get_data10(offset_for_banks[10], aa);
  data_from_banks[11] =  get_data11(offset_for_banks[11], aa);
  data_from_banks[12] =  get_data12(offset_for_banks[12], aa);
  data_from_banks[13] =  get_data13(offset_for_banks[13], aa);
  data_from_banks[14] =  get_data14(offset_for_banks[14], aa);
  data_from_banks[15] =  get_data15(offset_for_banks[15], aa);
  data_from_banks[16] =  get_data16(offset_for_banks[16], aa);
  data_from_banks[17] =  get_data17(offset_for_banks[17], aa);
  data_from_banks[18] =  get_data18(offset_for_banks[18], aa);
  data_from_banks[19] =  get_data19(offset_for_banks[19], aa);
  data_from_banks[20] =  get_data20(offset_for_banks[20], aa);
  data_from_banks[21] =  get_data21(offset_for_banks[21], aa);
  data_from_banks[22] =  get_data22(offset_for_banks[22], aa);
  data_from_banks[23] =  get_data23(offset_for_banks[23], aa);
  data_from_banks[24] =  get_data24(offset_for_banks[24], aa);
  data_from_banks[25] =  get_data25(offset_for_banks[25], aa);
  data_from_banks[26] =  get_data26(offset_for_banks[26], aa);
  data_from_banks[27] =  get_data27(offset_for_banks[27], aa);

CHOOSE_DATA:
  for (int i = 0; i < 12; i ++ )
  {
    #pragma HLS unroll
    output[i] = data_from_banks[bank[i]];
  }

}

// Content of called function
uint18_t get_data14(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[3  ]; break;
    case 1: a = aa[10 ]; break;
    case 2: a = aa[17 ]; break;
    case 3: a = aa[35 ]; break;
    case 4: a = aa[66 ]; break;
    case 5: a = aa[99 ]; break;
    case 6: a = aa[152]; break;
    case 7: a = aa[178]; break;
    case 8: a = aa[248]; break;
    case 9: a = aa[259]; break;
    case 10:a = aa[270]; break;
    case 11:a = aa[290]; break;
    case 12:a = aa[321]; break;
    case 13:a = aa[336]; break;
    case 14:a = aa[337]; break;
    case 15:a = aa[361]; break;
    case 16:a = aa[382]; break;
    case 17:a = aa[393]; break;
    case 18:a = aa[416]; break;
    case 19:a = aa[473]; break;
    case 20:a = aa[502]; break;
    case 21:a = aa[545]; break;
    case 22:a = aa[600]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data5(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[23 ]; break;
    case 1: a = aa[53 ]; break;
    case 2: a = aa[94 ]; break;
    case 3: a = aa[95 ]; break;
    case 4: a = aa[101]; break;
    case 5: a = aa[139]; break;
    case 6: a = aa[171]; break;
    case 7: a = aa[180]; break;
    case 8: a = aa[222]; break;
    case 9: a = aa[267]; break;
    case 10:a = aa[275]; break;
    case 11:a = aa[311]; break;
    case 12:a = aa[312]; break;
    case 13:a = aa[333]; break;
    case 14:a = aa[365]; break;
    case 15:a = aa[390]; break;
    case 16:a = aa[397]; break;
    case 17:a = aa[409]; break;
    case 18:a = aa[410]; break;
    case 19:a = aa[426]; break;
    case 20:a = aa[443]; break;
    case 21:a = aa[463]; break;
    case 22:a = aa[537]; break;
    case 23:a = aa[571]; break;
    case 24:a = aa[599]; break;
    case 25:a = aa[608]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data0(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[19 ]; break;
    case 1: a = aa[29 ]; break;
    case 2: a = aa[52 ]; break;
    case 3: a = aa[100]; break;
    case 4: a = aa[132]; break;
    case 5: a = aa[161]; break;
    case 6: a = aa[193]; break;
    case 7: a = aa[220]; break;
    case 8: a = aa[239]; break;
    case 9: a = aa[253]; break;
    case 10:a = aa[284]; break;
    case 11:a = aa[309]; break;
    case 12:a = aa[362]; break;
    case 13:a = aa[385]; break;
    case 14:a = aa[396]; break;
    case 15:a = aa[447]; break;
    case 16:a = aa[448]; break;
    case 17:a = aa[449]; break;
    case 18:a = aa[451]; break;
    case 19:a = aa[466]; break;
    case 20:a = aa[492]; break;
    case 21:a = aa[531]; break;
    case 22:a = aa[562]; break;
    default: a = 0; break;
  }; 

  return a;
}

// Content of called function
uint18_t get_data25(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[33 ]; break;
    case 1: a = aa[51 ]; break;
    case 2: a = aa[64 ]; break;
    case 3: a = aa[78 ]; break;
    case 4: a = aa[86 ]; break;
    case 5: a = aa[110]; break;
    case 6: a = aa[130]; break;
    case 7: a = aa[216]; break;
    case 8: a = aa[254]; break;
    case 9: a = aa[298]; break;
    case 10:a = aa[320]; break;
    case 11:a = aa[402]; break;
    case 12:a = aa[419]; break;
    case 13:a = aa[438]; break;
    case 14:a = aa[446]; break;
    case 15:a = aa[455]; break;
    case 16:a = aa[491]; break;
    case 17:a = aa[500]; break;
    case 18:a = aa[590]; break;
    case 19:a = aa[622]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data24(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[31 ]; break;
    case 1: a = aa[84 ]; break;
    case 2: a = aa[113]; break;
    case 3: a = aa[116]; break;
    case 4: a = aa[129]; break;
    case 5: a = aa[158]; break;
    case 6: a = aa[182]; break;
    case 7: a = aa[227]; break;
    case 8: a = aa[276]; break;
    case 9: a = aa[380]; break;
    case 10:a = aa[404]; break;
    case 11:a = aa[460]; break;
    case 12:a = aa[470]; break;
    case 13:a = aa[493]; break;
    case 14:a = aa[494]; break;
    case 15:a = aa[503]; break;
    case 16:a = aa[514]; break;
    case 17:a = aa[566]; break;
    case 18:a = aa[580]; break;
    case 19:a = aa[602]; break;
    case 20:a = aa[617]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data23(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[37 ]; break;
    case 1: a = aa[50 ]; break;
    case 2: a = aa[88 ]; break;
    case 3: a = aa[114]; break;
    case 4: a = aa[134]; break;
    case 5: a = aa[189]; break;
    case 6: a = aa[205]; break;
    case 7: a = aa[214]; break;
    case 8: a = aa[236]; break;
    case 9: a = aa[273]; break;
    case 10:a = aa[297]; break;
    case 11:a = aa[349]; break;
    case 12:a = aa[354]; break;
    case 13:a = aa[432]; break;
    case 14:a = aa[457]; break;
    case 15:a = aa[477]; break;
    case 16:a = aa[498]; break;
    case 17:a = aa[512]; break;
    case 18:a = aa[605]; break;
    case 19:a = aa[616]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data15(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[8  ]; break;
    case 1: a = aa[25 ]; break;
    case 2: a = aa[30 ]; break;
    case 3: a = aa[57 ]; break;
    case 4: a = aa[120]; break;
    case 5: a = aa[123]; break;
    case 6: a = aa[169]; break;
    case 7: a = aa[192]; break;
    case 8: a = aa[217]; break;
    case 9: a = aa[241]; break;
    case 10:a = aa[271]; break;
    case 11:a = aa[274]; break;
    case 12:a = aa[285]; break;
    case 13:a = aa[306]; break;
    case 14:a = aa[327]; break;
    case 15:a = aa[368]; break;
    case 16:a = aa[403]; break;
    case 17:a = aa[434]; break;
    case 18:a = aa[474]; break;
    case 19:a = aa[476]; break;
    case 20:a = aa[504]; break;
    case 21:a = aa[538]; break;
    case 22:a = aa[563]; break;
    case 23:a = aa[568]; break;
    case 24:a = aa[596]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data4(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[24 ]; break;
    case 1: a = aa[34 ]; break;
    case 2: a = aa[47 ]; break;
    case 3: a = aa[58 ]; break;
    case 4: a = aa[105]; break;
    case 5: a = aa[128]; break;
    case 6: a = aa[162]; break;
    case 7: a = aa[179]; break;
    case 8: a = aa[218]; break;
    case 9: a = aa[226]; break;
    case 10:a = aa[346]; break;
    case 11:a = aa[364]; break;
    case 12:a = aa[369]; break;
    case 13:a = aa[388]; break;
    case 14:a = aa[406]; break;
    case 15:a = aa[425]; break;
    case 16:a = aa[440]; break;
    case 17:a = aa[453]; break;
    case 18:a = aa[458]; break;
    case 19:a = aa[486]; break;
    case 20:a = aa[510]; break;
    case 21:a = aa[552]; break;
    case 22:a = aa[594]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data10(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[6  ]; break;
    case 1: a = aa[40 ]; break;
    case 2: a = aa[103]; break;
    case 3: a = aa[146]; break;
    case 4: a = aa[173]; break;
    case 5: a = aa[174]; break;
    case 6: a = aa[232]; break;
    case 7: a = aa[268]; break;
    case 8: a = aa[279]; break;
    case 9: a = aa[341]; break;
    case 10:a = aa[374]; break;
    case 11:a = aa[386]; break;
    case 12:a = aa[405]; break;
    case 13:a = aa[420]; break;
    case 14:a = aa[439]; break;
    case 15:a = aa[471]; break;
    case 16:a = aa[488]; break;
    case 17:a = aa[509]; break;
    case 18:a = aa[526]; break;
    case 19:a = aa[612]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data9(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[14 ]; break;
    case 1: a = aa[46 ]; break;
    case 2: a = aa[119]; break;
    case 3: a = aa[147]; break;
    case 4: a = aa[150]; break;
    case 5: a = aa[151]; break;
    case 6: a = aa[163]; break;
    case 7: a = aa[185]; break;
    case 8: a = aa[198]; break;
    case 9: a = aa[242]; break;
    case 10:a = aa[262]; break;
    case 11:a = aa[286]; break;
    case 12:a = aa[302]; break;
    case 13:a = aa[315]; break;
    case 14:a = aa[340]; break;
    case 15:a = aa[343]; break;
    case 16:a = aa[358]; break;
    case 17:a = aa[359]; break;
    case 18:a = aa[429]; break;
    case 19:a = aa[481]; break;
    case 20:a = aa[489]; break;
    case 21:a = aa[507]; break;
    case 22:a = aa[520]; break;
    case 23:a = aa[525]; break;
    case 24:a = aa[572]; break;
    case 25:a = aa[577]; break;
    case 26:a = aa[591]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data6(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[15 ]; break;
    case 1: a = aa[42 ]; break;
    case 2: a = aa[55 ]; break;
    case 3: a = aa[122]; break;
    case 4: a = aa[138]; break;
    case 5: a = aa[177]; break;
    case 6: a = aa[204]; break;
    case 7: a = aa[215]; break;
    case 8: a = aa[228]; break;
    case 9: a = aa[231]; break;
    case 10:a = aa[250]; break;
    case 11:a = aa[287]; break;
    case 12:a = aa[307]; break;
    case 13:a = aa[308]; break;
    case 14:a = aa[366]; break;
    case 15:a = aa[391]; break;
    case 16:a = aa[411]; break;
    case 17:a = aa[424]; break;
    case 18:a = aa[435]; break;
    case 19:a = aa[468]; break;
    case 20:a = aa[497]; break;
    case 21:a = aa[539]; break;
    case 22:a = aa[555]; break;
    case 23:a = aa[609]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data26(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[70 ]; break;
    case 1: a = aa[89 ]; break;
    case 2: a = aa[115]; break;
    case 3: a = aa[127]; break;
    case 4: a = aa[142]; break;
    case 5: a = aa[272]; break;
    case 6: a = aa[348]; break;
    case 7: a = aa[370]; break;
    case 8: a = aa[379]; break;
    case 9: a = aa[430]; break;
    case 10:a = aa[461]; break;
    case 11:a = aa[485]; break;
    case 12:a = aa[513]; break;
    case 13:a = aa[541]; break;
    case 14:a = aa[550]; break;
    case 15:a = aa[583]; break;
    case 16:a = aa[607]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data7(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[38 ]; break;
    case 1: a = aa[82 ]; break;
    case 2: a = aa[93 ]; break;
    case 3: a = aa[135]; break;
    case 4: a = aa[159]; break;
    case 5: a = aa[195]; break;
    case 6: a = aa[212]; break;
    case 7: a = aa[237]; break;
    case 8: a = aa[238]; break;
    case 9: a = aa[258]; break;
    case 10:a = aa[269]; break;
    case 11:a = aa[283]; break;
    case 12:a = aa[310]; break;
    case 13:a = aa[328]; break;
    case 14:a = aa[355]; break;
    case 15:a = aa[421]; break;
    case 16:a = aa[427]; break;
    case 17:a = aa[465]; break;
    case 18:a = aa[523]; break;
    case 19:a = aa[547]; break;
    case 20:a = aa[557]; break;
    case 21:a = aa[570]; break;
    case 22:a = aa[593]; break;
    case 23:a = aa[606]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data12(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[5  ]; break;
    case 1: a = aa[39 ]; break;
    case 2: a = aa[54 ]; break;
    case 3: a = aa[61 ]; break;
    case 4: a = aa[75 ]; break;
    case 5: a = aa[76 ]; break;
    case 6: a = aa[106]; break;
    case 7: a = aa[140]; break;
    case 8: a = aa[165]; break;
    case 9: a = aa[209]; break;
    case 10:a = aa[245]; break;
    case 11:a = aa[246]; break;
    case 12:a = aa[316]; break;
    case 13:a = aa[347]; break;
    case 14:a = aa[412]; break;
    case 15:a = aa[413]; break;
    case 16:a = aa[444]; break;
    case 17:a = aa[464]; break;
    case 18:a = aa[490]; break;
    case 19:a = aa[530]; break;
    case 20:a = aa[534]; break;
    case 21:a = aa[535]; break;
    case 22:a = aa[560]; break;
    case 23:a = aa[586]; break;
    case 24:a = aa[618]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data17(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[11 ]; break;
    case 1: a = aa[67 ]; break;
    case 2: a = aa[77 ]; break;
    case 3: a = aa[104]; break;
    case 4: a = aa[125]; break;
    case 5: a = aa[160]; break;
    case 6: a = aa[203]; break;
    case 7: a = aa[207]; break;
    case 8: a = aa[243]; break;
    case 9: a = aa[244]; break;
    case 10:a = aa[264]; break;
    case 11:a = aa[299]; break;
    case 12:a = aa[323]; break;
    case 13:a = aa[367]; break;
    case 14:a = aa[400]; break;
    case 15:a = aa[401]; break;
    case 16:a = aa[441]; break;
    case 17:a = aa[456]; break;
    case 18:a = aa[480]; break;
    case 19:a = aa[528]; break;
    case 20:a = aa[579]; break;
    case 21:a = aa[589]; break;
    case 22:a = aa[619]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data21(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[4  ]; break;
    case 1: a = aa[32 ]; break;
    case 2: a = aa[73 ]; break;
    case 3: a = aa[81 ]; break;
    case 4: a = aa[108]; break;
    case 5: a = aa[172]; break;
    case 6: a = aa[190]; break;
    case 7: a = aa[194]; break;
    case 8: a = aa[224]; break;
    case 9: a = aa[266]; break;
    case 10:a = aa[318]; break;
    case 11:a = aa[338]; break;
    case 12:a = aa[360]; break;
    case 13:a = aa[392]; break;
    case 14:a = aa[437]; break;
    case 15:a = aa[475]; break;
    case 16:a = aa[505]; break;
    case 17:a = aa[574]; break;
    case 18:a = aa[601]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint5_t get_offset(uint10_t addr)
{
  #pragma HLS inline
  return offset_mapping[addr];
}

// Content of called function
uint18_t get_data22(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[71 ]; break;
    case 1: a = aa[85 ]; break;
    case 2: a = aa[92 ]; break;
    case 3: a = aa[124]; break;
    case 4: a = aa[133]; break;
    case 5: a = aa[143]; break;
    case 6: a = aa[166]; break;
    case 7: a = aa[211]; break;
    case 8: a = aa[225]; break;
    case 9: a = aa[304]; break;
    case 10:a = aa[305]; break;
    case 11:a = aa[351]; break;
    case 12:a = aa[352]; break;
    case 13:a = aa[407]; break;
    case 14:a = aa[423]; break;
    case 15:a = aa[431]; break;
    case 16:a = aa[472]; break;
    case 17:a = aa[495]; break;
    case 18:a = aa[515]; break;
    case 19:a = aa[549]; break;
    case 20:a = aa[553]; break;
    case 21:a = aa[558]; break;
    case 22:a = aa[588]; break;
    case 23:a = aa[614]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data1(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[7  ]; break;
    case 1: a = aa[18 ]; break;
    case 2: a = aa[65 ]; break;
    case 3: a = aa[72 ]; break;
    case 4: a = aa[148]; break;
    case 5: a = aa[149]; break;
    case 6: a = aa[153]; break;
    case 7: a = aa[164]; break;
    case 8: a = aa[191]; break;
    case 9: a = aa[208]; break;
    case 10:a = aa[229]; break;
    case 11:a = aa[240]; break;
    case 12:a = aa[251]; break;
    case 13:a = aa[256]; break;
    case 14:a = aa[280]; break;
    case 15:a = aa[384]; break;
    case 16:a = aa[450]; break;
    case 17:a = aa[478]; break;
    case 18:a = aa[506]; break;
    case 19:a = aa[517]; break;
    case 20:a = aa[527]; break;
    case 21:a = aa[542]; break;
    case 22:a = aa[554]; break;
    case 23:a = aa[573]; break;
    case 24:a = aa[576]; break;
    case 25:a = aa[621]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data27(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[36 ]; break;
    case 1: a = aa[91 ]; break;
    case 2: a = aa[98 ]; break;
    case 3: a = aa[107]; break;
    case 4: a = aa[176]; break;
    case 5: a = aa[202]; break;
    case 6: a = aa[278]; break;
    case 7: a = aa[467]; break;
    case 8: a = aa[482]; break;
    case 9: a = aa[546]; break;
    case 10:a = aa[556]; break;
    case 11:a = aa[569]; break;
    case 12:a = aa[592]; break;
    case 13:a = aa[620]; break;
    default: a = 0; break;
  }; return a;
}

// Content of called function
uint18_t get_data13(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[1  ]; break;
    case 1: a = aa[27 ]; break;
    case 2: a = aa[59 ]; break;
    case 3: a = aa[87 ]; break;
    case 4: a = aa[118]; break;
    case 5: a = aa[131]; break;
    case 6: a = aa[167]; break;
    case 7: a = aa[175]; break;
    case 8: a = aa[247]; break;
    case 9: a = aa[319]; break;
    case 10:a = aa[334]; break;
    case 11:a = aa[335]; break;
    case 12:a = aa[371]; break;
    case 13:a = aa[387]; break;
    case 14:a = aa[395]; break;
    case 15:a = aa[414]; break;
    case 16:a = aa[442]; break;
    case 17:a = aa[501]; break;
    case 18:a = aa[544]; break;
    case 19:a = aa[548]; break;
    case 20:a = aa[565]; break;
    case 21:a = aa[603]; break;
    case 22:a = aa[604]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data11(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[22 ]; break;
    case 1: a = aa[45 ]; break;
    case 2: a = aa[102]; break;
    case 3: a = aa[136]; break;
    case 4: a = aa[137]; break;
    case 5: a = aa[156]; break;
    case 6: a = aa[157]; break;
    case 7: a = aa[183]; break;
    case 8: a = aa[184]; break;
    case 9: a = aa[210]; break;
    case 10:a = aa[221]; break;
    case 11:a = aa[235]; break;
    case 12:a = aa[291]; break;
    case 13:a = aa[324]; break;
    case 14:a = aa[344]; break;
    case 15:a = aa[353]; break;
    case 16:a = aa[377]; break;
    case 17:a = aa[398]; break;
    case 18:a = aa[417]; break;
    case 19:a = aa[418]; break;
    case 20:a = aa[454]; break;
    case 21:a = aa[511]; break;
    case 22:a = aa[524]; break;
    case 23:a = aa[540]; break;
    case 24:a = aa[559]; break;
    case 25:a = aa[584]; break;
    case 26:a = aa[613]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data18(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[21 ]; break;
    case 1: a = aa[43 ]; break;
    case 2: a = aa[62 ]; break;
    case 3: a = aa[144]; break;
    case 4: a = aa[145]; break;
    case 5: a = aa[196]; break;
    case 6: a = aa[197]; break;
    case 7: a = aa[199]; break;
    case 8: a = aa[292]; break;
    case 9: a = aa[301]; break;
    case 10:a = aa[317]; break;
    case 11:a = aa[330]; break;
    case 12:a = aa[331]; break;
    case 13:a = aa[332]; break;
    case 14:a = aa[350]; break;
    case 15:a = aa[363]; break;
    case 16:a = aa[381]; break;
    case 17:a = aa[433]; break;
    case 18:a = aa[469]; break;
    case 19:a = aa[484]; break;
    case 20:a = aa[522]; break;
    case 21:a = aa[561]; break;
    case 22:a = aa[587]; break;
    case 23:a = aa[623]; break;
    case 24:a = aa[624]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data8(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0:a = aa[0]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data3(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[41 ]; break;
    case 1: a = aa[56 ]; break;
    case 2: a = aa[79 ]; break;
    case 3: a = aa[96 ]; break;
    case 4: a = aa[109]; break;
    case 5: a = aa[141]; break;
    case 6: a = aa[155]; break;
    case 7: a = aa[201]; break;
    case 8: a = aa[249]; break;
    case 9: a = aa[263]; break;
    case 10:a = aa[293]; break;
    case 11:a = aa[322]; break;
    case 12:a = aa[383]; break;
    case 13:a = aa[394]; break;
    case 14:a = aa[408]; break;
    case 15:a = aa[415]; break;
    case 16:a = aa[428]; break;
    case 17:a = aa[445]; break;
    case 18:a = aa[459]; break;
    case 19:a = aa[479]; break;
    case 20:a = aa[532]; break;
    case 21:a = aa[564]; break;
    case 22:a = aa[575]; break;
    case 23:a = aa[598]; break;
    case 24:a = aa[611]; break;
    default: a = 0; break;
  };
  return a;
}

// Content of called function
uint18_t get_data16(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[12 ]; break;
    case 1: a = aa[26 ]; break;
    case 2: a = aa[49 ]; break;
    case 3: a = aa[68 ]; break;
    case 4: a = aa[83 ]; break;
    case 5: a = aa[121]; break;
    case 6: a = aa[219]; break;
    case 7: a = aa[234]; break;
    case 8: a = aa[252]; break;
    case 9: a = aa[265]; break;
    case 10:a = aa[281]; break;
    case 11:a = aa[282]; break;
    case 12:a = aa[300]; break;
    case 13:a = aa[313]; break;
    case 14:a = aa[342]; break;
    case 15:a = aa[378]; break;
    case 16:a = aa[389]; break;
    case 17:a = aa[483]; break;
    case 18:a = aa[496]; break;
    case 19:a = aa[516]; break;
    case 20:a = aa[578]; break;
    case 21:a = aa[582]; break;
    case 22:a = aa[595]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint5_t get_bank(uint10_t addr)
{
  #pragma HLS inline
  return bank_mapping[addr];
}

// Content of called function
uint18_t get_data19(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[16 ]; break;
    case 1: a = aa[60 ]; break;
    case 2: a = aa[69 ]; break;
    case 3: a = aa[80 ]; break;
    case 4: a = aa[112]; break;
    case 5: a = aa[117]; break;
    case 6: a = aa[170]; break;
    case 7: a = aa[186]; break;
    case 8: a = aa[206]; break;
    case 9: a = aa[223]; break;
    case 10:a = aa[255]; break;
    case 11:a = aa[288]; break;
    case 12:a = aa[289]; break;
    case 13:a = aa[325]; break;
    case 14:a = aa[326]; break;
    case 15:a = aa[345]; break;
    case 16:a = aa[357]; break;
    case 17:a = aa[372]; break;
    case 18:a = aa[487]; break;
    case 19:a = aa[508]; break;
    case 20:a = aa[521]; break;
    case 21:a = aa[543]; break;
    case 22:a = aa[581]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data20(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;

  switch (offset)
  {
    case 0: a = aa[2  ]; break;
    case 1: a = aa[13 ]; break;
    case 2: a = aa[44 ]; break;
    case 3: a = aa[63 ]; break;
    case 4: a = aa[90 ]; break;
    case 5: a = aa[111]; break;
    case 6: a = aa[126]; break;
    case 7: a = aa[154]; break;
    case 8: a = aa[181]; break;
    case 9: a = aa[200]; break;
    case 10:a = aa[230]; break;
    case 11:a = aa[257]; break;
    case 12:a = aa[294]; break;
    case 13:a = aa[295]; break;
    case 14:a = aa[296]; break;
    case 15:a = aa[339]; break;
    case 16:a = aa[373]; break;
    case 17:a = aa[399]; break;
    case 18:a = aa[422]; break;
    case 19:a = aa[436]; break;
    case 20:a = aa[462]; break;
    case 21:a = aa[518]; break;
    case 22:a = aa[533]; break;
    case 23:a = aa[585]; break;
    case 24:a = aa[610]; break;
    default: a = 0; break;
  }; 
  return a;
}

// Content of called function
uint18_t get_data2(int offset, uint18_t aa[ROWS*COLS])
{
  #pragma HLS inline
  uint18_t a;
  switch (offset)
  {
    case 0: a = aa[9  ]; break;
    case 1: a = aa[20 ]; break;
    case 2: a = aa[28 ]; break;
    case 3: a = aa[48 ]; break;
    case 4: a = aa[74 ]; break;
    case 5: a = aa[97 ]; break;
    case 6: a = aa[168]; break;
    case 7: a = aa[187]; break;
    case 8: a = aa[188]; break;
    case 9: a = aa[213]; break;
    case 10:a = aa[233]; break;
    case 11:a = aa[260]; break;
    case 12:a = aa[261]; break;
    case 13:a = aa[277]; break;
    case 14:a = aa[303]; break;
    case 15:a = aa[314]; break;
    case 16:a = aa[329]; break;
    case 17:a = aa[356]; break;
    case 18:a = aa[375]; break;
    case 19:a = aa[376]; break;
    case 20:a = aa[452]; break;
    case 21:a = aa[499]; break;
    case 22:a = aa[519]; break;
    case 23:a = aa[529]; break;
    case 24:a = aa[536]; break;
    case 25:a = aa[551]; break;
    case 26:a = aa[567]; break;
    case 27:a = aa[597]; break;
    case 28:a = aa[615]; break;
    default: a = 0; break;
  };
  return a;
}