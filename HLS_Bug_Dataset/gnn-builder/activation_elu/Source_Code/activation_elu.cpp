T activation_elu(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE
    const T alpha = T(1.0);
    if (x > 0)
    {
        return x;
    }
    else
    {
        return alpha * (m_exp(x) - T(1.0));
    }
}