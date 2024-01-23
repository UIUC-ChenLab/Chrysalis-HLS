float compute_mae_2d(T arr1[M][N], T arr2[M][N])
{
    float mae = 0;
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            mae += std::abs(arr1[i][j] - arr2[i][j]);
        }
    }
    return mae / (M * N);
}