#ifndef GIFMETADATA_CLI_H
#define GIFMETADATA_CLI_H

#include <stdio.h>
#include <stdlib.h>

struct cli_args {
    int all_flag;
    int verbose_flag;
    int dev_flag;
    char *filename;
};

struct cli_args *parse_args(int argc, char **argv);

#endif