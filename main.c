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

int all_flag = 0;
int verbose_flag = 0;
int debug_flag = 0;

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
    } else {
        if (extension->type == comment) {
            printf("%s\n", extension->buffer);
        }
    }
    free(extension);
}

void state_cb(gifmetadata_state *s, enum gifmetadata_read_state state) {
    if (debug_flag) {
        fprintf(stderr, "DEBUG State change: %d\n", state);
    }
}

int main(int argc, char **argv) {

    cli_user_args *args = cli_parse(argc, argv);
    // returns null when handled e.g. help
    if (args == NULL)
        return 0;
    
    all_flag = args->all_flag;
    verbose_flag = args->verbose_flag;
    debug_flag = args->debug_flag;
    
    if (args->filename == NULL) {
        fprintf(stderr, "ERROR No file specified\n");
        cli_free_user_args(args);
        return EXIT_IO_ERROR;
    }

    if (access(args->filename, F_OK) != 0) {
        fprintf(stderr, "ERROR File '%s' cannot be accessed\n", args->filename);
        cli_free_user_args(args);
        return EXIT_IO_ERROR;
    }
    
    FILE *f;
    f = fopen(args->filename, "rb");
    if (!f) {
        fprintf(stderr, "ERROR Failed to open file '%s'\n", args->filename);
        cli_free_user_args(args);
        return EXIT_IO_ERROR;
    }
    cli_free_user_args(args);

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "ERROR Failed to seek file '%s'\n", args->filename);
        fclose(f);
        return EXIT_IO_ERROR;
    }

    size_t size = ftell(f);
    if (size < 0) {
        fprintf(stderr, "ERROR Failed to read file '%s'\n", args->filename);
        fclose(f);
        return EXIT_IO_ERROR;
    }

    if (verbose_flag)
        fprintf(stderr, "VERBOSE File size: %ld\n", size);

    rewind(f);

    unsigned char *buf = malloc(size);
    if (!buf) {
        fprintf(stderr, "ERROR Failed to allocate file buffer\n");
        fclose(f);
        return EXIT_IO_ERROR;
    }

    fread(buf, 1, size, f);
    fclose(f);

    gifmetadata_state *gifmetadata_s = gifmetadata_state_new();
    if (gifmetadata_s == NULL) {
        fprintf(stderr, "ERROR Failed to allocate state memory\n");
        return EXIT_MEM_ERROR;
    }
    int status = gifmetadata_parse_gif(gifmetadata_s, buf, size, &extension_cb, &state_cb);

    switch (status) {
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
    case GIFMETADATA_UNEXPECTED_EOF:
        // non-fatal status code
        fprintf(stderr, "WARNING Unexpected end of file\n");
        break;
    default:
        fprintf(stderr, "ERROR Unknown error\n");
        return 1;
    }

    if (verbose_flag) {
        fprintf(stderr, "VERBOSE GIF version: ");
        switch (gifmetadata_s->gif_version) {
        case gif87a:
            printf("87a\n");
            break;
        case gif89a:
            printf("89a\n");
            break;
        default:
            printf("unknown (%d)\n", gifmetadata_s->gif_version);
        }

        fprintf(stderr, "VERBOSE Canvas width: %d\n", gifmetadata_s->canvas_width);
        fprintf(stderr, "VERBOSE Canvas height: %d\n", gifmetadata_s->canvas_height);
    }

    return 0;
}

