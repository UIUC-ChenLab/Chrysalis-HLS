void pna_conv_copy_num_in_neighbors(
    T &num_in_neighbors,
    T &num_in_neighbors_0,
    T &num_in_neighbors_1,
    T &num_in_neighbors_2)
{
#pragma HLS INLINE off
    num_in_neighbors_0 = num_in_neighbors;
    num_in_neighbors_1 = num_in_neighbors;
    num_in_neighbors_2 = num_in_neighbors;
}