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
#include "gif.h"

int all_flag = 0;

void extension_callback(struct extension_info *extension) {
    if (all_flag) {
        switch (extension->type) {
        case plain_text:
            printf("plain text: %s\n", extension->buffer);
            break;
        case application:
            printf("application: %s (%ld bytes)\n", extension->buffer, extension->buffer_len);
            break;
        case application_subblock:
            printf("application sub-block (%ld bytes)\n", extension->buffer_len);
            break;
        case comment:
            printf("comment: %s (%ld bytes)\n", extension->buffer, extension->buffer_len);
        }
    } else {
        if (extension->type == comment) {
            printf("%s\n", extension->buffer);
        }
    }
}

int main(int argc, char **argv) {

    cli_user_args *args = cli_parse(argc, argv);
    // returns null when handled e.g. help
    if (args == NULL)
        return 0;
    
    all_flag = args->all_flag;

    if (args->dev_flag) {
        printf("[dev] dev flag active\n");
        if (args->verbose_flag) {
            printf("[dev] verbose flag active\n");
        }
    }
    
    if (args->filename == NULL) {
        fprintf(stderr, "[error] you never specified a file to open\n");
        cli_free_user_args(args);
        return 1;
    }

    FILE *fileptr;
    
    if (access(args->filename, F_OK) != 0) {
        fprintf(stderr, "[error] file '%s' cannot be accessed\n", args->filename);
        cli_free_user_args(args);
        return 1;
    }
    
    fileptr = fopen(args->filename, "rb");

    enum read_gif_file_status gif_status = read_gif_file(fileptr, &extension_callback, NULL, args->verbose_flag, args->dev_flag);
    if (gif_status > 0) {
        switch (gif_status) {
        case GIF_FILE_INVALID_SIG:
            fprintf(stderr, "[error] file is an unsupported gif version\n");
            break;
        case GIF_FILE_COMMENT_EXCEEDS_BOUNDS:
            fprintf(stderr, "[error] file contains an invalid comment\n");
            break;
        default:
            fprintf(stderr, "[error] unhandled gif state\n");
            break;
        }
    } else {
        if (args->dev_flag)
            printf("[dev] finished reading image\n");
    }
    
    fclose(fileptr);
    cli_free_user_args(args);

    return gif_status;
}

