/*
 * ===========================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/20/2021 07:36:30 PM
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
#include "SRBio.h"

int main(int argc, char **argv){
    if (argc != 2 && argc != 3){
        fprintf(stderr, "Usage: ./main filename [compress mode]\n");
        return -1;
    }
    rb_file_compress_t flag = 0;
    if (argc == 3)
        flag = (rb_file_compress_t)strtol(argv[2], NULL, 10);
    rb_matrix_info_t mat;
    SRB_init(&mat);
    int info = SRB_read(argv[1], &mat, flag);

    if (info){
        printf("SRB_read exited with error (%d)\n", info);
    } else {
        printf("SRB_read exited successfully.\n");
    }

    info = SRB_write_p("rbmat", &mat, 4, flag);

    if (info){
        printf("SRB_write_p exited with error (%d)\n", info);
    } else {
        printf("SRB_write_p exited successfully.\n");
    }

    SRB_destroy(&mat);
    return 0;
}
