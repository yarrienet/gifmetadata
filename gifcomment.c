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

// understanding of how gifs work comes from
//     http://giflib.sourceforge.net/whatsinagif/bits_and_bytes.html
// and
//     https://www.w3.org/Graphics/GIF/spec-gif89a.txt

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "cli.h"
#include "gifmetadata.h"

#define EXIT_IO_ERROR 2
#define EXIT_MEM_ERROR 3
#define EXIT_PARSE_ERROR 4

#define CHUNK_SIZE 2048

const unsigned char comment_extension[] = { 0x21, 0xfe };

int all_flag = 0;
int verbose_flag = 0;

int output_comments = 1;

// file buffer
unsigned char *buf;

// have written comments to output
int w_comments = 0;
int w_chunk_i = 0;
FILE *w_out = NULL;
cli_flag_arg *comment_flags = NULL;

void extension_cb(gifmetadata_state *s, gifmetadata_extension_info *extension) {
    if (extension == NULL)
        return;
    if (all_flag) {
        switch (extension->type) {
        case plain_text:
            printf("Plain text: %s\n", extension->buffer);
            break;
        case application:
            printf("Application: %s (%ld bytes)\n", extension->buffer, extension->buffer_len);
            break;
        case application_subblock:
            printf("Application sub-block (%ld bytes)\n", extension->buffer_len);
            break;
        case comment:
            printf("Comment: %s (%ld bytes)\n", extension->buffer, extension->buffer_len);
        }
    } else if (output_comments) {
        if (extension->type == comment) {
            printf("%s\n", extension->buffer);
        }
    }
    free(extension);
}

void state_cb(gifmetadata_state *s, enum gifmetadata_read_state state) {
    // state is called on the exact byte of first encounter

    int write_comment = w_out != NULL && comment_flags != NULL && state != searching && state > global_color_table && !w_comments;
    if (!write_comment) {
        return;
    }

    if (s->chunk_i > 0) {
        // write to before the current byte
        // no need to minus one as index starts at zero
        fwrite(s->chunk, 1, s->chunk_i, w_out);
        w_chunk_i = s->chunk_i;
    }

    cli_flag_arg *comment = comment_flags;
    while (comment != NULL) {
        fwrite(&comment_extension, 1, sizeof(comment_extension), w_out);
        unsigned char len;
        if (comment->string_len > 255) {
            fprintf(stderr, "WARNING Comment length is longer than 255 characters, this is may cause incompatibility issues\n");
            len = 255;
        } else {
            len = (unsigned char)comment->string_len;
        }
        fwrite(&len, 1, 1, w_out);
        fwrite(comment->string, 1, comment->string_len, w_out);
        fputc(0, w_out);

        comment = comment->next;
    }
    w_comments = 1; 
}

// TODO gif comment scrubbing
int main(int argc, char **argv) {
    cli_user_args *args = cli_new_user_args();
    if (args == NULL) {
        fprintf(stderr, "ERROR Failed to allocate user arguments\n");
        return EXIT_MEM_ERROR;
    }

    int cli_status = cli_parse(args, argc, argv);
    char invalid_flag = args->invalid_flag;
    if (cli_status < 0)
        cli_free_user_args(args);
    switch (cli_status) {
    case CLI_SUCCESS:
        break;
    case CLI_INVALID_FLAG:
        fprintf(stderr, "ERROR Invalid flag '%c'\n", invalid_flag);
        return EXIT_PARSE_ERROR;
    case CLI_ALLOC_FAILURE:
        fprintf(stderr, "ERROR Memory alloc failure\n");
        return EXIT_MEM_ERROR;
    case CLI_EXCEEDS_ARG_MAX_LEN:
        fprintf(stderr, "ERROR Argument exceeds the maximum length of %d characters\n", CLI_ARG_MAX_LEN);
        return EXIT_PARSE_ERROR;
    case CLI_MULTIPLE_INPUTS:
        fprintf(stderr, "ERROR More than one input file provided\n");
        return EXIT_PARSE_ERROR;
    case CLI_MULTIPLE_OUTPUTS:
        fprintf(stderr, "ERROR More than one output file provided\n");
        return EXIT_PARSE_ERROR;
    case CLI_MISSING_FLAG_ARG:
        fprintf(stderr, "ERROR Flag '%c' is missing an argument\n", invalid_flag);
        return EXIT_PARSE_ERROR;
    default:
        fprintf(stderr, "ERROR Unknown error\n");
        return EXIT_PARSE_ERROR;
    }

    if (args->help_flag) {
        printf("gifcomment [-h] [-a] [-v] [-d] [-c <comment>] [-o <output>] [input]\n");
        cli_free_user_args(args);
        return 0;
    }

    verbose_flag = args->verbose_flag;
    all_flag = args->all_flag;

    // configuring the file for reading
    if (args->output_flag != NULL && args->output_flag->string != NULL) {
        w_out = fopen(args->output_flag->string, "wb");
        if (w_out == NULL) {
            fprintf(stderr, "ERROR Failed to open output file for writing\n");
            return EXIT_IO_ERROR;
        }
        output_comments = 0;
    }

    if (args->comment_flags != NULL) {
        if (w_out == NULL) {
            w_out = stdout;
        }
        output_comments = 0;
        comment_flags = args->comment_flags;
    }

    FILE *f;
    if (args->filename == NULL) {
        f = stdin;
    } else {
        if (access(args->filename, F_OK) != 0) {
            fprintf(stderr, "ERROR File '%s' cannot be accessed\n", args->filename);
            cli_free_user_args(args);
            return EXIT_IO_ERROR;
        }
        
        f = fopen(args->filename, "rb");
        if (f == NULL) {
            fprintf(stderr, "ERROR Failed to open file '%s'\n", args->filename);
            cli_free_user_args(args);
            return EXIT_IO_ERROR;
        }
    }

    // TODO cli_free_user_args used to be here, must be called again on all
    // exit lines below

    // configure gif parsing state
    gifmetadata_state *gifmetadata_s = gifmetadata_state_new();
    if (gifmetadata_s == NULL) {
        fprintf(stderr, "ERROR Failed to allocate state memory\n");
        return EXIT_MEM_ERROR;
    }

    // read file chunk by chunk
    unsigned char *buf = malloc(CHUNK_SIZE);
    if (buf == NULL) {
        fprintf(stderr, "ERROR Buffer memory alloc failure\n");
        return EXIT_MEM_ERROR;
    }

    size_t total_b;
    size_t b;
    int parse_status;
    while ((b = fread(buf, 1, CHUNK_SIZE, f)) != 0) {
        w_chunk_i = 0;
        parse_status = gifmetadata_parse_gif(gifmetadata_s, buf, b, &extension_cb, &state_cb);
        switch (parse_status) {
        case GIFMETADATA_SUCCESS:
            break;
        case GIFMETADATA_INVALID_SIG:
            fprintf(stderr, "ERROR Unsupported GIF version (invalid signature)\n");
            return EXIT_PARSE_ERROR;
        case GIFMETADATA_COMMENT_EXCEEDS_BOUNDS:
            fprintf(stderr, "ERROR Comment exceeds maximum comment length\n");
            return EXIT_PARSE_ERROR;
        case GIFMETADATA_ALLOC_FAILED:
            fprintf(stderr, "ERROR Failed to allocate memory\n");
            return EXIT_MEM_ERROR;
        default:
            fprintf(stderr, "ERROR Unknown error\n");
            return 1;
        }
        total_b += b;

        // before the next loop, write remaining
        if (w_out != NULL) {
            fwrite(buf+w_chunk_i, 1, b-w_chunk_i, w_out);
        }
    }

    if (ferror(f) != 0) {
        fclose(f);
        fprintf(stderr, "ERROR Error reading input file\n"); 
        return EXIT_IO_ERROR;
    }

    fclose(f);

    if (total_b == 0) {
        fprintf(stderr, "ERROR Empty file\n");
        return EXIT_IO_ERROR;
    }
    if (gifmetadata_s->gif_version == 0) {
        fprintf(stderr, "ERROR Invalid GIF file (missing signature)\n");
        return EXIT_IO_ERROR;
    }
    // check for unexpected eof
    if (gifmetadata_s->read_state != trailer) {
        // non-fatal status code
        fprintf(stderr, "WARNING Unexpected end of file\n");
    }

    if (verbose_flag) {
        fprintf(stderr, "VERBOSE GIF version: ");
        switch (gifmetadata_s->gif_version) {
        case gif87a:
            fprintf(stderr, "87a\n");
            break;
        case gif89a:
            fprintf(stderr, "89a\n");
            break;
        default:
            fprintf(stderr, "unknown (%d)\n", gifmetadata_s->gif_version);
        }

        fprintf(stderr, "VERBOSE File size: %ld bytes\n", total_b);
        fprintf(stderr, "VERBOSE Canvas width: %d\n", gifmetadata_s->canvas_width);
        fprintf(stderr, "VERBOSE Canvas height: %d\n", gifmetadata_s->canvas_height);
    }

    cli_free_user_args(args);
    return 0;
}

