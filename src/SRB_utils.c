/*
 * ===========================================================================
 *
 *       Filename:  SRB_destroy.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/20/2021 08:40:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoyang Liu (), liuhaoyang@pku.edu.cn
 *   Organization:  BICMR, Peking University
 *      Copyright:  Copyright (c) 2021, Haoyang Liu
 *
 * ===========================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SRBio.h"

void SRB_print_csc(const rb_matrix_info_t *);
void SRB_print_ele(const rb_matrix_info_t *);

int SRB_digits(SRB_INT v){
    int i = 1;
    while ((v = v / 10) != 0) ++i;
    return i;
}

void SRB_init(rb_matrix_info_t *mat){
    mat->rows = 0;
    mat->cols = 0;
    mat->nnz = 0;
    mat->colptr = NULL;
    mat->rowind = NULL;
    mat->valptr_d = NULL;
    mat->valptr_i = NULL;
}

void SRB_destroy(rb_matrix_info_t *mat){
    if (mat->colptr != NULL){
        free(mat->colptr);
        mat->colptr = NULL;
    }

    if (mat->rowind != NULL){
        free(mat->rowind);
        mat->rowind = NULL;
    }

    if (mat->valptr_d != NULL){
        free(mat->valptr_d);
        mat->valptr_d = NULL;
    }

    if (mat->valptr_i != NULL){
        free(mat->valptr_i);
        mat->valptr_i = NULL;
    }
}

void SRB_print(const rb_matrix_info_t *mat){
    switch (mat->ftype){
        case 'a':
            SRB_print_csc(mat);
            break;
        case 'e':
            SRB_print_ele(mat);
            break;
        default:
            break;
    }
}

void SRB_print_csc(const rb_matrix_info_t *mat){
    char storage_str[20], buff[50];
#ifdef SRBIO_ILP64
    char *fmt_header = "%ld times %ld sparse %s matrix with %ld nonzeros:\n\n";
    char *fmt_data = "(%ld, %ld)";
#else
    char *fmt_header = "%d times %d sparse %s matrix with %d nonzeros:\n\n";
    char *fmt_data = "(%d, %d)";
#endif
    switch (mat->stype){
        case 's':
            strncpy(storage_str, "symmetric", 20);
            break;
        case 'u':
            strncpy(storage_str, "unsymmetric", 20);
            break;
        case 'h':
            strncpy(storage_str, "Hermitian", 20);
            break;
        case 'z':
            strncpy(storage_str, "skew symmetric", 20);
            break;
        case 'r':
            strncpy(storage_str, "rectangular", 20);
            break;
        default:
            strncpy(storage_str, "(unknown type)", 20);
            break;
    }
    printf(fmt_header, mat->rows, mat->cols, storage_str, mat->nnz);
    int drows = SRB_digits(mat->rows);
    int dcols = SRB_digits(mat->cols);
    int len = drows + dcols + 8;
    for (int i = 0; i < mat->cols; ++i){
        for (int j = mat->colptr[i]; j < mat->colptr[i+1]; ++j){
            snprintf(buff, 50, fmt_data, mat->rowind[j-1], i + 1);
            if (mat->mtype == 'r')
                printf("%*s  %9.4e\n", len, buff, mat->valptr_d[j-1]);
            else if (mat->mtype == 'i')
#ifdef SRBIO_ILP64
                printf("%*s  %ld\n", len, buff, mat->valptr_i[j-1]);
#else
                printf("%*s  %d\n", len, buff, mat->valptr_i[j-1]);
#endif
            else if (mat->mtype == 'p')
                printf("%20s\n", buff);
        }
    }
}

void SRB_print_ele(const rb_matrix_info_t *mat){
    printf("Not implemented");
}
