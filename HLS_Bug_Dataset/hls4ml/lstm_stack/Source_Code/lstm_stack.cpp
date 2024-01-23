void lstm_stack(data_T data[CONFIG_T::n_sequence * CONFIG_T::n_in], res_T res[CONFIG_T::n_sequence_out * CONFIG_T::n_state],
                typename CONFIG_T::weight_t param[CONFIG_T::n_state * 4 * CONFIG_T::n_in],
                typename CONFIG_T::weight_t param_r[CONFIG_T::n_state * 4 * CONFIG_T::n_state],
                typename CONFIG_T::bias_t param_b[CONFIG_T::n_state * 4],
                typename CONFIG_T::bias_t param_br[CONFIG_T::n_state * 4]) {

    res_T h_newstate[CONFIG_T::n_state];
    res_T s_newstate[CONFIG_T::n_state];
    data_T data_in[CONFIG_T::n_in];
    bool reset_state = true;

    #pragma HLS ARRAY_PARTITION variable=h_newstate complete
    #pragma HLS ARRAY_PARTITION variable=s_newstate complete

    for (int ii = 0; ii < CONFIG_T::n_state; ii++) {
        #pragma HLS UNROLL
        h_newstate[ii] = 0;
        s_newstate[ii] = 0;
    }
    for (int iloop = 0; iloop < CONFIG_T::n_sequence; iloop++) {
        for (int j = 0; j < CONFIG_T::n_in; j++) {
            #pragma HLS UNROLL
            data_in[j] = data[j + iloop * CONFIG_T::n_in];
        }
        if (CONFIG_T::use_static)
            nnet::lstm_static<data_T, res_T, CONFIG_T>(reset_state, data_in, h_newstate, s_newstate, param, param_r, param_b,
                                                       param_br);
        else
            nnet::lstm<data_T, res_T, CONFIG_T>(reset_state, data_in, h_newstate, s_newstate, param, param_r, param_b,
                                                param_br);
        if (CONFIG_T::n_sequence_out > 1)
            for (int i = CONFIG_T::n_state * iloop, j = 0; i < (CONFIG_T::n_state * (iloop + 1)); i++, j++) {
                #pragma HLS UNROLL
                res[i] = h_newstate[j];
            }
        reset_state = false;
    }
    if (CONFIG_T::n_sequence_out == 1)
        for (int i = 0; i < (CONFIG_T::n_state); i++) {
            #pragma HLS UNROLL
            res[i] = h_newstate[i];
        }
}

// Content of called function
void lstm(bool reset_state, data_T data[CONFIG_T::n_in], res_T h_newstate[CONFIG_T::n_state],
          res_T s_newstate[CONFIG_T::n_state], typename CONFIG_T::weight_t param[CONFIG_T::n_state * 4 * CONFIG_T::n_in],
          typename CONFIG_T::weight_t param_r[CONFIG_T::n_state * 4 * CONFIG_T::n_state],
          typename CONFIG_T::bias_t param_b[CONFIG_T::n_state * 4],
          typename CONFIG_T::bias_t param_br[CONFIG_T::n_state * 4]) {
    // Initialize the state variable -- will maintain state between function calls

    typename CONFIG_T::accum_t tmpres[CONFIG_T::n_state * 4];
    typename CONFIG_T::accum_t tmpres_state[CONFIG_T::n_state * 4];
    typename CONFIG_T::accum_t tmpres_ifo[CONFIG_T::n_state * 3];   // activated i,f,o matrices (keras notation)
    typename CONFIG_T::accum_t tmpres_c[CONFIG_T::n_state];         // activated c-matrix (keras notation)
    typename CONFIG_T::accum_t inputacc_ifo[CONFIG_T::n_state * 3]; // i,f,o matrices (keras notation)
    typename CONFIG_T::accum_t inputacc_c[CONFIG_T::n_state];       // c-matrix (keras notation)
    typename CONFIG_T::accum_t s_actstate[CONFIG_T::n_state];

    #pragma HLS ARRAY_PARTITION variable=h_newstate   complete
    #pragma HLS ARRAY_PARTITION variable=s_newstate   complete
    #pragma HLS ARRAY_PARTITION variable=tmpres       complete
    #pragma HLS ARRAY_PARTITION variable=tmpres_state complete
    #pragma HLS ARRAY_PARTITION variable=tmpres_ifo   complete
    #pragma HLS ARRAY_PARTITION variable=tmpres_c     complete
    #pragma HLS ARRAY_PARTITION variable=inputacc_ifo complete
    #pragma HLS ARRAY_PARTITION variable=inputacc_c   complete
    #pragma HLS ARRAY_PARTITION variable=s_actstate   complete

    nnet::dense<data_T, res_T, typename CONFIG_T::mult_config1>(data, tmpres, param, param_b);
    nnet::dense<data_T, res_T, typename CONFIG_T::mult_config2>(h_newstate, tmpres_state, param_r, param_br);

    for (int iacc = 0; iacc < (3 * CONFIG_T::n_state); iacc++) {
        #pragma HLS UNROLL
        int index = iacc;
        if (iacc > 2 * CONFIG_T::n_state - 1)
            index = iacc + CONFIG_T::n_state;
        inputacc_ifo[iacc] = tmpres[index] + tmpres_state[index];
    }
    for (int iacc = 0; iacc < (CONFIG_T::n_state); iacc++) {
        #pragma HLS UNROLL
        int index = iacc + CONFIG_T::n_state * 2;
        inputacc_c[iacc] = tmpres[index] + tmpres_state[index];
    }

    CONFIG_T::template activation_recr<data_T, typename CONFIG_T::weight_t, typename CONFIG_T::ACT_CONFIG_LSTM>::activation(
        inputacc_ifo, tmpres_ifo);

    // Now for the confusion matrix
    CONFIG_T::template activation<data_T, typename CONFIG_T::weight_t, typename CONFIG_T::ACT_CONFIG_T>::activation(
        inputacc_c, tmpres_c);

    // Operation: s=g*i+sold*f (update state with buffer to avoid timing issues)
    for (int iacc = 0; iacc < (CONFIG_T::n_state); iacc++) {
        #pragma HLS UNROLL
        s_newstate[iacc] = tmpres_c[iacc] * tmpres_ifo[iacc] + s_newstate[iacc] * tmpres_ifo[iacc + (CONFIG_T::n_state)];
    }
    // Operation: h=act(s)*o
    CONFIG_T::template activation<data_T, typename CONFIG_T::weight_t, typename CONFIG_T::ACT_CONFIG_T>::activation(
        s_newstate, s_actstate);

    for (int iacc = 0; iacc < CONFIG_T::n_state; iacc++) {
        #pragma HLS UNROLL
        h_newstate[iacc] = tmpres_ifo[iacc + 2 * (CONFIG_T::n_state)] * s_actstate[iacc];
    }
}

// Content of called function
void dense(hls::stream<data_T> &data_stream, hls::stream<res_T> &res_stream,
           typename CONFIG_T::weight_t weights[CONFIG_T::n_in * CONFIG_T::n_out],
           typename CONFIG_T::bias_t biases[CONFIG_T::n_out]) {
    typename data_T::value_type data[CONFIG_T::n_in];
    #pragma HLS ARRAY_PARTITION variable=data complete

    typename res_T::value_type res[CONFIG_T::n_out];
    #pragma HLS ARRAY_PARTITION variable=res complete

DataPrepare:
    for (int i_in = 0; i_in < CONFIG_T::n_in / data_T::size; i_in++) {
        if (CONFIG_T::n_in / data_T::size > 1) {
            #pragma HLS PIPELINE
        }
        data_T data_pack = data_stream.read();
    DataPack:
        for (int i_pack = 0; i_pack < data_T::size; i_pack++) {
            #pragma HLS UNROLL
            data[i_in * data_T::size + i_pack] = data_pack[i_pack];
        }
    }

    dense_wrapper<typename data_T::value_type, typename res_T::value_type, CONFIG_T>(data, res, weights, biases);

ResWrite:
    for (unsigned i_out = 0; i_out < CONFIG_T::n_out / res_T::size; i_out++) {
        if (CONFIG_T::n_out / res_T::size > 1) {
            #pragma HLS PIPELINE
        }
        res_T res_pack;
        PRAGMA_DATA_PACK(res_pack)
    ResPack:
        for (int i_pack = 0; i_pack < res_T::size; i_pack++) {
            #pragma HLS UNROLL
            res_pack[i_pack] = res[i_out * res_T::size + i_pack];
        }
        res_stream.write(res_pack);
    }
}

// Content of called function
void dense_wrapper(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_out],
                   typename CONFIG_T::weight_t weights[CONFIG_T::n_in * CONFIG_T::n_out],
                   typename CONFIG_T::bias_t biases[CONFIG_T::n_out]) {
    #pragma HLS INLINE recursive
    if (CONFIG_T::strategy == nnet::latency) {
        #pragma HLS PIPELINE II=CONFIG_T::reuse_factor
        dense_latency<data_T, res_T, CONFIG_T>(data, res, weights, biases);
    } else {
        dense_resource<data_T, res_T, CONFIG_T>(data, res, weights, biases);
    }
}

// Content of called function
void dense_resource(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_out],
                    typename CONFIG_T::weight_t weights[CONFIG_T::n_in * CONFIG_T::n_out],
                    typename CONFIG_T::bias_t biases[CONFIG_T::n_out]) {

    #pragma HLS INLINE recursive

    if (CONFIG_T::reuse_factor <= CONFIG_T::n_in) {
        dense_resource_rf_leq_nin<data_T, res_T, CONFIG_T>(data, res, weights, biases);
    } else if (CONFIG_T::reuse_factor % CONFIG_T::n_in == 0) {
        dense_resource_rf_gt_nin_rem0<data_T, res_T, CONFIG_T>(data, res, weights, biases);
    } else {
        dense_resource_rf_gt_nin<data_T, res_T, CONFIG_T>(data, res, weights, biases);
    }
}

// Content of called function
void dense_resource_rf_leq_nin(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_out],
                               typename CONFIG_T::weight_t weights[CONFIG_T::n_in * CONFIG_T::n_out],
                               typename CONFIG_T::bias_t biases[CONFIG_T::n_out]) {

    const int rufactor = CONFIG_T::reuse_factor;
    const int multfactor = MIN(CONFIG_T::n_in, CONFIG_T::reuse_factor);
    const int multiplier_limit = DIV_ROUNDUP(CONFIG_T::n_in * CONFIG_T::n_out, multfactor);
    const int block_factor = DIV_ROUNDUP(CONFIG_T::n_in * CONFIG_T::n_out, CONFIG_T::reuse_factor);
    const int multscale = multiplier_limit / CONFIG_T::n_out;
    const int nin = CONFIG_T::n_in;
    const int nout = CONFIG_T::n_out;

    assert((multiplier_limit % nout == 0 || rufactor >= nin) && "The current Reuse Factor is not allowed");
    assert((multiplier_limit == block_factor) && "This function is correct only for RF <= N_IN");

    #pragma HLS function_instantiate variable=weights,biases
    //#pragma HLS RESOURCE variable=weights core=RAM_2P_BRAM Commenting out the deisgnation HLS seems to choose correctly
    #pragma HLS ARRAY_RESHAPE   variable=weights block factor=block_factor
    #pragma HLS ARRAY_PARTITION variable=biases complete

    typename CONFIG_T::accum_t acc[CONFIG_T::n_out];
    #pragma HLS ARRAY_PARTITION variable=acc complete

InitAccum:
    for (int iacc = 0; iacc < nout; iacc++) {
        #pragma HLS UNROLL
        acc[iacc] = (typename CONFIG_T::accum_t)biases[iacc];
    }

ReuseLoop:
    for (int ir = 0; ir < rufactor; ir++) {
        #pragma HLS PIPELINE II=1 rewind

        int w_index = ir;
        int in_index = ir;
        int out_index = 0;
        int acc_step = 0;

    MultLoop:
        for (int im = 0; im < block_factor; im++) {
            #pragma HLS UNROLL

            acc[out_index] += static_cast<typename CONFIG_T::accum_t>(
                CONFIG_T::template product<data_T, typename CONFIG_T::weight_t>::product(data[in_index], weights[w_index]));

            // Increment w_index
            w_index += rufactor;
            // Increment in_index
            in_index += rufactor;
            if (in_index >= nin) {
                in_index = ir;
            }
            // Increment out_index
            if (acc_step + 1 >= multscale) {
                acc_step = 0;
                out_index++;
            } else {
                acc_step++;
            }
        }
    }

// Cast to "res_t" type
Result:
    for (int ires = 0; ires < CONFIG_T::n_out; ires++) {
        #pragma HLS UNROLL
        res[ires] = cast<data_T, res_T, CONFIG_T>(acc[ires]);
    }
}

// Content of called function
cast(typename CONFIG_T::accum_t x) {
    return (res_T)x;
}

// Content of called function
void dense_resource_rf_gt_nin(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_out],
                              typename CONFIG_T::weight_t weights[CONFIG_T::n_in * CONFIG_T::n_out],
                              typename CONFIG_T::bias_t biases[CONFIG_T::n_out]) {

    const int rufactor = CONFIG_T::reuse_factor;
    const int multfactor = MIN(CONFIG_T::n_in, CONFIG_T::reuse_factor);
    const int multiplier_limit = DIV_ROUNDUP(CONFIG_T::n_in * CONFIG_T::n_out, multfactor);
    const int block_factor = DIV_ROUNDUP(CONFIG_T::n_in * CONFIG_T::n_out, CONFIG_T::reuse_factor);
    const int multscale = multiplier_limit / CONFIG_T::n_out;
    const int nin = CONFIG_T::n_in;
    const int nout = CONFIG_T::n_out;

    assert((multiplier_limit % nout == 0 || rufactor >= nin) && "The current Reuse Factor is not allowed");
    assert((rufactor > nin) && "This function is correct only for RF > N_IN");

    #pragma HLS function_instantiate variable=weights,biases
    //#pragma HLS RESOURCE variable=weights core=RAM_2P_BRAM Commenting out the deisgnation HLS seems to choose correctly
    #pragma HLS ARRAY_RESHAPE   variable=weights block factor=block_factor
    #pragma HLS ARRAY_PARTITION variable=biases complete

    typename CONFIG_T::accum_t acc[CONFIG_T::n_out];
    #pragma HLS ARRAY_PARTITION variable=acc complete

InitAccum:
    for (int iacc = 0; iacc < nout; iacc++) {
        #pragma HLS UNROLL
        acc[iacc] = (typename CONFIG_T::accum_t)biases[iacc];
    }

ReuseLoop:
    for (int ir = 0; ir < rufactor; ir++) {
        #pragma HLS PIPELINE II=1 rewind
        typename CONFIG_T::accum_t tmpmult[block_factor];
        #pragma HLS ARRAY_PARTITION variable=tmpmult complete

    MultLoop:
        for (int im = 0; im < block_factor; im++) {
            #pragma HLS UNROLL
            int w_index = ir + rufactor * im;
            int in_index = w_index % nin;
            if (w_index >= CONFIG_T::n_in * CONFIG_T::n_out)
                continue; // check out of bounds
            tmpmult[im] =
                CONFIG_T::template product<data_T, typename CONFIG_T::weight_t>::product(data[in_index], weights[w_index]);
        }

        typename CONFIG_T::accum_t mult[multiplier_limit];
        #pragma HLS ARRAY_PARTITION variable=mult complete

    ResetMult:
        for (int imult = 0; imult < multiplier_limit; imult++) {
            #pragma HLS UNROLL
            mult[imult] = 0;
        }

    AccumLoop1:
        for (int im = 0; im < block_factor; im++) {
            #pragma HLS UNROLL
            int w_index = ir + rufactor * im;
            int out_index = w_index / multfactor;
            if (out_index >= multiplier_limit)
                continue; // check out of bounds
            mult[out_index] += tmpmult[im];
        }

    AccumLoop2:
        for (int im = 0; im < multiplier_limit; im++) {
            #pragma HLS UNROLL
            // int out_index = im/multscale; // This is the general case
            // acc[out_index] += mult[im];
            acc[im] += mult[im]; // If RF > N_IN then multiplier_limit == n_out
        }
    }

// Cast to "res_t" type
Result:
    for (int ires = 0; ires < CONFIG_T::n_out; ires++) {
        #pragma HLS UNROLL
        res[ires] = cast<data_T, res_T, CONFIG_T>(acc[ires]);
    }
}

// Content of called function
void dense_resource_rf_gt_nin_rem0(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_out],
                                   typename CONFIG_T::weight_t weights[CONFIG_T::n_in * CONFIG_T::n_out],
                                   typename CONFIG_T::bias_t biases[CONFIG_T::n_out]) {

    const int rufactor = MIN(CONFIG_T::reuse_factor, CONFIG_T::n_in * CONFIG_T::n_out);
    const int multfactor = MIN(CONFIG_T::n_in, CONFIG_T::reuse_factor);
    const int multiplier_limit = DIV_ROUNDUP(CONFIG_T::n_in * CONFIG_T::n_out, multfactor);
    const int block_factor = DIV_ROUNDUP(CONFIG_T::n_in * CONFIG_T::n_out, CONFIG_T::reuse_factor);
    const int multscale = multiplier_limit / CONFIG_T::n_out;
    const int nin = CONFIG_T::n_in;
    const int nout = CONFIG_T::n_out;

    assert((multiplier_limit % nout == 0 || rufactor >= nin) && "The current Reuse Factor is not allowed");
    assert((rufactor > nin && rufactor % nin == 0) && "This function is correct only for RF > N_IN && RF % N_IN == 0");

    #pragma HLS function_instantiate variable=weights,biases
    //#pragma HLS RESOURCE variable=weights core=RAM_2P_BRAM Commenting out the deisgnation HLS seems to choose correctly
    #pragma HLS ARRAY_RESHAPE   variable=weights block factor=block_factor
    #pragma HLS ARRAY_PARTITION variable=biases complete

    typename CONFIG_T::accum_t acc[CONFIG_T::n_out];
    #pragma HLS ARRAY_PARTITION variable=acc complete

InitAccum:
    for (int iacc = 0; iacc < nout; iacc++) {
        #pragma HLS UNROLL
        acc[iacc] = (typename CONFIG_T::accum_t)biases[iacc];
    }

    int w_index;
    int in_index = 0;
    int out_index;
    int outstep = 0;
    const int outscale = rufactor / nin;

    int outidx[rufactor];
IndexLoop:
    for (int ir = 0; ir < rufactor; ir++) {
        outidx[ir] = outstep;
        if ((ir + 1) % nin == 0) {
            outstep++;
        }
    }

ReuseLoop:
    for (int ir = 0; ir < rufactor; ir++) {
        #pragma HLS PIPELINE II=1 rewind

        w_index = ir;
        out_index = outidx[ir] /*outstep*/;

    MultLoop:
        for (int im = 0; im < block_factor; im++) {
            #pragma HLS UNROLL
            acc[out_index] += static_cast<typename CONFIG_T::accum_t>(
                CONFIG_T::template product<data_T, typename CONFIG_T::weight_t>::product(data[in_index], weights[w_index]));

            w_index += rufactor;
            if (w_index >= CONFIG_T::n_in * CONFIG_T::n_out)
                break; // check out of bounds
            out_index += outscale;
        }

        in_index++;
        if (in_index >= nin) {
            in_index = 0;
            // outstep++; // This causes a huge increase in scheduling and RTL generation times, hence the above workaround.
        }
    }

// Cast to "res_t" type
Result:
    for (int ires = 0; ires < CONFIG_T::n_out; ires++) {
        #pragma HLS UNROLL
        res[ires] = cast<data_T, res_T, CONFIG_T>(acc[ires]);
    }
}

// Content of called function
void dense_latency(data_T data[CONFIG_T::n_in], res_T res[CONFIG_T::n_out],
                   typename CONFIG_T::weight_t weights[CONFIG_T::n_in * CONFIG_T::n_out],
                   typename CONFIG_T::bias_t biases[CONFIG_T::n_out]) {
    data_T cache;
    typename CONFIG_T::accum_t mult[CONFIG_T::n_in * CONFIG_T::n_out];
    typename CONFIG_T::accum_t acc[CONFIG_T::n_out];

    // Use a function_instantiate in case it helps to explicitly optimize unchanging weights/biases
    #pragma HLS function_instantiate variable=weights,biases

    // For parallel inputs:
    //   - completely partition arrays -- target fabric
    //   - if we have an unroll factor, limit number of multipliers
    #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

    // #pragma HLS ARRAY_PARTITION variable=weights complete // remove this line for now, it breaks compression sometimes
    #pragma HLS ARRAY_PARTITION variable=biases complete
    #pragma HLS ARRAY_PARTITION variable=mult complete
    #pragma HLS ARRAY_PARTITION variable=acc complete

    #pragma HLS ALLOCATION operation instances=mul limit=CONFIG_T::multiplier_limit

// Do the matrix-multiply
Product1:
    for (int ii = 0; ii < CONFIG_T::n_in; ii++) {
        cache = data[ii];
    Product2:
        for (int jj = 0; jj < CONFIG_T::n_out; jj++) {
            int index = ii * CONFIG_T::n_out + jj;
            mult[index] = CONFIG_T::template product<data_T, typename CONFIG_T::weight_t>::product(cache, weights[index]);
        }
    }

// Initialize accumulator with input biases
ResetAccum:
    for (int iacc = 0; iacc < CONFIG_T::n_out; iacc++) {
        acc[iacc] = (typename CONFIG_T::accum_t)biases[iacc];
    }

// Accumulate multiplication result
Accum1:
    for (int ii = 0; ii < CONFIG_T::n_in; ii++) {
    Accum2:
        for (int jj = 0; jj < CONFIG_T::n_out; jj++) {
            int index = ii * CONFIG_T::n_out + jj;
            acc[jj] += mult[index];
        }
    }

// Cast to "res_t" type
Result:
    for (int ires = 0; ires < CONFIG_T::n_out; ires++) {
        // res[ires] = (res_T) (acc[ires]);
        res[ires] = cast<data_T, res_T, CONFIG_T>(acc[ires]);
    }
}

// Content of called function
void multiply(hls::stream<input1_T> &data1, hls::stream<input2_T> &data2, hls::stream<res_T> &res) {
    assert(input1_T::size == input2_T::size && input1_T::size == res_T::size);

MultiplyLoop:
    for (int i = 0; i < CONFIG_T::n_elem / input1_T::size; i++) {
        #pragma HLS PIPELINE II=CONFIG_T::reuse_factor

        input1_T in_data1 = data1.read();
        input2_T in_data2 = data2.read();
        res_T out_data;
        PRAGMA_DATA_PACK(out_data)

    MultiplyPack:
        for (int j = 0; j < res_T::size; j++) {
            #pragma HLS UNROLL
            out_data[j] = in_data1[j] * in_data2[j];
        }

        res.write(out_data);
    }
}

// Content of called function
void lstm_static(bool reset_state, data_T data[CONFIG_T::n_in], res_T h_newstate[CONFIG_T::n_state],
                 res_T s_newstate[CONFIG_T::n_state],
                 typename CONFIG_T::weight_t param[CONFIG_T::n_state * 4 * CONFIG_T::n_in],
                 typename CONFIG_T::weight_t param_r[CONFIG_T::n_state * 4 * CONFIG_T::n_state],
                 typename CONFIG_T::bias_t param_b[CONFIG_T::n_state * 4],
                 typename CONFIG_T::bias_t param_br[CONFIG_T::n_state * 4]) {
    static res_T h_state[CONFIG_T::n_state];
    static res_T s_state[CONFIG_T::n_state];
    // Initialize the state variable -- will maintain state between function calls
    typename CONFIG_T::accum_t tmpres[CONFIG_T::n_state * 4];
    typename CONFIG_T::accum_t tmpres_state[CONFIG_T::n_state * 4];
    typename CONFIG_T::accum_t tmpres_ifo[CONFIG_T::n_state * 3];   // activated i,f,o matrices (keras notation)
    typename CONFIG_T::accum_t tmpres_c[CONFIG_T::n_state];         // activated c-matrix (keras notation)
    typename CONFIG_T::accum_t inputacc_ifo[CONFIG_T::n_state * 3]; // i,f,o matrices (keras notation)
    typename CONFIG_T::accum_t inputacc_c[CONFIG_T::n_state];       // c-matrix (keras notation)
    typename CONFIG_T::accum_t s_actstate[CONFIG_T::n_state];

    #pragma HLS ARRAY_PARTITION variable=h_newstate   complete
    #pragma HLS ARRAY_PARTITION variable=s_newstate   complete
    #pragma HLS ARRAY_PARTITION variable=h_state      complete
    #pragma HLS ARRAY_PARTITION variable=s_state      complete
    #pragma HLS ARRAY_PARTITION variable=tmpres       complete
    #pragma HLS ARRAY_PARTITION variable=tmpres_state complete
    #pragma HLS ARRAY_PARTITION variable=tmpres_ifo   complete
    #pragma HLS ARRAY_PARTITION variable=tmpres_c     complete
    #pragma HLS ARRAY_PARTITION variable=inputacc_ifo complete
    #pragma HLS ARRAY_PARTITION variable=inputacc_c   complete
    #pragma HLS ARRAY_PARTITION variable=s_actstate   complete

    if (reset_state) {
        for (int i_state = 0; i_state < (CONFIG_T::n_state); i_state++) {
            #pragma HLS UNROLL
            s_state[i_state] = 0;
            h_state[i_state] = 0;
        }
    }

    nnet::dense<data_T, typename CONFIG_T::accum_t, typename CONFIG_T::mult_config1>(data, tmpres, param, param_b);
    nnet::dense<res_T, typename CONFIG_T::accum_t, typename CONFIG_T::mult_config2>(h_state, tmpres_state, param_r,
                                                                                    param_br);

    for (int iacc = 0; iacc < (3 * CONFIG_T::n_state); iacc++) {
        #pragma HLS UNROLL
        int index = iacc;
        if (iacc > 2 * CONFIG_T::n_state - 1)
            index = iacc + CONFIG_T::n_state;
        inputacc_ifo[iacc] = tmpres[index] + tmpres_state[index];
    }
    for (int iacc = 0; iacc < (CONFIG_T::n_state); iacc++) {
        #pragma HLS UNROLL
        int index = iacc + CONFIG_T::n_state * 2;
        inputacc_c[iacc] = tmpres[index] + tmpres_state[index];
    }

    CONFIG_T::template activation_recr<data_T, typename CONFIG_T::weight_t, typename CONFIG_T::ACT_CONFIG_LSTM>::activation(
        inputacc_ifo, tmpres_ifo);

    // Now for the confusion matrix
    CONFIG_T::template activation<data_T, typename CONFIG_T::weight_t, typename CONFIG_T::ACT_CONFIG_T>::activation(
        inputacc_c, tmpres_c);

    // Operation: s=g*i+sold*f (update state with buffer to avoid timing issues)
    for (int iacc = 0; iacc < (CONFIG_T::n_state); iacc++) {
        #pragma HLS UNROLL
        s_state[iacc] = tmpres_c[iacc] * tmpres_ifo[iacc] + s_state[iacc] * tmpres_ifo[iacc + (CONFIG_T::n_state)];
        s_newstate[iacc] = s_state[iacc];
    }
    // Operation: h=act(s)*o
    CONFIG_T::template activation<data_T, typename CONFIG_T::weight_t, typename CONFIG_T::ACT_CONFIG_T>::activation(
        s_state, s_actstate);

    for (int iacc = 0; iacc < CONFIG_T::n_state; iacc++) {
        #pragma HLS UNROLL
        h_state[iacc] = tmpres_ifo[iacc + 2 * (CONFIG_T::n_state)] * s_actstate[iacc];
        h_newstate[iacc] = h_state[iacc];
    }
}