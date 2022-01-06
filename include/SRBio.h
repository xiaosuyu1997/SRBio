/*
 * ===========================================================================
 *
 *       Filename:  SRBio.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/20/2021 07:08:16 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoyang Liu (), liuhaoyang@pku.edu.cn
 *   Organization:  BICMR, Peking University
 *
 * ===========================================================================
 */

#ifndef SRBIO_H
#define SRBIO_H

#include "SRBio_config.h"

#ifdef SRBIO_ILP64
typedef long int SRB_INT;
#else
typedef int SRB_INT;
#endif

#ifdef SRBIO_SINGLE_PRECISION
typedef float SRB_Scalar;
#else
#ifndef SRBIO_DOUBLE_PRECISION
#define SRBIO_DOUBLE_PRECISION
#endif // of ifndef SRBIO_DOUBLE_PRECISION
typedef double SRB_Scalar;
#endif

// max characters per line in RB format file
#define SRBIO_LINE_MAX 80

// max precision
#define SRBIO_MAX_PRECISION 16

struct rb_matrix_info {
    char descr[73];
    char key[9];
    char mtype;
    char stype;
    char ftype;
    SRB_INT rows;
    SRB_INT cols;
    SRB_INT nnz;

    // for csc
    SRB_INT *colptr;
    SRB_INT *rowind;
    SRB_Scalar *valptr_d;
    SRB_INT *valptr_i;

    // for element-wise
};

enum rb_file_compress {
    SRB_COMPRESS_NONE = 0,
    SRB_COMPRESS_GZIP,
    SRB_COMPRESS_BZIP2
};

typedef struct rb_matrix_info rb_matrix_info_t;
typedef enum rb_file_compress rb_file_compress_t;

typedef void *(*SRB_open_f)(const char *, const char *);
typedef void (*SRB_close_f)(void *);
typedef char *(*SRB_gets_f)(char *, int, void *);
typedef int (*SRB_puts_f)(const char*, void*);

int SRB_read(const char *, rb_matrix_info_t*, rb_file_compress_t);
int SRB_write(const char *, const rb_matrix_info_t*, rb_file_compress_t);
int SRB_write_p(const char *, const rb_matrix_info_t*, int, rb_file_compress_t);
void SRB_init(rb_matrix_info_t*);
void SRB_destroy(rb_matrix_info_t*);
void SRB_print(const rb_matrix_info_t*);
int SRB_digits(SRB_INT);

#endif
