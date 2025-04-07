#ifndef GIFMETADATA_CLI_H
#define GIFMETADATA_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct cli_args {
    int all_flag;
    int verbose_flag;
    int dev_flag;
    int help_flag;
    char *filename;
    size_t filename_size;
};

void free_cli_args(struct cli_args *args);
struct cli_args *cli_parse(int argc, char **argv);

#endif
