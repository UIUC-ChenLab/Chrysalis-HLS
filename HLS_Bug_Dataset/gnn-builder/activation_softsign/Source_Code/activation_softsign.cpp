T activation_softsign(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    return x / (T(1.0) + m_abs(x));
}