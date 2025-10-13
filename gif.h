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

#ifndef GIFMETADATA_GIF_H
#define GIFMETADATA_GIF_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// temp for debugging
#include "debug.h"

#define SCRATCHPAD_CHUNK_SIZE 256

// extension callbacks

enum extension_type {
    plain_text,
    application,
    application_subblock,
    comment
};
struct extension_info {
    enum extension_type type;
    unsigned char *buffer;
    size_t buffer_len;
};

// state callbacks

enum file_read_state {
    header,
    logical_screen_descriptor,
    global_color_table,
    control_extension,
    image_descriptor,
    local_color_table,
    image_data,
    extension,
    known_extension,
    unknown_extension,
    trailer,
    searching
};

// error
enum read_gif_file_status {
    GIF_FILE_SUCCESS,
    GIF_FILE_INVALID_SIG,
    GIF_FILE_COMMENT_EXCEEDS_BOUNDS
};

enum read_gif_file_status read_gif_file(FILE *file, void (*extension_cb)(struct extension_info*), void (*state_cb)(enum file_read_state),  int verbose_flag, int dev_flag);

#endif
