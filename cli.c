#include "cli.h"

void print_help() {
    printf("gifmetadata\n");
    printf("version 0.0.1\n\n");

    printf("Harry Stanton <harry@harrystanton.com>\n");
    printf("https://github.com/harrego/gifmetadata\n\n");
    
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

struct defined_arg {
    char *name;
    size_t name_len;
    char short_name;
    int *flag;
};

struct defined_arg defined_args[4];

struct cli_args *cli_parse(int argc, char **argv) {

    struct cli_args *args = malloc(sizeof(struct cli_args));
    args->all_flag = 0;
    args->verbose_flag = 0;
    args->dev_flag = 0;
    args->help_flag = 0;
    args->filename = NULL;

    // setting up defined args

    struct defined_arg *all_arg = &defined_args[0];
    all_arg->name = "all";
    all_arg->name_len = 3;
    all_arg->short_name = 'a';
    all_arg->flag = &args->all_flag;

    struct defined_arg *verbose_arg = &defined_args[1];
    verbose_arg->name = "verbose";
    verbose_arg->name_len = 7;
    verbose_arg->short_name = 'v';
    verbose_arg->flag = &args->verbose_flag;

    struct defined_arg *dev_arg = &defined_args[2];
    dev_arg->name = "dev";
    dev_arg->name_len = 3;
    dev_arg->short_name = 'd';
    dev_arg->flag = &args->dev_flag;

    struct defined_arg *help_arg = &defined_args[3];
    help_arg->name = "help";
    help_arg->name_len = 4;
    help_arg->short_name = 'h';
    help_arg->flag = &args->help_flag;

    // start parsing

    for (int x = 1; x < argc; x++) {
        size_t argv_len = strnlen(argv[x], 256);
        
        int dash_state = 0;
        int double_dash_state = 0;
        for (int y = 0; y < argv_len; y++) {
            char argv_c = argv[x][y];
            if (y == 0 && argv_c == '-' && argv_len > 1) {
                dash_state = 1;
            } else if (dash_state) {
                if (y == 1 && argv_c == '-') {
                    dash_state = 0;
                    double_dash_state = 1;
                } else {
                    int z = 0;
                    int flag_match = 0;
                    for (z = 0; z < sizeof(defined_args) / sizeof(struct defined_arg); z++) {
                        struct defined_arg arg = defined_args[z];
                        if (arg.short_name == argv_c) {
                            flag_match = 1;
                            *arg.flag = 1;
                            break;
                        }
                    }
                    if (!flag_match) {
                        fprintf(stderr, "[error] unknown flag: %c\n", argv_c);
                        free(args);
                        return NULL;
                    }
                }
            } else if (double_dash_state) {
                if (argv_c == 0)
                    break;
            } else {
                break;
            }
        }
        if (double_dash_state) {
            int flag_match = 0;
            char *arg = argv[x];
            size_t arg_len = strnlen(arg, 256);

            if (arg_len > 2) {
                arg += 2;
                arg_len -= 2;

                for (int z = 0; z < sizeof(defined_args) / sizeof(struct defined_arg); z++) {
                    struct defined_arg defined_arg = defined_args[z];
                    if (defined_arg.name_len == arg_len && strncmp(defined_arg.name, arg, defined_arg.name_len) == 0) {
                        flag_match = 1;
                        *defined_arg.flag = 1;
                        break;
                    }
                }
            }
            if (!flag_match) {
                fprintf(stderr, "[error] unknown flag: %s\n", argv[x]+2);
                free(args);
                return NULL;
            }
        } else if (!dash_state && !double_dash_state) {
            if (args->filename == NULL)
                args->filename = argv[x];
            else {
                fprintf(stderr, "[error] more than one file provided\n");
                free(args);
                return NULL;
            }
        }
    }

    if (args->help_flag) {
        print_help();
        free(args);
        return NULL;
    }

    return args;
}