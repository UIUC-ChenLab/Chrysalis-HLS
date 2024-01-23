static
void kernel_fdtd_apml(int cz,
		      int cxm,
		      int cym,
		      DATA_TYPE mui,
		      DATA_TYPE ch,
		      DATA_TYPE POLYBENCH_2D(Ax,CZ+1,CYM+1,cz+1,cym+1),
		      DATA_TYPE POLYBENCH_2D(Ry,CZ+1,CYM+1,cz+1,cym+1),
		      DATA_TYPE POLYBENCH_2D(clf,CYM+1,CXM+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_2D(tmp,CYM+1,CXM+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_3D(Bza,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_3D(Ex,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_3D(Ey,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_3D(Hz,CZ+1,CYM+1,CXM+1,cz+1,cym+1,cxm+1),
		      DATA_TYPE POLYBENCH_1D(czm,CZ+1,cz+1),
		      DATA_TYPE POLYBENCH_1D(czp,CZ+1,cz+1),
		      DATA_TYPE POLYBENCH_1D(cxmh,CXM+1,cxm+1),
		      DATA_TYPE POLYBENCH_1D(cxph,CXM+1,cxm+1),
		      DATA_TYPE POLYBENCH_1D(cymh,CYM+1,cym+1),
		      DATA_TYPE POLYBENCH_1D(cyph,CYM+1,cym+1))
{
  int iz, iy, ix;

#pragma scop
  for (iz = 0; iz < _PB_CZ; iz++)
    {
      for (iy = 0; iy < _PB_CYM; iy++)
	{
	  for (ix = 0; ix < _PB_CXM; ix++)
	    {
	      clf[iz][iy] = Ex[iz][iy][ix] - Ex[iz][iy+1][ix] + Ey[iz][iy][ix+1] - Ey[iz][iy][ix];
	      tmp[iz][iy] = (cymh[iy] / cyph[iy]) * Bza[iz][iy][ix] - (ch / cyph[iy]) * clf[iz][iy];
	      Hz[iz][iy][ix] = (cxmh[ix] /cxph[ix]) * Hz[iz][iy][ix]
		+ (mui * czp[iz] / cxph[ix]) * tmp[iz][iy]
		- (mui * czm[iz] / cxph[ix]) * Bza[iz][iy][ix];
	      Bza[iz][iy][ix] = tmp[iz][iy];
	    }
	  clf[iz][iy] = Ex[iz][iy][_PB_CXM] - Ex[iz][iy+1][_PB_CXM] + Ry[iz][iy] - Ey[iz][iy][_PB_CXM];
	  tmp[iz][iy] = (cymh[iy] / cyph[iy]) * Bza[iz][iy][_PB_CXM] - (ch / cyph[iy]) * clf[iz][iy];
	  Hz[iz][iy][_PB_CXM]=(cxmh[_PB_CXM] / cxph[_PB_CXM]) * Hz[iz][iy][_PB_CXM]
	    + (mui * czp[iz] / cxph[_PB_CXM]) * tmp[iz][iy]
	    - (mui * czm[iz] / cxph[_PB_CXM]) * Bza[iz][iy][_PB_CXM];
	  Bza[iz][iy][_PB_CXM] = tmp[iz][iy];
	  for (ix = 0; ix < _PB_CXM; ix++)
	    {
	      clf[iz][iy] = Ex[iz][_PB_CYM][ix] - Ax[iz][ix] + Ey[iz][_PB_CYM][ix+1] - Ey[iz][_PB_CYM][ix];
	      tmp[iz][iy] = (cymh[_PB_CYM] / cyph[iy]) * Bza[iz][iy][ix] - (ch / cyph[iy]) * clf[iz][iy];
	      Hz[iz][_PB_CYM][ix] = (cxmh[ix] / cxph[ix]) * Hz[iz][_PB_CYM][ix]
		+ (mui * czp[iz] / cxph[ix]) * tmp[iz][iy]
		- (mui * czm[iz] / cxph[ix]) * Bza[iz][_PB_CYM][ix];
	      Bza[iz][_PB_CYM][ix] = tmp[iz][iy];
	    }
	  clf[iz][iy] = Ex[iz][_PB_CYM][_PB_CXM] - Ax[iz][_PB_CXM] + Ry[iz][_PB_CYM] - Ey[iz][_PB_CYM][_PB_CXM];
	  tmp[iz][iy] = (cymh[_PB_CYM] / cyph[_PB_CYM]) * Bza[iz][_PB_CYM][_PB_CXM] - (ch / cyph[_PB_CYM]) * clf[iz][iy];
	  Hz[iz][_PB_CYM][_PB_CXM] = (cxmh[_PB_CXM] / cxph[_PB_CXM]) * Hz[iz][_PB_CYM][_PB_CXM]
	    + (mui * czp[iz] / cxph[_PB_CXM]) * tmp[iz][iy]
	    - (mui * czm[iz] / cxph[_PB_CXM]) * Bza[iz][_PB_CYM][_PB_CXM];
	  Bza[iz][_PB_CYM][_PB_CXM] = tmp[iz][iy];
	}
    }
#pragma endscop

}