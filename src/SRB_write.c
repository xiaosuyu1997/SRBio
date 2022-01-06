/*
 * ===========================================================================
 *
 *       Filename:  SRB_write.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/24/2021 05:33:28 PM
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

int SRB_write_csc_impl(void*, const rb_matrix_info_t*, int, SRB_puts_f);
int SRB_write_impl(const char *, const rb_matrix_info_t*, int, SRB_open_f, SRB_close_f, SRB_puts_f);

int SRB_write(const char *filename, const rb_matrix_info_t *mat, rb_file_compress_t flag){
    return SRB_write_p(filename, mat, -1, flag);
}

int SRB_write_p(const char *filename, const rb_matrix_info_t *mat,
        int precision, rb_file_compress_t flag){
    SRB_puts_f rb_puts;
    SRB_close_f rb_close;
    SRB_open_f rb_open;
    switch (flag) {
        case SRB_COMPRESS_NONE:
            rb_open = SRB_fopen;
            rb_close = SRB_fclose;
            rb_puts = SRB_fputs;
            break;
#ifdef SRBIO_USE_ZLIB
        case SRB_COMPRESS_GZIP:
            rb_open = SRB_gzopen;
            rb_close = SRB_gzclose;
            rb_puts = SRB_gzputs;
            break;
#endif
#ifdef SRBIO_USE_BZIP2
        case SRB_COMPRESS_BZIP2:
            rb_open = SRB_bz2open;
            rb_close = SRB_bz2close;
            rb_puts = SRB_bz2puts;
            break;
#endif
        default:
            return -999;
    }

    // target precision
    // <0: auto: 7 for float, 15 for double
    // 0-16: user-defined precision
    if (precision > SRBIO_MAX_PRECISION)
        precision = SRBIO_MAX_PRECISION;

    if (precision < 0){ // auto-determined
        precision = 2 * sizeof(SRB_Scalar) - 1;
    }

#ifndef NDEBUG
    printf("Compress mode: %d | Precision: %d\n", flag, precision);
#endif

    return SRB_write_impl(filename, mat, precision, rb_open, rb_close, rb_puts);
}

int SRB_write_impl(const char *filename, const rb_matrix_info_t *mat, int precision,
        SRB_open_f rb_open, SRB_close_f rb_close, SRB_puts_f rb_puts){
    void *fp;
    char buffer[SRBIO_LINE_MAX + 2];
    int ret;

    fp = rb_open(filename, "w");
    if (fp == NULL){
        fprintf(stderr, "Failed to open file: %s.\n", filename);
        return -100;
    }

#ifndef NDEBUG
    printf("Writing to %s\n", filename);
#endif

    // line 1: title and id
    snprintf(buffer, SRBIO_LINE_MAX + 2, "%-72s%-8s\n", mat->descr, mat->key);
    rb_puts(buffer, fp);

#ifndef NDEBUG
    printf("Writing title and id\n");
#endif

    // line 2-end:
    ret = -999;
    if (mat->ftype == 'a') // csc format
        ret = SRB_write_csc_impl(fp, mat, precision, rb_puts);
    else if (mat->ftype == 'e') // elemental format
        ret = -999;

    rb_close(fp);
    return ret;
}

int SRB_write_csc_impl(void *fp, const rb_matrix_info_t *mat, int precision, SRB_puts_f rb_puts){
    SRB_INT totcrd, ptrcrd, indcrd, valcrd;
    int ptr_w, ptr_n, ind_w, ind_n, val_w, val_n;
    char buffer[SRBIO_LINE_MAX + 2];

    // ptrcrd
    ptr_w = 1 + SRB_digits(1 + mat->nnz);
    ptr_n = SRBIO_LINE_MAX / ptr_w;
    ptrcrd = (1 + mat->cols) / ptr_n;
    if ((1 + mat->cols) % ptr_n != 0) ++ptrcrd;

    // indcrd
    ind_w = 1 + SRB_digits(mat->rows);
    ind_n = SRBIO_LINE_MAX / ind_w;
    indcrd = mat->nnz / ind_n;
    if (mat->nnz % ind_n != 0) ++indcrd;

    // valcrd
    // (-)X.YYYYYYYYYE[+-]ZZZ for real
    // (-)XXXXX for integer
    switch (mat->mtype){
        case 'r':
            val_w = 9 + precision;
            break;
        case 'i':
            val_w = 10; // FIXIT: max element
            break;
        case 'c':
            return -999;
        default:
            val_w = 1;
            break;
    }
    val_n = SRBIO_LINE_MAX / val_w;
    if (mat->mtype == 'r' || mat->mtype == 'i' || mat->mtype == 'c'){
        valcrd = mat->nnz / val_n;
        if (mat->nnz % val_n != 0) ++valcrd;
    }
    else {
        val_n = 0;
        valcrd = 0;
    }

    totcrd = ptrcrd + indcrd + valcrd;

#ifndef NDEBUG
    printf("ptr: %ld, %d, %d\n", (long)ptrcrd, ptr_n, ptr_w);
    printf("ind: %ld, %d, %d\n", (long)indcrd, ind_n, ind_w);
    printf("val: %ld, %d, %d\n", (long)valcrd, val_n, val_w);
#endif

    // line 2: line info
    snprintf(buffer, SRBIO_LINE_MAX + 2, "%14ld %13ld %13ld %13ld\n",
            (long)totcrd, (long)ptrcrd, (long)indcrd, (long)valcrd);
    rb_puts(buffer, fp);

#ifndef NDEBUG
    printf("Writing line info\n");
#endif

    // line 3: matrix info
    snprintf(buffer, SRBIO_LINE_MAX + 2, "%c%c%c            %13ld %13ld %13ld %13ld\n",
            mat->mtype, mat->stype, mat->ftype,
            (long)mat->rows, (long)mat->cols, (long)mat->nnz, 0L);
    rb_puts(buffer, fp);

#ifndef NDEBUG
    printf("Writing matrix info: rows = %ld, cols = %ld, nnz = %ld\n",
            (long)mat->rows, (long)mat->cols, (long)mat->nnz);
#endif

    // line 4: FORTRAN format
    char ptrfmt[17], indfmt[17], valfmt[21];
    snprintf(ptrfmt, 17, "(%dI%d)", ptr_n, ptr_w);
    snprintf(indfmt, 17, "(%dI%d)", ind_n, ind_w);
    switch (mat->mtype){
        case 'r':
            snprintf(valfmt, 21, "(%dE%d.%d)", val_n, val_w, precision);
            break;
        case 'i':
        case 'p':
            snprintf(valfmt, 21, "(%dI%d)", val_n, val_w);
            break;
    }
    snprintf(buffer, SRBIO_LINE_MAX + 2, "%-16s%-16s%-20s\n", ptrfmt, indfmt, valfmt);
    rb_puts(buffer, fp);

#ifndef NDEBUG
    printf("Writing FORTRAN format info\n");
#endif

    // data block: ptr
    SRB_INT n = 0;
    for (SRB_INT i = 0; i < ptrcrd; ++i){
        int ipos = 0;
        for (int j = 0; j < ptr_n && n < mat->cols + 1; ++j, ++n){
            ipos += snprintf(buffer + ipos, SRBIO_LINE_MAX + 2 - ipos, "%*ld", ptr_w, (long)mat->colptr[n]);
        }
        snprintf(buffer + ipos, 2, "\n");
        rb_puts(buffer, fp);
    }

#ifndef NDEBUG
    printf("Writing data block: colptr\n");
#endif

    // data block: ind
    n = 0;
    for (SRB_INT i = 0; i < indcrd; ++i){
        int ipos = 0;
        for (int j = 0; j < ind_n && n < mat->nnz; ++j, ++n){
            ipos += snprintf(buffer + ipos, SRBIO_LINE_MAX + 2 - ipos, "%*ld", ind_w, (long)mat->rowind[n]);
        }
        snprintf(buffer + ipos, 2, "\n");
        rb_puts(buffer, fp);
    }
    
#ifndef NDEBUG
    printf("Writing data block: rowind\n");
#endif

    // data block: value
    switch (mat->mtype){
        case 'r':
            n = 0;
            for (SRB_INT i = 0; i < valcrd; ++i){
                int ipos = 0;
                for (int j = 0; j < val_n && n < mat->nnz; ++j, ++n){
                    ipos += snprintf(buffer + ipos, SRBIO_LINE_MAX + 2 - ipos, 
                            "%*.*e", val_w, precision, mat->valptr_d[n]);
                }
                snprintf(buffer + ipos, 2, "\n");
                rb_puts(buffer, fp);
            }
            break;
        case 'i':
            n = 0;
            for (SRB_INT i = 0; i < valcrd; ++i){
                int ipos = 0;
                for (int j = 0; j < val_n && n < mat->nnz; ++j, ++n){
                    ipos += snprintf(buffer + ipos, SRBIO_LINE_MAX + 2 - ipos, 
                            "%*ld", val_w, (long)mat->valptr_i[n]);
                }
                snprintf(buffer + ipos, 2, "\n");
                rb_puts(buffer, fp);
            }
            break;
        case 'c':
            return -999;
        case 'p':
            break;
        case 'q':
            break;
        default:
            return -999;
    }

#ifndef NDEBUG
    printf("Writing data block: valptr\n");
#endif

    return 0;
}
