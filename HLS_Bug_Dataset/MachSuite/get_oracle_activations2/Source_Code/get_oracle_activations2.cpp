void get_oracle_activations2(TYPE weights3[nodes_per_layer*possible_outputs], 
                             TYPE output_differences[possible_outputs], 
                             TYPE oracle_activations[nodes_per_layer],
                             TYPE dactivations[nodes_per_layer]) {
    int i, j;
    for( i = 0; i < nodes_per_layer; i++) {
        oracle_activations[i] = (TYPE)0.0;
        for( j = 0; j < possible_outputs; j++) {
            oracle_activations[i] += output_differences[j] * weights3[i*possible_outputs + j];
        }
        oracle_activations[i] = oracle_activations[i] * dactivations[i];
    }
}