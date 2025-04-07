#include "cli.h"

#define ARG_MAX_LEN 512

void print_help() {
    printf("gifmetadata\n");
    printf("version 0.0.1\n\n");

    printf("Harry Stanton <harry@harrystanton.com>\n\n");
    
    printf("OVERVIEW:\n");
    printf("    GIFs contain 'comments' that were commonly used to attribute copyright\n");
    printf("    and attribution in the early days of the web. Since then, programs have\n");
    printf("    lost the ability to read and write this data.\n\n");
    
    printf("    gifmetadata reads and outputs this data.\n\n");
    
    printf("OUTPUT:\n");
    printf("    gifmetadata can read comments, application extensions and plain text\n");
    printf("    embedded within a GIF.\n\n");
    
    printf("    comments:                Text messages limited to 256 characters, primarily\n");
    printf("                             copyright and attribution messages. Prefixed with\n");
    printf("                             \"comment:\".\n\n");
    
    printf("    application extensions:  Custom extensions to GIFs that applications\n");
    printf("                             may use to add additional features to the GIF.\n");
    printf("                             For example Netscape 2.0 used them to add early\n");
    printf("                             animation looping. Application extensions contain\n");
    printf("                             a name and then 'sub-blocks' of binary data, this\n");
    printf("                             may ping your terminal. Application name prefixed\n");
    printf("                             with \"application:\", sub-blocks with: \"-\".\n\n");
    
    printf("    plain text:              A feature within the 89a specification to display\n");
    printf("                             plain text on-top of images that was never utilized.\n");
    printf("                             Prefixed with \"plain text:\".\n\n");
    
    printf("USAGE: gifmetadata [options] file\n\n");
    
    printf("OPTIONS:\n");
    
    printf("    -h / --help      Display help, options and program info\n");
    printf("    -a / --all       Display all GIF metadata blocks instead of only the comment\n");
    printf("    -v / --verbose   Display more data about the gif, e.g. width/height\n");
    printf("    -d / --dev       Display inner program workings intended for developers\n");
}

void cli_free_user_args(cli_user_args *a) {
    // free filename field
    if (a != NULL && a->filename != NULL)
        free(a->filename);

    // free whole struct
    if (a != NULL)
        free(a);
}

/**
 * @brief Parse a short flag and update a provided @ref cli_user_args structure.
 *
 * The function parses a provided short flag and updates a provided
 * @ref cli_user_args structure by updating the field indicating the presence
 * of the known flag. If a flag wasn't recognized then a 0 is returned and the
 * structure is not modified.
 *
 * @param flag A single character short flag to be parsed.
 *
 * @param a The structure to be modified upon flag parsing.
 *
 * @return If a flag was recognized and the structure was updated then a 1
 *         integer value is returned. If a flag was not recognized then a 0
 *         integer value is returned and the structure is not modified.
 */
int parse_short_flag(char flag, cli_user_args *a) {
    switch (flag) {
    case 'h':
        a->help_flag = 1;
        return 1;
    case 'a':
        a->all_flag = 1;
        return 1;
    case 'v':
        a->verbose_flag = 1;
        return 1;
    case 'd':
        a->dev_flag = 1;
        return 1;
    default:
        return 0;
    }
}

/**
 * @brief Parse a long flag and update a provided @ref cli_user_args structure.
 *
 * The function parses a provided long flag and updates a provided
 * @ref cli_user_args structure by updating the field indicating the presence
 * of the known flag. If a flag wasn't recognized then a 0 is returned and the
 * structure is not modified.
 *
 * @param flag Buffer containing the long flag.
 * 
 * @param flag_len Length of characters within the flag buffer.
 *
 * @param a The structure to be modified upon flag parsing.
 *
 * @return If a flag was recognized and the structure was updated then a 1
 *         integer value is returned. If a flag was not recognized then a 0
 *         integer value is returned and the structure is not modified.
 */
int parse_long_flag(const char *flag, size_t flag_len, cli_user_args *a) {
    if (flag_len == 4 && strncmp(flag, "help", flag_len) == 0) {
        a->help_flag = 1;
        return 1; 
    } else if (flag_len == 3 && strncmp(flag, "all", flag_len) == 0) {
        a->all_flag = 1;
        return 1;
    } else if (flag_len == 7 && strncmp(flag, "verbose", flag_len) == 0) {
        a->verbose_flag = 1;
        return 1;
    } else if (flag_len == 3 && strncmp(flag, "dev", flag_len) == 0) {
        a->dev_flag = 1;
        return 1;
    } else {
        return 0;
    }
}

cli_user_args *cli_parse(int argc, char **argv) {

    cli_user_args *parsed_args = malloc(sizeof(cli_user_args));
    if (parsed_args == NULL) {
        fprintf(stderr, "[error] failed to alloc parsed args\n");
        return NULL;
    }

    // loop each command line arguments
    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        // unlikely event of a null argv member
        if (arg == NULL) {
            fprintf(stderr, "[error] missing arg at index %d\n", i);
            cli_free_user_args(parsed_args);
            return NULL;
        }
        // determine len of arg for looping character-by-character
        size_t arg_len = strnlen(arg, ARG_MAX_LEN + 1);
        if (arg_len == ARG_MAX_LEN + 1) {
            fprintf(stderr, "[error] arg exceeds max character len %d\n",
                ARG_MAX_LEN);
            cli_free_user_args(parsed_args);
            return NULL;
        }
        
        // determine the number of preceding dashes and break when encountering
        // no more
        int dash_count = 0;
        for (int arg_i = 0; arg_i < arg_len; arg_i++) {
            if (arg[arg_i] == '-')
                dash_count++;
            else
                break;
        }

        if (dash_count == 1 && arg_len > 1) {
            // parse short flag and update struct
            for (int arg_i = 1; arg_i < arg_len; arg_i++) {
                char flag_c = arg[arg_i];
                if (parse_short_flag(flag_c, parsed_args) != 1) {
                    fprintf(stderr, "[error] invalid flag '%c'\n", flag_c);
                    cli_free_user_args(parsed_args);
                    return NULL;
                }
            }
        } else if (dash_count == 2 && arg_len > 2) {
            // is a long name double dash flag (e.g. --verbose)
            const char *flag_s = arg+2;
            if (parse_long_flag(flag_s, arg_len-2, parsed_args) != 1) {
                fprintf(stderr, "[error] invalid flag '%s'\n", flag_s);
                cli_free_user_args(parsed_args);
                return NULL;
            }
        } else {
            // filename

            // no more one than file can be provided
            if (parsed_args->filename != NULL) {
                fprintf(stderr, "[error] more than one file provided\n");
                cli_free_user_args(parsed_args);
                return NULL;
            }
            // copy the filename to its own buffer
            size_t filename_size = sizeof(char) * arg_len + 1;
            char *filename = malloc(filename_size);
            if (filename == NULL) {
                fprintf(stderr, "[error] failed to alloc filename\n");
                cli_free_user_args(parsed_args);
                return NULL;
            }
            strncpy(filename, arg, filename_size);
            filename[filename_size-1] = '\0';
            parsed_args->filename = filename;
            parsed_args->filename_size = filename_size;
        }
    }

    if (parsed_args->help_flag) {
        print_help();
        cli_free_user_args(parsed_args);
        return NULL;
    }

    return parsed_args;
}

