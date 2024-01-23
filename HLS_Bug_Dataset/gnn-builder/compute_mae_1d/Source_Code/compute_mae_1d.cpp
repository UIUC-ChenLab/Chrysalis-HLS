float compute_mae_1d(T arr1[M], T arr2[M])
{
    float mae = 0;
    for (int i = 0; i < M; i++)
    {
        mae += std::abs(arr1[i] - arr2[i]);
    }
    return mae / M;
}