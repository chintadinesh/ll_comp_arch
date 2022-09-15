#include "transform.h"

#define CONSTANT 100
#define N 10

int mail_boxes[N*N];
int result_matrix[N*N];

inline void mail_check(int row, int column) {
    while(mail_boxes[row*N + column]);
}

int main() {

    int row =0;
    int column=4;
    int left_id = (row*N) + (column - 1), top_id = (row-1)*N + column;
    int core_id =  row * N + column;
    int right_id = row*N + column + 1, bottom_id = (row+1)*N + column;

    for(; row < N; row++){
        if((row == 0) && (column == 0)) {
            result_matrix[row*N + column] = CONSTANT; 
        }
        else if(row == 0) {
            mail_check(left_id, column);
            result_matrix[row*N + column] = transform_function(result_matrix[left_id*N + column], 0);
        }
        else if(column == 0){
            mail_check(row, top_id); 
            result_matrix[row*N + column] = transform_function(0, result_matrix[row*N + top_id]);
        }
        else{
            mail_check(left_id, top_id);
            result_matrix[row*N + column] = transform_function(result_matrix[left_id*N + column], result_matrix[row*N + top_id]);
        }

        mail_boxes[row*N + column] = 1;
    }

    return 0;
}

