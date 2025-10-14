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


const char gif_sig[] = { 'G', 'I', 'F' };
const char gif_87a[] = { '8', '7', 'a' };
const char gif_89a[] = { '8', '9', 'a' };

/*
enum read_gif_file_status read_gif_file(FILE *file, void (*extension_cb)(struct extension_info*), void (*state_cb)(enum file_read_state), int verbose_flag, int dev_flag) {
    fseek(file, 0, SEEK_END);
    size_t filelen = ftell(file);
    rewind(file);
    
    // if (verbose_flag)
        // printf("[verbose] opened file '%s'\n", args->filename);
    
    if (verbose_flag)
        printf("[verbose] file size: %ld bytes\n", filelen);

    // step 1: check file is a gif
    if (6 > filelen) {
        return GIFMETADATA_INVALID_SIG;
    }
    
    // program reads the file in 256 chunks, and then requests
    // more if it needs them
    
    enum file_read_state state = header;
    unsigned char buffer[256];
    
    int color_table_size = 0;
    int color_table_len = 0;
    
    // scratchpad is a buffer used for misc reading/writing
    // while traversing thru the gif. 256 is enough since
    // the size as given by gif blocks can only be represented
    // in an unsigned char.
    unsigned char *scratchpad = malloc(sizeof(unsigned char) * SCRATCHPAD_CHUNK_SIZE);
    // allocated size of the buffer
    size_t scratchpad_size = SCRATCHPAD_CHUNK_SIZE;
    // i is max index of written data in the buffer
    int scratchpad_i = 0;
    // len is the expected length to write into the buffer according
    // to an unsigned char that prefixes data in the GIF data, can
    // be inaccurate for comments because some applications overloaded
    // comment data
    int scratchpad_len = 0;
    
    // local screen descriptor
    enum lsd_state local_lsd_state;
    enum extension_type local_extension_type;

    int16_t result;
    int global_color_table_flag;
    int color_resolution;

    int bytes_to_read;

    struct extension_info *extension_cb_info;
    if (extension_cb != NULL)
        extension_cb_info = malloc(sizeof(struct extension_info));

    while ((bytes_to_read = fread(buffer, sizeof(unsigned char), 256, file)) > 0) {
*/

int gifmetadata_parse_gif(
    gifmetadata_state *s,
    unsigned char *chunk,
    size_t chunk_len,
    void (*extension_cb)(gifmetadata_extension_info*)) {

    // TODO remove print code from function
    int verbose_flag = 1;
    int dev_flag = 1;

    for (int i = 0; i < chunk_len; i++) {
        unsigned char byte = chunk[i];
        
        // safe to assume that magic numbers fit in
        // a full 256 chunk bcs its at the start of
        // a file
        if (s->read_state == header) {

            // TODO replace gif magic number check with chunked compatible
            // version

            /*
            const size_t gif_sig_len = sizeof(gif_sig);
            for (i = 0; i < gif_sig_len; i++) {
                if (byte != gif_sig[i]) {
                    free(scratchpad);
                    free(extension_cb_info);
                    return GIFMETADATA_INVALID_SIG;
                }
            }

            int unsupported_version = 0;
            for (i = 0; i < 3; i++) {
                if (chunk[i+gif_sig_len] != gif_87a[i] && chunk[i+gif_sig_len] != gif_89a[i]) {
                    unsupported_version = 1;
                    break;
                }
            }
            // pushing i past the sig (length of 6)
            i = 6;
            
            if (verbo) {
                if (buffer[3] == '8') {
                    if (buffer[4] == '7') {
                        printf("[verbose] gif is version 87a\n");
                    } else if (buffer[4] == '9') {
                        printf("[verbose] gif is version 89a\n");
                    }
                }
            }
            if (unsupported_version) {
                fprintf(stderr, "[warning] gif is an unsupported version: ");
                for (i = sizeof(gif_sig); i < 6; i++) {
                    fprintf(stderr, "%c", byte);
                }
                fprintf(stderr, "\n");
            }
            */
            s->read_state = logical_screen_descriptor;
            s->local_lsd_state = 0;
        }
        
        // for (; i < bytes_to_read; i++) {
        switch (s->read_state) {
        case logical_screen_descriptor:
            
            switch (s->local_lsd_state) {
            case width:
            case height:
                s->scratchpad[s->scratchpad_i] = byte;
                s->scratchpad_i++;
                if (s->scratchpad_i >= 2) {
                    s->result = s->scratchpad[0] | (s->scratchpad[1] << 8);
                    /*
                    if (s->local_lsd_state == width) {
                        if (verbose_flag)
                            printf("[verbose] canvas width: %d\n", s->result);
                    } else {
                        if (verbose_flag)
                            printf("[verbose] canvas height: %d\n", s->result);
                    }
                    */
                    s->scratchpad_i = 0;
                    if (s->local_lsd_state == width)
                        s->local_lsd_state = height;
                    else
                        s->local_lsd_state = packed;
                }
                break;
            case packed:
                s->global_color_table_flag = byte >> 7 & 1;

                if (s->global_color_table_flag) {
                    if (dev_flag)
                        printf("[dev] has a global color table\n");
                    
                    s->color_resolution = (byte >> 4) & 0b111;
                    
                    if (dev_flag)
                        printf("[dev] color resolution: %d\n", s->color_resolution);

                    s->color_table_size = byte & 0b111;
                    s->color_table_len = 3 * pow(2, s->color_table_size+1);
                    if (dev_flag)
                        printf("[dev] color table size: %d, color table len: %d\n",
                            s->color_table_size, s->color_table_len);
                    
                    // use the scratchpad index as color table index
                    s->scratchpad_i = 0;
                    s->read_state = global_color_table;
                } else {
                    if (dev_flag)
                        printf("[dev] does not have a global color table\n");
                    s->read_state = searching;
                }
                
                break;
            default:
                if (dev_flag)
                    printf("[dev][warning] reached the end of the lsd switch\n");
                break;
            }
            
            break;
        case global_color_table:
            // loop through the global color table, ignoring the contents
            if (s->color_table_len < s->scratchpad_i) {
                if (dev_flag)
                    printf("[dev] finished the global color table\n");
                s->read_state = searching;
            }
            s->scratchpad_i++;
            break;
        case searching:
            switch (byte) {
            case 0x21:
                if (dev_flag)
                    printf("[dev] found an extension\n");
                s->read_state = extension;
                break;
            case 0x2c:
                if (dev_flag)
                    printf("[dev] found an image descriptor\n");
                s->read_state = image_descriptor;
                s->scratchpad_i = 0;
                s->scratchpad_len = 0;
                break;
            case 0x3b:
                if (dev_flag)
                    printf("[dev] found the trailer\n");
                // if this were a real gif parser you would terminate here
                // but i'm speculating that at least one gif
                // has been made with comment data coming after
                // the trailer as a mistake or easter egg
                s->read_state = trailer;
                break;
            default:
                if (dev_flag) {
                    printf("[dev][warning] unknown byte: (0x%x)...\n", byte);
                    // void printf_offset(char *str, FILE file, size_t file_buffer_size, int offset);
                    //printf_offset("unknown byte", file, 256, i);
                }
                // again, this has never occured but persists to avoid the potential
                break;
            }
            break;
        case extension:
            s->scratchpad_i = 0;
            s->scratchpad_len = 0;
            s->read_state = known_extension;
            switch (byte) {
                case 0x01:
                    s->local_extension_type = plain_text;
                    if (dev_flag)
                        printf("[dev] found a plain text extension\n");
                    break;
                case 0xff:
                    s->local_extension_type = application;
                    if (dev_flag)
                        printf("[dev] found an application extension\n");
                    break;
                case 0xfe:
                    s->local_extension_type = comment;
                    if (dev_flag)
                        printf("[dev] found a comment extension\n");
                    break;
                default:
                    s->scratchpad_i = 0;
                    s->scratchpad_len = 0;
                    s->read_state = unknown_extension;
                    if (dev_flag)
                        printf("[dev] found an unknown extension\n");
                    break;
            }
            break;
        case unknown_extension:
            if (s->scratchpad_len == 0) {
                if (byte == 0) {
                    s->read_state = searching;
                    break;
                }
                s->scratchpad_len = byte;
                s->scratchpad_i = 0;
            } else {
                if (byte == 0x0 && s->scratchpad_i >= s->scratchpad_len) {
                    if (dev_flag)
                        printf("[dev] reached the end of the unknown extension\n");
                    s->read_state = searching;
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
                    if (dev_flag)
                        printf("[dev] new extension block was empty\n");
                    break;
                }
                // else get ready for a new block
                s->scratchpad_len = byte;
                s->scratchpad_i = 0;
                if (dev_flag)
                    printf("[dev] new extension block len: %d\n", s->scratchpad_len);
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
                        if (dev_flag)
                            printf("[dev] reallocated scratchpad to a size of: %ld (* type sizeof)\n", s->scratchpad_size);
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
                        extension_cb(extension_cb_info);
                    }

                    // if the next extension type is an application then
                    // optional sub blocks can follow
                    if (s->local_extension_type == application || s->local_extension_type == application_subblock) {
                        s->local_extension_type = application_subblock;
                        s->scratchpad_i = 0;
                        s->scratchpad_len = byte;
                        
                        if (s->scratchpad_len == 0) {
                            s->read_state = searching;
                            break;
                        }
                    } else {
                        s->read_state = searching;
                    }
                }
            }
            break;
        case image_descriptor:
        
            if (s->scratchpad_i >= 8) {
                if (dev_flag)
                    printf("[dev] reached the end of an image descriptor, now parsing\n");
                // local color table check
                if (byte >> 7 == 1) {
                    s->scratchpad_i = 0;
                    int local_color_table = byte & 0b111;
                    s->scratchpad_len = 3*pow(2,local_color_table+1);
                    s->read_state = local_color_table;
                    if (dev_flag)
                        printf("[dev] image descriptor contains a local color table with length %d\n", s->scratchpad_len);
                } else {
                    s->scratchpad_i = 0;
                    s->scratchpad_len = 0;
                    s->read_state = image_data;
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
                if (dev_flag)
                    printf("[dev] reached the end of the local color table\n");
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
                    if (dev_flag)
                        printf("[dev] start of image data blocks, initial block size: %d\n", s->scratchpad_len);
                    break;
                }
            } else {
                if (s->scratchpad_i >= s->scratchpad_len) {
                    if (byte == 0) {
                        if (dev_flag)
                            printf("[dev] reached the end of image data blocks\n");
                        s->read_state = searching;
                        break;
                    } else {
                        s->scratchpad_i = 0;
                        s->scratchpad_len = byte;
                        if (dev_flag)
                            printf("[dev] read an image block, next block size: %d\n", s->scratchpad_len);
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
