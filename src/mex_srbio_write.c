/*
 * ===========================================================================
 *
 *       Filename:  mex_srbio_write.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/25/2021 11:17:15 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoyang Liu (), liuhaoyang@pku.edu.cn
 *   Organization:  BICMR, Peking University
 *      Copyright:  Copyright (c) 2021, Haoyang Liu
 *
 * ===========================================================================
 */

#include <stdlib.h>
#include <string.h>
#include "mex.h"

#define SRBIO_ILP64
#include "SRBio.h"

/* mex_srbio_write(filename, A, descr, key, precision, sym_flag, compress_flag)
 */

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]){
    if (nlhs != 0 || nrhs != 7){
        mexErrMsgTxt("Usage: mex_srbio_write(filename, A, descr, key, precision, sym_flag, compress_flag)");
        return;
    }
    int len, info;
    rb_matrix_info_t mat;

    // read input args
    len = mxGetN(prhs[0]);
    char filename[len + 1];
    info = mxGetString(prhs[0], filename, len + 1);
    if (info != 0){
        mexErrMsgTxt("Error occured when invoking mxGetString");
        return;
    }

    len = mxGetN(prhs[2]);
    char descr[len + 1];
    info = mxGetString(prhs[2], descr, len + 1);
    if (info != 0){
        mexErrMsgTxt("Error occured when invoking mxGetString");
        return;
    }

    len = mxGetN(prhs[3]);
    char key[len + 1];
    info = mxGetString(prhs[3], key, len + 1);
    if (info != 0){
        mexErrMsgTxt("Error occured when invoking mxGetString");
        return;
    }

    mwSize rows = mxGetM(prhs[1]);
    mwSize cols = mxGetN(prhs[1]);
    mwIndex *rowind = mxGetIr(prhs[1]);
    mwIndex *colptr = mxGetJc(prhs[1]);
    double *valptr = mxGetPr(prhs[1]);
    mwSize nnz = colptr[cols];

    int precision = (int)*mxGetPr(prhs[4]);
    int issym = (int)*mxGetPr(prhs[5]);
    int flag = (int)*mxGetPr(prhs[6]);

    // create rb_matrix
    strncpy(mat.descr, descr, 72);
    strncpy(mat.key, key, 8);
    mat.cols = cols;
    mat.rows = rows;
    mat.nnz = nnz;
    mat.mtype = 'r';
    mat.stype = issym ? 's' : 'u';
    mat.ftype = 'a';

    // matlab uses 0-based indexing
    mat.colptr = (SRB_INT*)malloc((cols + 1) * sizeof(SRB_INT));
    if (mat.colptr == NULL){
        mexErrMsgTxt("Failed to allocate memory (colptr)");
        return;
    }
    mat.rowind = (SRB_INT*)malloc((nnz) * sizeof(SRB_INT));
    if (mat.rowind == NULL){
        mexErrMsgTxt("Failed to allocate memory (rowind)");
        free(mat.colptr);
        return;
    }
    mat.valptr_d = valptr;

    for (SRB_INT i = 0; i < mat.cols + 1; ++i)
        mat.colptr[i] = colptr[i] + 1;
    for (SRB_INT i = 0; i < mat.nnz; ++i)
        mat.rowind[i] = rowind[i] + 1;

    // call SRB_write_p
    info = SRB_write_p(filename, &mat, precision, flag);
    if (info != 0){
        mexErrMsgTxt("IO Error: SRB_write_p exited with non-zero return code");
    }

    free(mat.colptr);
    free(mat.rowind);
}

