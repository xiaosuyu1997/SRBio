/*
 * ===========================================================================
 *
 *       Filename:  wrap.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/22/2021 10:53:39 AM
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

#ifdef SRBIO_USE_ZLIB
#include <zlib.h>
#endif

#ifdef SRBIO_USE_BZIP2
#include <bzlib.h>
#endif

#include "private/wrap.h"

void *SRB_fopen(const char *filename, const char *mode){
    return fopen(filename, mode);
}

void SRB_fclose(void *p){
    fclose((FILE*)p);
}

char *SRB_fgets(char *buff, int size, void *p){
    return fgets(buff, size, (FILE*)p);
}

int SRB_fputs(const char *buff, void *p){
    return fputs(buff, (FILE*)p);
}

#ifdef SRBIO_USE_ZLIB

void *SRB_gzopen(const char *filename, const char *mode){
    return gzopen(filename, mode);
}

void SRB_gzclose(void *p){
    gzclose((gzFile)p);
}

char *SRB_gzgets(char *buff, int size, void *p){
    return gzgets((gzFile)p, buff, size);
}

int SRB_gzputs(const char *buff, void *p){
    return gzputs((gzFile)p, buff);
}

#endif

#ifdef SRBIO_USE_BZIP2
void *SRB_bz2open(const char *filename, const char *mode){
    FILE *fp;
    BZFILE *bzf;
    int info;
    char mode_b[10] = {'\0'};
    char rw = 'r';

    // check mode
    if (strstr(mode, "r"))
        rw = 'r';
    else if (strstr(mode, "w") || strstr(mode, "a"))
        rw = 'w';

    // add 'b' for non-UNIX platforms
    strcpy(mode_b, mode);
    if (!strstr(mode_b, "b")){
        strcat(mode_b, "b");
    }

    fp = fopen(filename, mode_b);
    if (fp == NULL)
        return NULL;

    if (rw == 'w')
#ifndef NDEBUG
        bzf = BZ2_bzWriteOpen(&info, fp, 9, 1, 0);
#else
        bzf = BZ2_bzWriteOpen(&info, fp, 9, 0, 0);
#endif
    else
        bzf = BZ2_bzReadOpen(&info, fp, 0, 0, NULL, 0);

    if (info != BZ_OK){
        if (rw == 'w')
            BZ2_bzWriteClose(&info, bzf, 0, NULL, NULL);
        else
            BZ2_bzReadClose(&info, bzf);
        fclose(fp);
        return NULL;
    }

    rb_bzip2_file_t *rb_bzf = (rb_bzip2_file_t*)malloc(sizeof(rb_bzip2_file_t));
    if (rb_bzf == NULL){
        if (rw == 'w')
            BZ2_bzWriteClose(&info, bzf, 0, NULL, NULL);
        else
            BZ2_bzReadClose(&info, bzf);
        fclose(fp);
        return NULL;
    }

    rb_bzf->mode = rw;
    rb_bzf->f = fp;
    rb_bzf->bzf = bzf;
    rb_bzf->ipos = SRBIO_BZ2_BUFF_SIZE;
    return rb_bzf;
}


void SRB_bz2close(void *p){
    int info;
    rb_bzip2_file_t *rb_bzf = (rb_bzip2_file_t*)p;

    if (rb_bzf->mode == 'r')
        BZ2_bzReadClose(&info, rb_bzf->bzf);
    else {
#ifndef NDEBUG
        unsigned int nbytes_in, nbytes_out;
        BZ2_bzWriteClose(&info, rb_bzf->bzf, 0, &nbytes_in, &nbytes_out);
        printf("BZ2: original: %u bytes; compressed: %u bytes.\n", nbytes_in, nbytes_out);
#else
        BZ2_bzWriteClose(&info, rb_bzf->bzf, 0, NULL, NULL);
#endif
    }
    fclose(rb_bzf->f);
    free(rb_bzf);
}


char *SRB_bz2gets(char *buff, int size, void *p){
    int info, i;
    rb_bzip2_file_t *rb_bzf = (rb_bzip2_file_t*)p;

    // copy at most (size - 1) chars from rb_bzf->buffer to buff
    for (i = 0; i < size - 1; ++i, ++rb_bzf->ipos){
        // if the end of rb_bzf->buffer is reached,
        // then read another data block
        if (rb_bzf->ipos == SRBIO_BZ2_BUFF_SIZE){
            BZ2_bzRead(&info, rb_bzf->bzf, rb_bzf->buffer, SRBIO_BZ2_BUFF_SIZE);
            if (info != BZ_OK && info != BZ_STREAM_END)
                return NULL;
            rb_bzf->ipos = 0;
        }

        buff[i] = rb_bzf->buffer[rb_bzf->ipos];
        if (buff[i] == '\n' || buff[i] == '\r'){
            // discard CR/CRLF
            while (rb_bzf->buffer[rb_bzf->ipos] == '\n' ||
                    rb_bzf->buffer[rb_bzf->ipos] == '\r') ++rb_bzf->ipos;
            break;
        }
    }
    
    // append '\0'
    buff[i] = '\0';

    return buff;
}

int SRB_bz2puts(const char *buff, void *p){
    int len = strlen(buff);
    int info;
    
    // write to BZ2 file
    BZ2_bzWrite(&info, ((rb_bzip2_file_t*)p)->bzf, (void*)buff, len);

    return len;
}
#endif

