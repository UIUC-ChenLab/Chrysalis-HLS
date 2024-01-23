T activation_sigmoid(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    return T(1.0) / (T(1.0) + m_exp(-x));
}