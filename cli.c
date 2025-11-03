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

#include "cli.h"

cli_flag_arg *new_cli_flag_arg() {
    cli_flag_arg *new_flag_arg = malloc(sizeof(cli_flag_arg));
    if (new_flag_arg == NULL) {
        return NULL;
    }
    new_flag_arg->string = NULL;
    new_flag_arg->string_len = 0;
    new_flag_arg->next = NULL;
    return new_flag_arg;
}

void append_cli_flag_arg(cli_flag_arg *dst, cli_flag_arg *src) {
    cli_flag_arg *end = dst;
    while (dst->next != NULL) {
        end = end->next;
    }
    end->next = src;
}

cli_user_args *cli_new_user_args() {
    cli_user_args *a = malloc(sizeof(cli_user_args));
    if (a == NULL) {
        return NULL;
    }
    memset(a, 0, sizeof(cli_user_args));
    a->comment_flags = NULL;
    a->output_flag = NULL;
    a->filename = NULL;
    return a;
}

void cli_free_user_args(cli_user_args *a) {
    // free filename field
    if (a != NULL && a->filename != NULL)
        free(a->filename);

    // free comments linked list
    cli_flag_arg *item = a->comment_flags;
    while (item != NULL) {
        if (item->string != NULL) {
            free(item->string);
        }
        cli_flag_arg *to_free = item;
        item = item->next;
        free(to_free);
    } 

    // free whole struct
    if (a != NULL)
        free(a);
}

int cli_parse(cli_user_args *a, int argc, char **argv) {
    cli_flag_arg *awaiting_flag_arg = NULL;

    // loop each command line arguments
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];

        // determine len of arg for looping character-by-character
        size_t arg_len = strnlen(arg, CLI_ARG_MAX_LEN);
        if (arg_len == CLI_ARG_MAX_LEN) {
            return CLI_EXCEEDS_ARG_MAX_LEN;
        }

        // if awaiting flag arg, capture
        if (awaiting_flag_arg != NULL) {
            awaiting_flag_arg->string = malloc(arg_len);
            if (awaiting_flag_arg->string == NULL) {
                return CLI_ALLOC_FAILURE;
            }
            awaiting_flag_arg->string_len = arg_len;
            strncpy(awaiting_flag_arg->string, arg, arg_len);

            awaiting_flag_arg = NULL;
            a->invalid_flag = 0;
            continue;
        }
        
        if (arg[0] == '-' && arg_len > 1) {
            // parse flags
            for (int arg_i = 1; arg_i < arg_len; arg_i++) {
                char flag_c = arg[arg_i];
                switch (flag_c) {
                case 'h':
                    a->help_flag = 1;
                    break;
                case 'a':
                    a->all_flag = 1;
                    break;
                case 'v':
                    a->verbose_flag = 1;
                    break;
                case 'd':
                    a->debug_flag = 1;
                    break;
                case 'c':
                    awaiting_flag_arg = new_cli_flag_arg();
                    if (awaiting_flag_arg == NULL) {
                        return CLI_ALLOC_FAILURE;
                    }
                    if (a->comment_flags != NULL) {
                        append_cli_flag_arg(a->comment_flags, awaiting_flag_arg);
                    } else {
                        a->comment_flags = awaiting_flag_arg;
                    }
                    // used for error messages if the awaiting flag arg is
                    // never fulfilled
                    a->invalid_flag = flag_c;
                    break;
                case 'o':
                    if (a->output_flag != NULL) {
                        printf("ERROR more than one output provided\n");
                    }
                    a->output_flag = new_cli_flag_arg();
                    if (a->output_flag == NULL) {
                        return CLI_ALLOC_FAILURE;
                    }
                    awaiting_flag_arg = a->output_flag;
                    break;
                default:
                    a->invalid_flag = flag_c;
                    return CLI_INVALID_FLAG;
                }
            }
        } else {
            // filename
            if (a->filename != NULL) {
                return CLI_MULTIPLE_INPUTS;
            }
            // copy the filename to its own buffer
            size_t filename_size = arg_len + 1;
            char *filename = malloc(filename_size);
            if (filename == NULL) {
                return CLI_ALLOC_FAILURE;
            }
            strncpy(filename, arg, filename_size);
            filename[filename_size-1] = '\0';
            a->filename = filename;
            a->filename_size = filename_size;
        }
    }

    if (awaiting_flag_arg != NULL) {
        return CLI_MISSING_FLAG_ARG;
    }

    return CLI_SUCCESS;
}

