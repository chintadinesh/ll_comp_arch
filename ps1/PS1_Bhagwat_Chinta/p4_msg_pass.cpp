#include "transform.h"

#define CONSTANT 100
#define N 10        // matrix dimensions

int result[N];         // memory to store the result and communicate

int main() {

    int left_val = 0;
    int row =0; 
    int column =4 ;  // current row and column no of the core

    for(; row < N; row++){
        if((row == 0) && (column == 0)) {
            result[row] = CONSTANT; 
        }
        else if(row == 0) {
            RECEIVE(&left_val, sizeof(int), row*N + column - 1);
            result[row] = transform_function(left_val, 0);
        }
        else if(column == 0){
            result[row] = transform_function(0, result[row-1]);
        }
        else{
            RECEIVE(&left_val, sizeof(int), row*N + column - 1);
            result[row] = transform_function(left_val, result[row-1]);
        }

        // communicate the result to right and bottom cores. We assume that last
        // row and column won't raise an error when sending data to a non-existat core
        SEND(result + row, sizeof(int), row*N + column + 1); 
    }

    return 0;
}

