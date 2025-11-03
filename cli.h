// gifmetadata
// Copyright (C) 2025  Harry Stanton
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef GIFMETADATA_CLI_H
#define GIFMETADATA_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_ARG_MAX_LEN 4096

#define CLI_SUCCESS 0
#define CLI_INVALID_FLAG -1
#define CLI_ALLOC_FAILURE -2
#define CLI_EXCEEDS_ARG_MAX_LEN -3
#define CLI_MULTIPLE_INPUTS -4
#define CLI_MULTIPLE_OUTPUTS -5
#define CLI_MISSING_FLAG_ARG -6

typedef struct cli_flag_arg {
    char *string;
    size_t string_len;
    struct cli_flag_arg *next;
} cli_flag_arg;

typedef struct cli_user_args {
    int all_flag;
    int verbose_flag;
    int debug_flag;
    int help_flag;
    cli_flag_arg *comment_flags;
    cli_flag_arg *output_flag;

    char invalid_flag;

    char *filename;
    size_t filename_size;
} cli_user_args;

cli_user_args *cli_new_user_args();
void cli_free_user_args(cli_user_args *a);
int cli_parse(cli_user_args *a, int argc, char **argv);

#endif
