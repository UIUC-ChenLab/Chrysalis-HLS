T activation_tanh(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
#if __FLOATING_POINT_MODEL__
    T out = m_tanh(x);
    return out;
#else
    T out = m_tanh(x);
    T out_fixed = (hls::signbit(x) != hls::signbit(out)) ? T(-out) : out;
    return out_fixed;
#endif
}