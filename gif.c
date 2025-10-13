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

// lsd = local screen descriptor
enum lsd_state {
    width,
    height,
    packed,
    bg_color,
    pixel_aspect_ratio
};

const char gif_sig[] = { 'G', 'I', 'F' };
const char gif_87a[] = { '8', '7', 'a' };
const char gif_89a[] = { '8', '9', 'a' };

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
        return GIF_FILE_INVALID_SIG;
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
        int i = 0;
        
        // safe to assume that magic numbers fit in
        // a full 256 chunk bcs its at the start of
        // a file
        if (state == header) {

            const size_t gif_sig_len = sizeof(gif_sig);
            for (i = 0; i < gif_sig_len; i++) {
                if (buffer[i] != gif_sig[i]) {
                    free(scratchpad);
                    free(extension_cb_info);
                    return GIF_FILE_INVALID_SIG;
                }
            }

            int unsupported_version = 0;
            for (i = 0; i < 3; i++) {
                if (buffer[i+gif_sig_len] != gif_87a[i] && buffer[i+gif_sig_len] != gif_89a[i]) {
                    unsupported_version = 1;
                    break;
                }
            }
            // pushing i past the sig (length of 6)
            i = 6;
            
            if (verbose_flag) {
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
                    fprintf(stderr, "%c", buffer[i]);
                }
                fprintf(stderr, "\n");
            }
            state = logical_screen_descriptor;
            local_lsd_state = 0;
        }
        
        for (; i < bytes_to_read; i++) {
            switch (state) {
            case logical_screen_descriptor:
                
                switch (local_lsd_state) {
                case width:
                case height:
                    scratchpad[scratchpad_i] = buffer[i];
                    scratchpad_i++;
                    if (scratchpad_i >= 2) {
                        result = scratchpad[0] | (scratchpad[1] << 8);
                        if (local_lsd_state == width) {
                            if (verbose_flag)
                                printf("[verbose] canvas width: %d\n", result);
                            scratchpad_i = 0;
                            local_lsd_state = height;
                        } else {
                            if (verbose_flag)
                                printf("[verbose] canvas height: %d\n", result);
                            scratchpad_i = 0;
                            local_lsd_state = packed;
                        }
                    }
                    break;
                case packed:
                    if (dev_flag)
                        printf("[dev] packed byte: 0x%x\n", buffer[i]); 
                    global_color_table_flag = buffer[i] >> 7 & 1;

                    if (global_color_table_flag) {
                        if (dev_flag)
                            printf("[dev] has a global color table\n");
                        
                        color_resolution = (buffer[i] >> 4) & 0b111;
                        
                        if (dev_flag)
                            printf("[dev] color resolution: %d\n", color_resolution);

                        color_table_size = buffer[i] & 0b111;
                        color_table_len = 3 * pow(2, color_table_size+1);
                        if (dev_flag)
                            printf("[dev] color table size: %d, color table len: %d\n",
                                color_table_size, color_table_len);
                        
                        // use the scratchpad index as color table index
                        scratchpad_i = 0;
                        state = global_color_table;
                    } else {
                        if (dev_flag)
                            printf("[dev] does not have a global color table\n");
                        state = searching;
                    }
                    
                    break;
                default:
                    break;
                }
                
                break;
            case global_color_table:
                // loop through the global color table, ignoring the contents
                if (color_table_len < scratchpad_i) {
                    if (dev_flag)
                        printf("[dev] finished the glboal color table\n");
                    state = searching;
                }
                scratchpad_i++;
                break;
            case searching:
                switch (buffer[i]) {
                case 0x21:
                    if (dev_flag)
                        printf("[dev] found an extension\n");
                    state = extension;
                    break;
                case 0x2c:
                    if (dev_flag)
                        printf("[dev] found an image descriptor\n");
                    state = image_descriptor;
                    scratchpad_i = 0;
                    scratchpad_len = 0;
                    break;
                case 0x3b:
                    if (dev_flag)
                        printf("[dev] found the trailer\n");
                    // if this were a real gif parser you would terminate here
                    // but i'm speculating that at least one gif
                    // has been made with comment data coming after
                    // the trailer as a mistake or easter egg
                    state = trailer;
                    break;
                default:
                    if (dev_flag) {
                        printf("[dev] unknown byte: (0x%x)...\n", buffer[i]);
                        // void printf_offset(char *str, FILE file, size_t file_buffer_size, int offset);
                        printf_offset("unknown byte", file, 256, i);
                    }
                    break;
                }
                break;
            case extension:
                scratchpad_i = 0;
                scratchpad_len = 0;
                switch (buffer[i]) {
                    case 0x01:
                        state = known_extension;
                        local_extension_type = plain_text;
                        if (dev_flag)
                            printf("[dev] found a plain text extension\n");
                        break;
                    case 0xff:
                        state = known_extension;
                        local_extension_type = application;
                        if (dev_flag)
                            printf("[dev] found an application extension\n");
                        break;
                    case 0xfe:
                        state = known_extension;
                        local_extension_type = comment;
                        if (dev_flag)
                            printf("[dev] found a comment extension\n");
                        break;
                    default:
                        scratchpad_i = 0;
                        scratchpad_len = 0;
                        state = unknown_extension;
                        if (dev_flag)
                            printf("[dev] found an unknown extension\n");
                        break;
                }
                break;
            case unknown_extension:
                if (scratchpad_len == 0) {
                    if (buffer[i] == 0) {
                        state = searching;
                        break;
                    }
                    scratchpad_len = buffer[i];
                    scratchpad_i = 0;
                } else {
                    if (buffer[i] == 0x0 && scratchpad_i >= scratchpad_len) {
                        if (dev_flag)
                            printf("[dev] reached the end of the unknown extension\n");
                        state = searching;
                    } else {
                        scratchpad_i++;
                    }	
                }
                break;
            case known_extension:
                // if the scratchpatch len is empty
                // then must be new block
                if (scratchpad_len == 0) {
                    // if the new size of the block is
                    // zero then terminate
                    if (buffer[i] == 0) {
                        state = searching;
                        if (dev_flag)
                            printf("[dev] new extension block was empty\n");
                        break;
                    }
                    // else get ready for a new block
                    scratchpad_len = buffer[i];
                    scratchpad_i = 0;
                    if (dev_flag)
                        printf("[dev] new extension block len: %d\n", scratchpad_len);
                } else {
                    if (scratchpad_i < scratchpad_len || (local_extension_type == comment && buffer[i] != 0)) {
                        // comments must be dealt differently and allowed to exceed previously defined lengths because applications
                        // sometimes disregard their given size and overload strings beyond 255 bytes
                        if (local_extension_type == comment && scratchpad_i + 1 >= scratchpad_size) {
                            scratchpad_size += SCRATCHPAD_CHUNK_SIZE;
                            if (scratchpad_size > SCRATCHPAD_CHUNK_SIZE * 10) {
                                // don't go past 2560 bytes of reallocation
                                free(extension_cb_info);
                                free(scratchpad);
                                return GIF_FILE_COMMENT_EXCEEDS_BOUNDS;
                            }
                            if (dev_flag)
                                printf("[dev] reallocated scratchpad to a size of: %ld (* type sizeof)\n", scratchpad_size);
                            scratchpad = realloc(scratchpad, sizeof(unsigned char) * scratchpad_size);
                        }

                        scratchpad[scratchpad_i] = buffer[i];
                        scratchpad_i++;
                    } else {
                        scratchpad[scratchpad_i] = '\0';
                        
                        if (extension_cb_info != NULL && extension_cb != NULL) {
                            extension_cb_info->type = local_extension_type;
                            extension_cb_info->buffer = scratchpad;
                            if (local_extension_type == comment) {
                                extension_cb_info->buffer_len = scratchpad_i;
                            } else {
                                extension_cb_info->buffer_len = scratchpad_len;
                            }
                            extension_cb(extension_cb_info);
                        }

                        // if the next extension type is an application then
                        // optional sub blocks can follow
                        if (local_extension_type == application || local_extension_type == application_subblock) {
                            local_extension_type = application_subblock;
                            scratchpad_i = 0;
                            scratchpad_len = buffer[i];
                            
                            if (scratchpad_len == 0) {
                                state = searching;
                                break;
                            }
                        } else {
                            state = searching;
                        }
                    }
                }
                break;
            case image_descriptor:
            
                if (scratchpad_i >= 8) {
                    if (dev_flag)
                        printf("[dev] reached the end of an image descriptor, now parsing\n");
                    // local color table check
                    if (buffer[i] >> 7 == 1) {
                        scratchpad_i = 0;
                        int local_color_table = buffer[i] & 0b111;
                        scratchpad_len = 3*pow(2,local_color_table+1);
                        state = local_color_table;
                        if (dev_flag)
                            printf("[dev] image descriptor contains a local color table with length %d\n", scratchpad_len);
                    } else {
                        scratchpad_i = 0;
                        scratchpad_len = 0;
                        state = image_data;
                        break;
                    }
                } else {
                    scratchpad_i++;
                }
                break;
            case local_color_table:
                // loop through the local color table, ignoring the contents
                if (scratchpad_i >= scratchpad_len) {
                    scratchpad_i = 0;
                    scratchpad_len = 0;
                    state = image_data;
                    if (dev_flag)
                        printf("[dev] reached the end of the local color table\n");
                } else {
                    scratchpad_i++;
                }
                break;
            case image_data:
                // loop through the image data, ignoring the contents
                if (scratchpad_len == 0) {
                    if (scratchpad_i == 1) {
                        scratchpad_i = 0;
                        scratchpad_len = buffer[i];
                        if (dev_flag)
                            printf("[dev] start of image data blocks, initial block size: %d\n", scratchpad_len);
                        break;
                    }
                } else {
                    if (scratchpad_i >= scratchpad_len) {
                        if (buffer[i] == 0) {
                            if (dev_flag)
                                printf("[dev] reached the end of image data blocks\n");
                            state = searching;
                            break;
                        } else {
                            scratchpad_i = 0;
                            scratchpad_len = buffer[i];
                            if (dev_flag)
                                printf("[dev] read an image block, next block size: %d\n", scratchpad_len);
                            break;
                        }
                    }
                }
                scratchpad_i++;
                break;
            default:
                break;
            }
        }
        
    }
    free(extension_cb_info);
    free(scratchpad);

        
    if (state != trailer)
        fprintf(stderr, "[warning] file was incompatible and therefore some data may have been missed, view this file in a hex editor to get more information\n");

    return GIF_FILE_SUCCESS;
}
