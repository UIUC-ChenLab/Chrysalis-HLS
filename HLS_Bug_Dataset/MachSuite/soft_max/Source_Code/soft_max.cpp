void soft_max(TYPE net_outputs[possible_outputs], TYPE activations[possible_outputs]) {
    int i;
    TYPE sum;
    sum = (TYPE) 0.0;

    for(i=0; i < possible_outputs; i++) {
        sum += exp(-activations[i]);
    }
    for(i=0; i < possible_outputs; i++) {
        net_outputs[i] = exp(-activations[i])/sum;
    }
}