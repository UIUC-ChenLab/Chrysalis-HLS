FIX_32_12 compute_engine_16(FIX_WT w0,  FIX_FM b0,
					  FIX_WT w1,  FIX_FM b1,
					  FIX_WT w2,  FIX_FM b2,
					  FIX_WT w3,  FIX_FM b3,
					  FIX_WT w4,  FIX_FM b4,
					  FIX_WT w5,  FIX_FM b5,
					  FIX_WT w6,  FIX_FM b6,
					  FIX_WT w7,  FIX_FM b7,
					  FIX_WT w8,  FIX_FM b8,
					  FIX_WT w9,  FIX_FM b9,
					  FIX_WT w10, FIX_FM b10,
					  FIX_WT w11, FIX_FM b11,
					  FIX_WT w12, FIX_FM b12,
					  FIX_WT w13, FIX_FM b13,
					  FIX_WT w14, FIX_FM b14,
					  FIX_WT w15, FIX_FM b15)
{
	FIX_32_12 mul0, mul1, mul2,  mul3,  mul4,  mul5,  mul6,  mul7;
	FIX_32_12 mul8, mul9, mul10, mul11, mul12, mul13, mul14, mul15;
	FIX_32_12 add0, add1, add2, add3,  add4,  add5,  add6;
	FIX_32_12 add7, add8, add9, add10, add11, add12, add13, add14;

	mul0  = w0  * b0;
	mul1  = w1  * b1;
	mul2  = w2  * b2;
	mul3  = w3  * b3;
	mul4  = w4  * b4;
	mul5  = w5  * b5;
	mul6  = w6  * b6;
	mul7  = w7  * b7;
	mul8  = w8  * b8;
	mul9  = w9  * b9;
	mul10 = w10 * b10;
	mul11 = w11 * b11;
	mul12 = w12 * b12;
	mul13 = w13 * b13;
	mul14 = w14 * b14;
	mul15 = w15 * b15;


	add0 = mul0  + mul1;
	add1 = mul2  + mul3;
	add2 = mul4  + mul5;
	add3 = mul6  + mul7;
	add4 = mul8  + mul9;
	add5 = mul10 + mul11;
	add6 = mul12 + mul13;
	add7 = mul14 + mul15;

	add8  = add0 + add1;
	add9  = add2 + add3;
	add10 = add4 + add5;
	add11 = add6 + add7;

	add12 = add8  + add9;
	add13 = add10 + add11;

	add14 = add12 + add13;

	return add14;

}