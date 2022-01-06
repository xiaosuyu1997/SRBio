/*
 * ===========================================================================
 *
 *       Filename:  wrap.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/22/2021 10:51:34 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Haoyang Liu (), liuhaoyang@pku.edu.cn
 *   Organization:  BICMR, Peking University
 *
 * ===========================================================================
 */

#ifndef SRBIO_PRIVATE_WRAP_H
#define SRBIO_PRIVATE_WRAP_H

#include "SRBio_config.h"

#include <stdio.h>
#ifdef SRBIO_USE_BZIP2
#include <bzlib.h>

#define SRBIO_BZ2_BUFF_SIZE 1024

struct rb_bzip2_file {
    BZFILE *bzf;
    FILE *f;
    char mode;
    char buffer[SRBIO_BZ2_BUFF_SIZE];
    int ipos;
};

typedef struct rb_bzip2_file rb_bzip2_file_t;
#endif

void *SRB_fopen(const char *, const char *);
void *SRB_gzopen(const char *, const char *);
void *SRB_bz2open(const char *, const char*);
void SRB_fclose(void*);
void SRB_gzclose(void*);
void SRB_bz2close(void*);
char *SRB_fgets(char *, int, void*);
char *SRB_gzgets(char *, int, void*);
char *SRB_bz2gets(char *, int, void*);
int SRB_fputs(const char *, void*);
int SRB_gzputs(const char *, void*);
int SRB_bz2puts(const char *, void*);


#endif

