/*
 * ===========================================================================
 *
 *       Filename:  SRB_read.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/20/2021 07:19:37 PM
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
#include "private/wrap.h"

int SRB_read_csc_impl(void*, rb_matrix_info_t*, SRB_gets_f, SRB_INT, SRB_INT, SRB_INT);
int SRB_read_impl(const char *, rb_matrix_info_t*, SRB_open_f, SRB_close_f, SRB_gets_f);

int SRB_read(const char *filename, rb_matrix_info_t *mat, rb_file_compress_t flag){
    SRB_gets_f rb_gets;
    SRB_close_f rb_close;
    SRB_open_f rb_open;
    switch (flag) {
        case SRB_COMPRESS_NONE:
            rb_open = SRB_fopen;
            rb_close = SRB_fclose;
            rb_gets = SRB_fgets;
            break;
#ifdef SRBIO_USE_ZLIB
        case SRB_COMPRESS_GZIP:
            rb_open = SRB_gzopen;
            rb_close = SRB_gzclose;
            rb_gets = SRB_gzgets;
            break;
#endif
#ifdef SRBIO_USE_BZIP2
        case SRB_COMPRESS_BZIP2:
            rb_open = SRB_bz2open;
            rb_close = SRB_bz2close;
            rb_gets = SRB_bz2gets;
            break;
#endif
        default:
            return -999;
    }
    return SRB_read_impl(filename, mat, rb_open, rb_close, rb_gets);
}

int SRB_read_impl(const char *filename, rb_matrix_info_t* mat,
        SRB_open_f rb_open, SRB_close_f rb_close, SRB_gets_f rb_gets){
    void *fp;
    char buffer[SRBIO_LINE_MAX + 2];
    int ret;
    SRB_INT totcrd, ptrcrd, indcrd, valcrd;

    fp = rb_open(filename, "r");
    if (fp == NULL){
        fprintf(stderr, "Failed to open file: %s.\n", filename);
        return -100;
    }

#ifndef NDEBUG
    printf("Successfully opened file %s\n", filename);
#endif

    // line 1: title and id
    if (!rb_gets(buffer, SRBIO_LINE_MAX + 2, fp)){
        fprintf(stderr, "SRB_read: failed to read line 1.");
        ret = -1;
        goto FINALIZE;
    }

    ret = snprintf(mat->descr, 73, "%s", buffer);
    ret = snprintf(mat->key, 9, "%s", buffer + 72);

    // truncate trailing spaces
    for (int j = 72; mat->descr[j] == ' '; --j) mat->descr[j] = '\0';
    for (int j = 8; mat->descr[j] == ' '; --j) mat->descr[j] = '\0';

#ifndef NDEBUG
    printf("Description is %s\n", mat->descr);
    printf("Key is %s\n", mat->key);
#endif

    // line 2: lines info
    if (!rb_gets(buffer, SRBIO_LINE_MAX + 2, fp)){
        fprintf(stderr, "SRB_read: failed to read line 2.\n");
        ret = -2;
        goto FINALIZE;
    }

#ifdef SRBIO_ILP64
    ret = sscanf(buffer, "%ld %ld %ld %ld", &totcrd, &ptrcrd, &indcrd, &valcrd);
#else
    ret = sscanf(buffer, "%d %d %d %d", &totcrd, &ptrcrd, &indcrd, &valcrd);
#endif
    if (ret != 4){
        fprintf(stderr, "SRB_read: line 2 is illegal.\n");
        ret = -2;
        goto FINALIZE;
    }

#ifndef NDEBUG
    printf("# lines (total): %d\n", (int)totcrd);
    printf("# lines (ptr): %d\n", (int)ptrcrd);
    printf("# lines (ind): %d\n", (int)indcrd);
    printf("# lines (val): %d\n", (int)valcrd);
#endif

    // line 3: matrix info
    if (!rb_gets(buffer, SRBIO_LINE_MAX + 2, fp)){
        fprintf(stderr, "SRB_read: failed to read line 3.\n");
        ret = -3;
        goto FINALIZE;
    }

    ret = sscanf(buffer, "%c%c%c", &mat->mtype, &mat->stype, &mat->ftype);
    if (mat->mtype < 'a') mat->mtype += 'a' - 'A';
    if (mat->stype < 'a') mat->stype += 'a' - 'A';
    if (mat->ftype < 'a') mat->ftype += 'a' - 'A';

    if (mat->ftype == 'a'){
#ifdef SRBIO_ILP64
        ret = sscanf(buffer + 3, "%ld %ld %ld", &mat->rows, &mat->cols, &mat->nnz);
#else
        ret = sscanf(buffer + 3, "%d %d %d", &mat->rows, &mat->cols, &mat->nnz);
#endif
        if (ret != 3){
            fprintf(stderr, "SRB_read: line 3 is illegal.\n");
            ret = -3;
            goto FINALIZE;
        }
    } else if (mat->ftype == 'e'){
        fprintf(stderr, "SRB_read: elemental format is not supported yet.\n");
        ret = -999;
        goto FINALIZE;
    } else {
        fprintf(stderr, "SRB_read: unknown format: %c\n", mat->ftype);
        ret = -31;
        goto FINALIZE;
    }
    
    // line 4: fortran format info
    // for C program, just discard it
    if (!rb_gets(buffer, SRBIO_LINE_MAX + 2, fp)){
        fprintf(stderr, "SRB_read: failed to read line 4.\n");
        ret = -4;
        goto FINALIZE;
    }

    // data block
    if (mat->ftype == 'a'){
        ret = SRB_read_csc_impl(fp, mat, rb_gets, ptrcrd, indcrd, valcrd);
    }

FINALIZE:
    rb_close(fp);
    return ret;
}

int SRB_read_csc_impl(void *fp, rb_matrix_info_t *mat, SRB_gets_f rb_gets,
        SRB_INT nl_ptr, SRB_INT nl_ind, SRB_INT nl_val){
    mat->colptr = (SRB_INT*)malloc((1 + mat->cols) * sizeof(SRB_INT));
    mat->rowind = (SRB_INT*)malloc(mat->nnz * sizeof(SRB_INT));
    mat->valptr_d = NULL;
    mat->valptr_i = NULL;
    char buffer[SRBIO_LINE_MAX + 2], *chret;

    // colptr block
    SRB_INT n = 0;
    for (SRB_INT i = 0; i < nl_ptr; ++i){
        chret = rb_gets(buffer, SRBIO_LINE_MAX + 2, fp);
        if (chret == NULL){
            fprintf(stderr, "SRB_read_csc_impl: file corrupted at line %d",
                    (int)(i + 4));
            SRB_destroy(mat);
            return -1;
        }
        SRB_INT num_per_line = (mat->cols + 1) / nl_ptr;
        if ((mat->cols + 1) % nl_ptr > 0) ++num_per_line;
        for (SRB_INT j = 0; j < num_per_line && n < mat->cols + 1; ++j, ++n){
            mat->colptr[n] = strtol(chret, &chret, 10);
        }
    }

    // rowind block
    n = 0;
    for (SRB_INT i = 0; i < nl_ind; ++i){
        chret = rb_gets(buffer, SRBIO_LINE_MAX + 2, fp);
        if (chret == NULL){
            fprintf(stderr, "SRB_read_csc_impl: file corrupted at line %d",
                    (int)(i + 4 + nl_ptr));
            SRB_destroy(mat);
            return -2;
        }
        SRB_INT num_per_line = (mat->nnz) / nl_ind;
        if ((mat->nnz) % nl_ind > 0) ++num_per_line;
        for (SRB_INT j = 0; j < num_per_line && n < mat->nnz; ++j, ++n){
            mat->rowind[n] = strtol(chret, &chret, 10);
        }
    }

    // value block
    switch (mat->mtype){
        case 'r': // real
            mat->valptr_d = (SRB_Scalar*)malloc(mat->nnz * sizeof(SRB_Scalar));
            n = 0;
            for (SRB_INT i = 0; i < nl_val; ++i){
                chret = rb_gets(buffer, SRBIO_LINE_MAX + 2, fp);
                if (chret == NULL){
                    fprintf(stderr, "SRB_read_csc_gzip: file corrupted at line %d",
                            (int)(i + 4 + nl_ptr + nl_ind));
                    SRB_destroy(mat);
                    return -3;
                }
                SRB_INT num_per_line = (mat->nnz) / nl_val;
                if ((mat->nnz) % nl_val > 0) ++num_per_line;
                for (SRB_INT j = 0; j < num_per_line && n < mat->nnz; ++j, ++n){
                    mat->valptr_d[n] = strtod(chret, &chret);
                }
            }
            break;
        case 'c': // complex
            fprintf(stderr, "SRB_read: complex is not supported.\n");
            return -999;
            break;
        case 'i': // integer
            mat->valptr_i = (SRB_INT*)malloc(mat->nnz * sizeof(SRB_INT));
            n = 0;
            for (SRB_INT i = 0; i < nl_val; ++i){
                chret = rb_gets(buffer, SRBIO_LINE_MAX + 2, fp);
                if (chret == NULL){
                    fprintf(stderr, "SRB_read_csc_impl: file corrupted at line %d",
                            (int)(i + 4 + nl_ptr + nl_ind));
                    SRB_destroy(mat);
                    return -3;
                }
                SRB_INT num_per_line = (mat->nnz) / nl_val;
                if ((mat->nnz) % nl_val > 0) ++num_per_line;
                for (SRB_INT j = 0; j < num_per_line && n < mat->nnz; ++j, ++n){
                    mat->valptr_i[n] = strtol(chret, &chret, 10);
                }
            }
            break;
        case 'p': // pattern
            break;
        case 'q': // pattern & aux file
            fprintf(stderr, "SRB_read: pattern + aux file is not supported.\n");
            return -999;
        default:  // error
            fprintf(stderr, "SRB_read: illegal type (%c)\n", mat->mtype);
            return -41;
            break;
    }
    return 0;
}

