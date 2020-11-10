#include <stdio.h> //remove if not using.
#include <pthread.h>
#include <stdlib.h>
#include "util.h"//implementing
#include <stdbool.h>
typedef struct { //example structure
    int example;
    int e_g;
} Example_Structure;

static void swap(double *x, double *y){
    // Functions defined with the modifier 'static' are only visible
    // to other functions in this file. They cannot be called from
    // outside (for example, from main.c). Use them to organize your
    // code. Remember that in C, you cannot use a function until you
    // declare it, so declare all your utility functions above the
    // point where you use them.
    //
    // Maintain the mat_sq_trans_xt functions as lean as possible
    // because we are measuring their speed. Unless you are debugging,
    // do not print anything on them, that consumes precious time.
    //
    // You may delete this example helper function and structure, and
    // write your own useful ones.
    double temp = *x;
    *x = *y;
    *y = temp;
    return;
}

void mat_sq_trans_st(Mat *mat){
    //Put your code here.
    // example_helper_function(1000);

    for(int i=0;i<mat->m-1;i++){
        for(int j=i+1;j<mat->n;j++){
            swap(&mat->ptr[i*mat->n+j],&mat->ptr[j*mat->n + i]);
        }
    }
    return;
}
typedef struct {
    pthread_mutex_t *pmtx;
    Mat *A;
    int *curX,*curY;
    int n;
} ThreadData;
static void* pickNext(void * arg){
    // extract ThreadData struct from arg
    ThreadData* td = (ThreadData*) arg;
    double * A_ptr = td->A->ptr;
    Mat* A = td->A;
    
    while(true){
        pthread_mutex_lock(td->pmtx);
        int st_x = *td->curX; 
        int st_y = *td->curY;
        // if st_x==-1 -> end process
        if(st_x==-1){
            pthread_mutex_unlock(td->pmtx);
            pthread_exit(NULL);
        }
        // add td->n to the same row and deal with the overflow
        int end_x = st_x;
        int end_y = *td->curY+ td->n-1;
        while(end_y >= A->n && end_x < A->m-1){
            end_x+=1;
            end_y = end_x+1+(end_y -(A->n-1));
        }
        if(end_x>= A->m-2){ // located in the last row to have data
            end_x = A->m-2;
            end_y = A->n-1;
            *td->curX = -1;
            *td->curY = -1;
        }else{
            if(end_y == A->n-1){ //located in the last column
                *td->curX = end_x+1;
                *td->curY = *td->curX+1;
            } else{
                *td->curY= end_y + 1;
                *td->curX = end_x;
            }
        }

        pthread_mutex_unlock(td->pmtx);
        int i = st_x;
        int j = st_y;
        while(i!=end_x || j!=end_y){
            swap(&A_ptr[i*A->n+j],&A_ptr[j*A->n + i]);
            j+=1;
            
            if(j >= A->n){
                i+=1;
                j = i+1;
            }  
        }

        swap(&A_ptr[end_x*A->n+end_y],&A_ptr[end_y*A->n + end_x]);
    }
    // terminate thread
    pthread_exit(NULL);
}

void mat_sq_trans_mt(Mat *mat, unsigned int grain, unsigned int threads){
    //Put your code here.
    //example_helper_function(2000);


    // initialize pthreads, ThreadData struct, and mutex
    pthread_t threadArr[threads];
    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
    ThreadData td;
    int sharedX = 0;
    int sharedY = 1;
    // set values of ThreadData struct
    td.pmtx = &mut;
    td.A = mat;
    td.curX = &sharedX;
    td.curY = &sharedY;
    td.n = grain;

    // initialize mutex
    int ret = pthread_mutex_init(td.pmtx, NULL);

    // return failure if mutex init failed
    if(ret)
    {
        printf("Error creating mutex\n");
        exit(-1);
    }

    // create pthreads
    for(int i = 0; i < threads; i++)
    {
        ret = pthread_create(&threadArr[i], NULL, &pickNext, (void*) &td);
        if(ret)
        {
            printf("Error creating pthread\n");
            exit(-1);
        }
    }

    // join pthreads to main thread
    for(int i = 0; i < threads; i++)
    {
        pthread_join(threadArr[i], NULL);
    }
    // all threads finished execution
    return;
}
