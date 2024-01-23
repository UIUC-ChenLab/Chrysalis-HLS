void matrix_vector_product_with_bias_output_layer(TYPE biases[possible_outputs],
                                                 TYPE weights[nodes_per_layer*possible_outputs],
                                                 TYPE activations[possible_outputs],
                                                 TYPE input_activations[nodes_per_layer]){
    int i, j;
    for(j = 0; j < possible_outputs; j++){
        activations[j] = (TYPE)0.0;
        for (i = 0; i < nodes_per_layer; i++){
            activations[j] += weights[j*nodes_per_layer + i] * input_activations[i];
        }
    }
    add_bias_to_activations(biases, activations, possible_outputs);
}

// Content of called function
void add_bias_to_activations(TYPE biases[nodes_per_layer], 
                               TYPE activations[nodes_per_layer],
                               int size) {
    int i;
    for( i = 0; i < size; i++){
        activations[i] = activations[i] + biases[i];
    }
}