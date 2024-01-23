T activation_gelu_approx_tanh(T x)
{
#pragma HLS INLINE off
#pragma HLS PIPELINE

    const T GELU_APPROX_MIN = -8.31776613691702;
    const T GELU_TANH_COEFF_LINEAR = 0.7978845608028654;
    const T GELU_TANH_COEFF_CUBIC = 0.035677408136300125;
    const T GELU_APPROX_MAX = 8.31776613691702;

    // prevent overflow of tanh_arg by setting limits where the
    // datatype is too coarse to represent the tanh result anyway.
    if (x < GELU_APPROX_MIN)
        return T(0.0);
    if (x > GELU_APPROX_MAX)
        return x;

    const T tanh_arg = T(T(T(GELU_TANH_COEFF_CUBIC * x) * x) + GELU_TANH_COEFF_LINEAR) * x;
    const T tanh = m_tanh(tanh_arg);
    const T tanh_fixed = (m_signbit(tanh_arg) != m_signbit(tanh)) ? T(-tanh) : tanh;
    // #if  __FLOATING_POINT_MODEL__

#if __FLOATING_POINT_MODEL__
    T out = (x / T(2.0)) * T(T(1.0) + tanh_fixed);
#else
    T out = (x >> 1) * T(T(1.0) + tanh_fixed);
#endif

    return out;
}