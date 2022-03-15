/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp, iblock, jblock, k, multy;
    if(N == 64){
    for(iblock = 0;iblock < N; iblock+=8)
        for(jblock = 0;jblock < M; jblock+=8){
            for(i = iblock;i < iblock + 4; i++)
                for(j = jblock;j < jblock + 8; j++){
                    if(j < jblock + 4){
                        tmp = A[i][j];
                        B[j][i] = tmp;
                    }
                    else{
                        tmp = A[i][j];
                        B[2 * jblock + 7 - j][2 * iblock + 7 - i] = tmp;
                    }
            }
            for(i = jblock+ 3;i >= jblock;i--){
                for(j = iblock+ 4;j < iblock + 8;j++){
                    tmp = B[i][j];
                    B[2 * jblock + 7 - i][2 * iblock + 7 -j] = tmp;
                }
                for(k = iblock + 4;k < iblock + 8;k++){
                    tmp = A[k][2 * jblock + 7 - i];
                    B[2 * jblock + 7 - i][k] = tmp;
                }
                for(k = iblock + 4;k < iblock + 8;k++){
                    tmp = A[k][i];
                    B[i][k] = tmp;
                }                    
            }
        }
}


    else if(N == 32){
        for(iblock = 0;iblock < N / 8;iblock++)
            for(jblock = 0;jblock < M / 8;jblock++){
                if(iblock == jblock)
                    continue;
                for(i = iblock * 8;i < (iblock + 1)*8;i++)
                    for(j = jblock * 8;j < (jblock + 1)*8;j++){
                        tmp = A[i][j];
                        B[j][i] = tmp;
                }
            }
        for(iblock = 0;iblock < N / 8;iblock++){
            for (i = iblock * 8; i < (iblock + 1) * 8; i++)
            {
            for(j = iblock * 8;j < (iblock + 1) * 8; j++){
                    if(i == j)  continue;   
                    tmp = A[i][j];
                    B[j][i] = tmp;           
            }
            tmp = A[i][i];
            B[i][i] = tmp; 
            }
        }
    }

    else if(N == 67){
        multy = 21;
        for(iblock = 0;iblock < N / multy;iblock++)
            for(jblock = 0;jblock < M / multy;jblock++){
                if(iblock == jblock)
                    continue;
                for(i = iblock * multy;i < (iblock + 1)*multy;i++)
                    for(j = jblock * multy;j < (jblock + 1)*multy;j++){
                        tmp = A[i][j];
                        B[j][i] = tmp;
                }
            }
        for(iblock = 0;iblock < M / multy;iblock++){
            for (i = iblock * multy; i < (iblock + 1) * multy; i++)
            {
            for(j = iblock * multy;j < (iblock + 1) * multy; j++){
                    if(i == j)  continue;   
                    tmp = A[i][j];
                    B[j][i] = tmp;           
            }
            tmp = A[i][i];
            B[i][i] = tmp; 
            }
        }
        for(i = 0;i < (N / multy) * multy;i++)
            for(j = (M / multy) * multy; j < M; j++){
                tmp = A[i][j];
                B[j][i] = tmp;                   
            }
        for(i = (N / multy) * multy;i < N;i++)
            for(j = 0;j < (M / multy) * multy;j++){
                tmp = A[i][j];
                B[j][i] = tmp;                  
            }
        for(i = (N / multy) * multy;i < N;i++)
            for(j = (M / multy) * multy;j < M;j++){
                tmp = A[i][j];
                B[j][i] = tmp;                 
            }        
    }
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

