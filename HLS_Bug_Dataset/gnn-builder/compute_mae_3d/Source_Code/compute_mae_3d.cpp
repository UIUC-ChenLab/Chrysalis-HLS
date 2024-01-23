float compute_mae_3d(T arr1[M][N][O], T arr2[M][N][O])
{
    float mae = 0;
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            for (int k = 0; k < O; k++)
            {
                mae += std::abs(arr1[i][j][k] - arr2[i][j][k]);
            }
        }
    }
    return mae / (M * N * O);
}