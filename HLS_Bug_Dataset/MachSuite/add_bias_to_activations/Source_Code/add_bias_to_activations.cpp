void add_bias_to_activations(TYPE biases[nodes_per_layer], 
                               TYPE activations[nodes_per_layer],
                               int size) {
    int i;
    for( i = 0; i < size; i++){
        activations[i] = activations[i] + biases[i];
    }
}