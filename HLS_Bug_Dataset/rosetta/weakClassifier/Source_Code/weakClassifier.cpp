int weakClassifier
(
  int stddev,
  uint18_t coord[12],  
  int haar_counter,
  int w_id 
)
{
  /* weights and threshold values for various classifiers */
                                                                                             
  #include "haar_dataEWC_with_partitioning.h"
  # pragma HLS INLINE

  int t = tree_thresh_array[haar_counter] * stddev; 
  
  int sum0 =0;
  int sum1 =0;
  int sum2 =0;
  int final_sum =0;
  int return_value;
	             
  sum0 = (coord[0] - coord[1] - coord[2] + coord[3]) * weights_array0[haar_counter];
  sum1 = (coord[4] - coord[5] - coord[6] + coord[7]) * weights_array1[haar_counter];  
  sum2 = (coord[8] - coord[9] - coord[10] + coord[11]) * weights_array2[haar_counter];
  final_sum = sum0+sum1+sum2;
  
  if(final_sum >= t) 
    return_value =  alpha2_array[haar_counter];  
  else   
    return_value =  alpha1_array[haar_counter];
  
  return return_value ; 
}