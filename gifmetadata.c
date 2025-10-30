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

#include "gifmetadata.h"

gifmetadata_state *gifmetadata_state_new() {
    gifmetadata_state *state = malloc(sizeof(gifmetadata_state));
    if (state == NULL)
        return NULL;

    state->file_i = 0;

    // configure the scratchpad

    // TODO include scratchpad size in header file
    state->scratchpad = malloc(SCRATCHPAD_CHUNK_SIZE);
    if (state->scratchpad == NULL) {
        free(state);
        return NULL;
    }
    state->scratchpad_size = SCRATCHPAD_CHUNK_SIZE;
    state->scratchpad_i = 0;
    state->scratchpad_len = 0;

    // initial values
    state->read_state = header;
    state->color_table_size = 0;
    state->color_table_len = 0;
    state->color_resolution = -1;
    state->global_color_table_flag = -1;

    state->canvas_width = -1;
    state->canvas_height = -1;

    return state;
}

void gifmetadata_state_free(gifmetadata_state *state) {
    if (state->scratchpad != NULL)
        free(state->scratchpad);
    free(state);
}

int gifmetadata_parse_gif(
    gifmetadata_state *s,
    unsigned char *chunk,
    size_t chunk_len,
    void (*extension_cb)(gifmetadata_state*, gifmetadata_extension_info*),
    void (*state_cb)(gifmetadata_state*, enum gifmetadata_read_state));

