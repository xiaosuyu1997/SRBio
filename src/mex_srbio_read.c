/*
 * ===========================================================================
 *
 *       Filename:  mex_srbio.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/25/2021 10:43:24 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoyang Liu (), liuhaoyang@pku.edu.cn
 *   Organization:  BICMR, Peking University
 *      Copyright:  Copyright (c) 2021, Haoyang Liu
 *
 * ===========================================================================
 */

#include <string.h>
#include "mex.h"

#define SRBIO_ILP64
#include "SRBio.h"

/* [A, sym_flag] = mex_srbio_read(filename, compress_flag)
 */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){
    rb_matrix_info_t mat;
    SRB_init(&mat);

    if (nlhs != 2 || nrhs != 2){
        mexErrMsgTxt("Usage: [A, sym_flag] = mex_srbio_read(filename, flag)");
        return;
    }

    // read from filename
    mwSize len = mxGetN(prhs[0]);
    char filename[len + 1];
    int info = mxGetString(prhs[0], filename, len + 1);
    if (info != 0){
        mexErrMsgTxt("Error occured when invoking mxGetString");
        return;
    }

    int flag = (int)*mxGetPr(prhs[1]); // flag can be 0, 1, 2, ...

    // call SRB_read
    info = SRB_read(filename, &mat, (rb_file_compress_t)flag);
    if (info != 0){
        mexErrMsgTxt("IO Error: SRB_read exited with non-zero return code");
        SRB_destroy(&mat);
        return;
    }

    // create sparse matrix
    plhs[0] = mxCreateSparse(mat.rows, mat.cols, mat.nnz, mxREAL);
    plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
    if (plhs[0] == NULL){
        mexErrMsgTxt("Failed to create mxArray with type sparse");
        SRB_destroy(&mat);
        return;
    }
    mwIndex *rowind = mxGetIr(plhs[0]);
    mwIndex *colptr = mxGetJc(plhs[0]);
    double *valptr = mxGetPr(plhs[0]);
    
    // matlab needs 0-based indexing
    for (SRB_INT i = 0; i < mat.cols + 1; ++i){
        colptr[i] = mat.colptr[i] - 1;
    }
    for (SRB_INT i = 0; i < mat.nnz; ++i){
        rowind[i] = mat.rowind[i] - 1;
    }
    if (mat.mtype == 'r')
        memcpy(valptr, mat.valptr_d, mat.nnz * sizeof(double));
    else if (mat.mtype == 'i'){
        for (SRB_INT i = 0; i < mat.nnz; ++i)
            valptr[i] = (double)mat.valptr_i[i];
    }

    // record sym_flag
    *mxGetPr(plhs[1]) = mat.stype == 's' ? 1 : 0;
    SRB_destroy(&mat);
}
