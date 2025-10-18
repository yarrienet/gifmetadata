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

#include "gif.h"

#define CALL_STATE_CB(cb, s) if (cb != NULL) cb(s, s->read_state)

const char gif_sig[] = { 'G', 'I', 'F', '8', 'x', 'a' };

int gifmetadata_parse_gif(
    gifmetadata_state *s,
    unsigned char *chunk,
    size_t chunk_len,
    void (*extension_cb)(gifmetadata_state*, gifmetadata_extension_info*),
    void (*state_cb)(gifmetadata_state*, enum gifmetadata_read_state)) {

    for (int i = 0; i < chunk_len; i++) {
        unsigned char byte = chunk[i];
       
        if (s->read_state == header) {
            // loading header bytes into scratchpad until complete
            s->scratchpad[s->scratchpad_i++] = byte;
            if (s->scratchpad_i > 6) {

                unsigned char sig_byte;
                for (int j = 0; j < 6; j++) {
                    sig_byte = s->scratchpad[j];
                    if (j == 4) {
                        if (sig_byte == 0x37)
                            s->gif_version = gif87a;
                        else if (sig_byte == 0x39)
                            s->gif_version = gif89a;
                        else
                            return GIFMETADATA_INVALID_SIG;
                    } else if (sig_byte != gif_sig[j]) {
                        return GIFMETADATA_INVALID_SIG; 
                    }
                }

                s->scratchpad_i = 0;
                s->read_state = logical_screen_descriptor;
                CALL_STATE_CB(state_cb, s);
                s->local_lsd_state = 0;
            }
        }
        
        // for (; i < bytes_to_read; i++) {
        switch (s->read_state) {
        case logical_screen_descriptor:
            
            switch (s->local_lsd_state) {
            case width:
            case height:
                s->scratchpad[s->scratchpad_i++] = byte;
                if (s->scratchpad_i >= 2) {
                    uint16_t result = s->scratchpad[0] | (s->scratchpad[1] << 8);
                    s->scratchpad_i = 0;

                    if (s->local_lsd_state == width) {
                        s->canvas_width = result;
                        s->local_lsd_state = height;
                    } else {
                        s->canvas_height = result;
                        s->local_lsd_state = packed;
                    }
                }
                break;
            case packed:
                s->global_color_table_flag = byte >> 7 & 1;

                if (s->global_color_table_flag) {
                    s->color_resolution = (byte >> 4) & 0b111;
                    s->color_table_size = byte & 0b111;
                    s->color_table_len = 3 * pow(2, s->color_table_size+1);
                    
                    // use the scratchpad index as color table index
                    s->scratchpad_i = 0;
                    s->read_state = global_color_table;
                    CALL_STATE_CB(state_cb, s);
                } else {
                    s->read_state = searching;
                    CALL_STATE_CB(state_cb, s);
                }
                
                break;
            default:
                break;
            }
            
            break;
        case global_color_table:
            // loop through the global color table, ignoring the contents
            if (s->color_table_len < s->scratchpad_i) {
                s->read_state = searching;
                CALL_STATE_CB(state_cb, s);
            }
            s->scratchpad_i++;
            break;
        case searching:
            switch (byte) {
            case 0x21:
                s->read_state = extension;
                CALL_STATE_CB(state_cb, s);
                break;
            case 0x2c:
                s->read_state = image_descriptor;
                CALL_STATE_CB(state_cb, s);
                s->scratchpad_i = 0;
                s->scratchpad_len = 0;
                break;
            case 0x3b:
                // if this were a real gif parser you would terminate here
                // but i'm speculating that at least one gif
                // has been made with comment data coming after
                // the trailer as a mistake or easter egg
                s->read_state = trailer;
                CALL_STATE_CB(state_cb, s);
                break;
            default:
                // unknown byte
                // again, this has never occured but persists to avoid the potential
                break;
            }
            break;
        case extension:
            s->scratchpad_i = 0;
            s->scratchpad_len = 0;
            s->read_state = known_extension;
            CALL_STATE_CB(state_cb, s);
            switch (byte) {
                case 0x01:
                    s->local_extension_type = plain_text;
                    break;
                case 0xff:
                    s->local_extension_type = application;
                    break;
                case 0xfe:
                    s->local_extension_type = comment;
                    break;
                default:
                    s->scratchpad_i = 0;
                    s->scratchpad_len = 0;
                    s->read_state = unknown_extension;
                    CALL_STATE_CB(state_cb, s);
                    break;
            }
            break;
        case unknown_extension:
            if (s->scratchpad_len == 0) {
                if (byte == 0) {
                    s->read_state = searching;
                    CALL_STATE_CB(state_cb, s);
                    break;
                }
                s->scratchpad_len = byte;
                s->scratchpad_i = 0;
            } else {
                if (byte == 0x0 && s->scratchpad_i >= s->scratchpad_len) {
                    s->read_state = searching;
                    CALL_STATE_CB(state_cb, s);
                } else {
                    s->scratchpad_i++;
                }	
            }
            break;
        case known_extension:
            // if the scratchpatch len is empty
            // then must be new block
            if (s->scratchpad_len == 0) {
                // if the new size of the block is
                // zero then terminate
                if (byte == 0) {
                    s->read_state = searching;
                    CALL_STATE_CB(state_cb, s);
                    break;
                }
                // else get ready for a new block
                s->scratchpad_len = byte;
                s->scratchpad_i = 0;
            } else {
                // comments must be dealt differently and allowed to exceed
                // previously defined lengths because applications
                // sometimes disregard their given size and overload
                // strings beyond 255 bytes
                //
                // if bytes to read remaining or is a comment
                int is_comment = s->local_extension_type == comment && byte != 0;
                if (s->scratchpad_i < s->scratchpad_len || is_comment) {
                    // if future bytes will exceed scratchpad size, realloc
                    if (s->local_extension_type == comment && s->scratchpad_i + 1 >= s->scratchpad_size) {
                        s->scratchpad_size += SCRATCHPAD_CHUNK_SIZE;
                        if (s->scratchpad_size > SCRATCHPAD_CHUNK_SIZE * 10) {
                            // don't go past 2560 bytes of reallocation
                            //free(extension_cb_info);
                            return GIFMETADATA_COMMENT_EXCEEDS_BOUNDS;
                        }
                        s->scratchpad = realloc(s->scratchpad, s->scratchpad_size);
                    }

                    s->scratchpad[s->scratchpad_i] = byte;
                    s->scratchpad_i++;
                } else {
                    s->scratchpad[s->scratchpad_i] = 0;

                    if (extension_cb != NULL) {
                        gifmetadata_extension_info *extension_cb_info = malloc(sizeof(gifmetadata_extension_info));
                        if (extension_cb_info == NULL) {
                            return GIFMETADATA_ALLOC_FAILED;
                        }
                        extension_cb_info->type = s->local_extension_type;
                        extension_cb_info->buffer = s->scratchpad;
                        // TODO ensure that callback receiver does not
                        // free scratchpad
                        if (s->local_extension_type == comment) {
                            extension_cb_info->buffer_len = s->scratchpad_i;
                        } else {
                            extension_cb_info->buffer_len = s->scratchpad_len;
                        }
                        // TODO ensure that callback receiver frees struct
                        extension_cb(s, extension_cb_info);
                    }

                    // if the next extension type is an application then
                    // optional sub blocks can follow
                    if (s->local_extension_type == application || s->local_extension_type == application_subblock) {
                        s->local_extension_type = application_subblock;
                        s->scratchpad_i = 0;
                        s->scratchpad_len = byte;
                        
                        if (s->scratchpad_len == 0) {
                            s->read_state = searching;
                            CALL_STATE_CB(state_cb, s);
                            break;
                        }
                    } else {
                        s->read_state = searching;
                        CALL_STATE_CB(state_cb, s);
                    }
                }
            }
            break;
        case image_descriptor:
        
            if (s->scratchpad_i >= 8) {
                // local color table check
                if (byte >> 7 == 1) {
                    s->scratchpad_i = 0;
                    int local_color_table = byte & 0b111;
                    s->scratchpad_len = 3*pow(2,local_color_table+1);
                    s->read_state = local_color_table;
                    CALL_STATE_CB(state_cb, s);
                } else {
                    s->scratchpad_i = 0;
                    s->scratchpad_len = 0;
                    s->read_state = image_data;
                    CALL_STATE_CB(state_cb, s);
                    break;
                }
            } else {
                s->scratchpad_i++;
            }
            break;
        case local_color_table:
            // loop through the local color table, ignoring the contents
            if (s->scratchpad_i >= s->scratchpad_len) {
                s->scratchpad_i = 0;
                s->scratchpad_len = 0;
                s->read_state = image_data;
                CALL_STATE_CB(state_cb, s);
            } else {
                s->scratchpad_i++;
            }
            break;
        case image_data:
            // loop through the image data, ignoring the contents
            if (s->scratchpad_len == 0) {
                if (s->scratchpad_i == 1) {
                    s->scratchpad_i = 0;
                    s->scratchpad_len = byte;
                    break;
                }
            } else {
                if (s->scratchpad_i >= s->scratchpad_len) {
                    if (byte == 0) {
                        s->read_state = searching;
                        CALL_STATE_CB(state_cb, s);
                        break;
                    } else {
                        s->scratchpad_i = 0;
                        s->scratchpad_len = byte;
                        break;
                    }
                }
            }
            s->scratchpad_i++;
            break;
        default:
            break;
        }
        // }
        
    }
    //free(extension_cb_info);
    //free(scratchpad);

    // TODO 
    if (s->read_state != trailer)
        return GIFMETADATA_UNEXPECTED_EOF;

    return GIFMETADATA_SUCCESS;
}
