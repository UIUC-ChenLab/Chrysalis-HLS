void RELU(TYPE activations[nodes_per_layer], TYPE dactivations[nodes_per_layer], int size) {
    int i;
    for( i = 0; i < size; i++) {
        dactivations[i] = activations[i]*(1.0-activations[i]);
        activations[i] = 1.0/(1.0+exp(-activations[i]));
    }
}