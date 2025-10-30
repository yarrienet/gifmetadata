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

#ifndef GIFMETADATA_H
#define GIFMETADATA_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// error
#define GIFMETADATA_SUCCESS 0 
#define GIFMETADATA_INVALID_SIG -1
#define GIFMETADATA_COMMENT_EXCEEDS_BOUNDS -2
#define GIFMETADATA_ALLOC_FAILED -3
#define GIFMETADATA_UNEXPECTED_EOF -4

#define SCRATCHPAD_CHUNK_SIZE 256

enum gifmetadata_gif_version {
    gif87a,
    gif89a
};

// TODO prefix all, private some

// lsd = local screen descriptor
enum lsd_state {
    width,
    height,
    packed,
    bg_color,
    pixel_aspect_ratio
};

// extension callbacks

enum extension_type {
    plain_text,
    application,
    application_subblock,
    comment
};

// state callbacks

enum gifmetadata_read_state {
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
    searching,
    eof
};

typedef struct gifmetadata_state {
    enum gifmetadata_read_state read_state;

    // TODO this is pulled from the initial representation however this buffer
    // will now be provided at parse function call
    unsigned char buffer[256];

    // file_i
    int file_i;

    int color_table_size;
    int color_table_len;

    // TODO init function
    // scratchpad is a buffer used for reading/writing while traversing through
    // the file. must be initalized before struct use, can be realloc'd later.
    unsigned char *scratchpad;
    // allocated size of the buffer
    size_t scratchpad_size;
    // i is max index of written data in the buffer
    int scratchpad_i;
    // len is the expected length to write into the buffer according
    // to an unsigned char that prefixes data in the GIF data, can
    // be inaccurate for comments because some applications overloaded
    // comment data
    int scratchpad_len;

    // local screen descriptor
    enum lsd_state local_lsd_state;
    enum extension_type local_extension_type;

    int global_color_table_flag;
    int color_resolution;

    // info
    uint16_t canvas_width;
    uint16_t canvas_height;

    enum gifmetadata_gif_version gif_version;
} gifmetadata_state;

typedef struct gifmetadata_extension_info {
    enum extension_type type;
    unsigned char *buffer;
    size_t buffer_len;
} gifmetadata_extension_info;

// Implementation can be found in gif.c
int gifmetadata_parse_gif(
    gifmetadata_state *s,
    unsigned char *chunk,
    size_t chunk_len,
    void (*extension_cb)(gifmetadata_state*, gifmetadata_extension_info*),
    void (*state_cb)(gifmetadata_state*, enum gifmetadata_read_state));

// TODO 'int bytes_to_read' is used as the function that the loop depends
// on for byte chunks. this will be moved to the function arguments

// struct extension_info *extension_cb_info;
// TODO callback will be moved to the function arguments.

gifmetadata_state *gifmetadata_state_new();
void gifmetadata_state_free(gifmetadata_state *state);

#endif
