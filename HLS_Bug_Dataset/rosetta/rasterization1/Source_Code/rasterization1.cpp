bit2 rasterization1 ( Triangle_2D triangle_2d, bit8 max_min[], Triangle_2D *triangle_2d_same, bit16 max_index[])
{
  #pragma HLS INLINE off
  // clockwise the vertices of input 2d triangle
  if ( check_clockwise( triangle_2d ) == 0 )
    return 1;
  if ( check_clockwise( triangle_2d ) < 0 )
    clockwise_vertices( &triangle_2d );

  // copy the same 2D triangle
  triangle_2d_same->x0 = triangle_2d.x0;
  triangle_2d_same->y0 = triangle_2d.y0;
  triangle_2d_same->x1 = triangle_2d.x1;
  triangle_2d_same->y1 = triangle_2d.y1;
  triangle_2d_same->x2 = triangle_2d.x2;
  triangle_2d_same->y2 = triangle_2d.y2;
  triangle_2d_same->z  = triangle_2d.z ;

  // find the rectangle bounds of 2D triangles
  max_min[0] = find_min( triangle_2d.x0, triangle_2d.x1, triangle_2d.x2 );
  max_min[1] = find_max( triangle_2d.x0, triangle_2d.x1, triangle_2d.x2 );
  max_min[2] = find_min( triangle_2d.y0, triangle_2d.y1, triangle_2d.y2 );
  max_min[3] = find_max( triangle_2d.y0, triangle_2d.y1, triangle_2d.y2 );
  max_min[4] = max_min[1] - max_min[0];

  // calculate index for searching pixels
  max_index[0] = (max_min[1] - max_min[0]) * (max_min[3] - max_min[2]);

  return 0;
}

// Content of called function
bit8 find_max( bit8 in0, bit8 in1, bit8 in2 )
{
  if (in0 > in1)
  {
    if (in0 > in2)
      return in0;
    else 
      return in2;
  }
  else 
  {
    if (in1 > in2) 
      return in1;
    else 
      return in2;
  }
}

// Content of called function
int check_clockwise( Triangle_2D triangle_2d )
{
  int cw;

  cw = (triangle_2d.x2 - triangle_2d.x0) * (triangle_2d.y1 - triangle_2d.y0)
       - (triangle_2d.y2 - triangle_2d.y0) * (triangle_2d.x1 - triangle_2d.x0);

  return cw;

}

// Content of called function
bit8 find_min( bit8 in0, bit8 in1, bit8 in2 )
{
  if (in0 < in1)
  {
    if (in0 < in2)
      return in0;
    else 
      return in2;
  }
  else 
  {
    if (in1 < in2) 
      return in1;
    else 
      return in2;
  }
}

// Content of called function
void clockwise_vertices( Triangle_2D *triangle_2d )
{

  bit8 tmp_x, tmp_y;

  tmp_x = triangle_2d->x0;
  tmp_y = triangle_2d->y0;

  triangle_2d->x0 = triangle_2d->x1;
  triangle_2d->y0 = triangle_2d->y1;

  triangle_2d->x1 = tmp_x;
  triangle_2d->y1 = tmp_y;

}