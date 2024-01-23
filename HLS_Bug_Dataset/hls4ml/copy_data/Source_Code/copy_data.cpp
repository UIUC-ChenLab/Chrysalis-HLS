void copy_data(std::vector<src_T> src, hls::stream<dst_T> &dst) {
    typename std::vector<src_T>::const_iterator in_begin = src.cbegin() + OFFSET;
    typename std::vector<src_T>::const_iterator in_end = in_begin + SIZE;

    size_t i_pack = 0;
    dst_T dst_pack;
    for (typename std::vector<src_T>::const_iterator i = in_begin; i != in_end; ++i) {
        dst_pack[i_pack++] = typename dst_T::value_type(*i);
        if (i_pack == dst_T::size) {
            i_pack = 0;
            dst.write(dst_pack);
        }
    }
}