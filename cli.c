#include "cli.h"

// custom strncpy implementation to avoid
// including string.h, be warned the
// less than/greater than functionality
// isn't implemented
char strncpy_(const char *s1, const char *s2, size_t n) {
	for (int x = 0; x < n; x++) {
		if (s1[x] != s2[x])
			return 1;
	}
	return 0;
}

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

const char arg_all[] = "all";
const char arg_verbose[] = "verbose";
const char arg_dev[] = "dev";
const char arg_help[] = "help";

struct cli_args *parse_args(int argc, char **argv) {
    // reading argv code
	//
	// cool note...apparently int is faster than
	// char even tho its a bigger size bcs
	// cpus are designed to work with ints
	// https://stackoverflow.com/questions/9521140/char-or-int-for-boolean-value-in-c
	
    struct cli_args *args = malloc(sizeof(struct cli_args));
    args->all_flag = 0;
    args->verbose_flag = 0;
    args->dev_flag = 0;
    args->filename = NULL;

	for (int x = 1; x < argc; x++) {
		int y = 0;
		if (argv[x][0] == '-') {
			if (argv[x][1] == '-') { 
				// calculate the length of the
				// long arg
				y = 2;
				while (argv[x][y] != 0)
					y++;
				int arg_len = y - 2 + 1;
				
				if (arg_len < 1)
					break;
				
				// copy the long arg into a buffer
				char *long_arg = malloc(sizeof(char) * arg_len);
				for (y = 2; y < arg_len + 2; y++)
					long_arg[y - 2] = argv[x][y];
				
				// check if buffer matches the known
				// buffers
				
				// dirty self strncmp... if i add more
				// args ill just make a function
				if (arg_len == sizeof(arg_all)) {
					if (strncpy_(long_arg, (const char*)&arg_all, arg_len) == 0) {
						args->all_flag = 1;
						free(long_arg);
						continue;
					}
				} else if (arg_len == sizeof(arg_verbose)) {
					if (strncpy_(long_arg, (const char*)&arg_verbose, arg_len) == 0) {
						args->verbose_flag = 1;
						free(long_arg);
						continue;
					}
				} else if (arg_len == sizeof(arg_dev)) {
					if (strncpy_(long_arg, (const char*)&arg_dev, arg_len) == 0) {
						args->dev_flag = 1;
						free(long_arg);
						continue;
					}
				} else if (arg_len == sizeof(arg_help)) {
					if (strncpy_(long_arg, (const char*)&arg_help, arg_len) == 0) {
						free(long_arg);
						if (args->filename != NULL)
							free(args->filename);
						print_help();
						return NULL;
					}
				}
				
				fprintf(stderr, "[error] unknown flag: %s\n", long_arg);
				if (args->filename != NULL)
					free(args->filename);
				free(long_arg);
				return NULL;
			} else {
				y = 1;
				while (argv[x][y] != '\0') {
					switch (argv[x][y]) {
					case 'a':
						args->all_flag = 1;
						break;
					case 'v':
						args->verbose_flag = 1;
						break;
					case 'd':
						args->dev_flag = 1;
						break;
					case 'h':
						if (args->filename != NULL)
							free(args->filename);
						print_help();
						return NULL;
					default:
						fprintf(stderr, "[error] unknown flag: %c\n", argv[x][y]);
						if (args->filename != NULL)
							free(args->filename);
						return NULL;
					}
					y++;
				}
			}
		} else {
			if (args->filename != NULL) {
				fprintf(stderr, "[error] specified more than one file, i can only read one\n");
				free(args->filename);
				return NULL;
			}
			// calculate filename len
			y = 0;
			while (argv[x][y] != 0)
				y++;
			// allocate filename buffer
			int filename_len = y + 1;
			args->filename = malloc(sizeof(char) * filename_len);
			// copy filename buffer
			for (y = 0; y < filename_len; y++)
				args->filename[y] = argv[x][y];
		}
	}
    return args;
}