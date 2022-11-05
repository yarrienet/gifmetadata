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
const char gif_87a[] = { '8', '7', 'a'};
const char gif_89a[] = { '8', '9', 'a'};

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
		fclose(file);
		return GIF_FILE_MISSING_SIG;
	}
	
	// program reads the file in 256 chunks, and then requests
	// more if it needs them
	
	enum file_read_state state = header;
	unsigned char buffer[256];
	
	int color_table_len = 0;
	
	// scratchpad is a buffer used for misc reading/writing
	// while traversing thru the gif. 256 is enough since
	// the size as given by gif blocks can only be represented
	// in an unsigned char.
	unsigned char *scratchpad = malloc(sizeof(unsigned char) * 256);
	// i is max index of written data in the buffer
	int scratchpad_i = 0;
	// len is the max length to write into the buffer according
	// to an unsigned char that prefixes data
	int scratchpad_len = 0;
	
	// local screen descriptor
	enum lsd_state local_lsd_state;
	enum extension_type local_extension_type;

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

			for (i = 0; i < sizeof(gif_sig); i++) {
				if (buffer[i] != gif_sig[i]) {
					fclose(file);
                    free(extension_cb_info);
					return GIF_FILE_INVALID_SIG;
				}
			}
			char unsupported_version = 0;
			for (; i < 6; i++) {
				if (buffer[i] != gif_87a[i] && buffer[i] != gif_89a[i]) {
					unsupported_version = 1;
					break;
				}
			}
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
						int result = scratchpad[0] + (scratchpad[1] << 8);
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
				case packed: {
					int color_resolution = ((buffer[i] & 0b1000000) >> 4) + ((buffer[i] & 0b100000) >> 4) + ((buffer[i] & 0b10000) >> 4);
					color_table_len = 3 * pow(2, color_resolution+1);
					
					if (dev_flag)
						printf("[dev] color depth is: %d, len is: %d\n", color_resolution, color_table_len);
					if ((buffer[i] & 0b10000000) == 0b10000000) {
						// global color table
						if (dev_flag)
							printf("[dev] has a global color table\n");
						
						int color_table_size = (buffer[i] & 0b100) + (buffer[i] & 0b10) + (buffer[i] & 0b1);
						color_table_len = 3 * pow(2, color_table_size+1);
						if (dev_flag)
							printf("[dev] color table size: %d, len: %d\n", color_table_size, color_table_len);
						
						
						// use the scratchpad index as color table
						// index
						scratchpad_i = 0;
						state = global_color_table;
					} else {
						state = searching;
					}
					
					break;
					}
				default:
					break;
				}
				
				break;
			case global_color_table:
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
					// the trailer as a mistake or easter egg:)~~
					state = trailer;
					break;
				default:
					if (dev_flag)
						printf("[dev] unknown byte: (0x%x)...\n", buffer[i]);
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
					if (scratchpad_i < scratchpad_len) {
						scratchpad[scratchpad_i] = buffer[i];
						scratchpad_i++;
					} else {
						// if length has been met...
						if (local_extension_type == application || local_extension_type == application_subblock) {
							scratchpad[scratchpad_i] = '\0';
							scratchpad_i = 0;
							scratchpad_len = buffer[i];
							
							if (scratchpad_len == 0) {
								state = searching;
								break;
							}
						} else {
							if (buffer[i] == 0) {
								// if null terminated,
								// terminate and await next block
								scratchpad[scratchpad_i] = '\0';

								state = searching;
							}
							// else await the null terminator and
							// do not overflow buffer
						}
                        if (extension_cb_info) {
                            extension_cb_info->type = local_extension_type;
                            extension_cb_info->buffer = scratchpad;
                            extension_cb_info->buffer_len = scratchpad_len;
                            extension_cb(extension_cb_info);
                        }
                        // for the next byte
                        if (local_extension_type == application)
                            local_extension_type = application_subblock;
					}
					
				}
				break;
			case image_descriptor:
			
				if (scratchpad_i >= 8) {
					if (dev_flag)
						printf("[dev] reached the end of an image descriptor, now parsing\n");
					if ((buffer[i] & 0b10000000) == 0b10000000) {
						// local color table
						scratchpad_i = 0;
						int local_color_table = (buffer[i] & 0b100) + (buffer[i] & 0b10) + (buffer[i] & 0b1);
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
						if (buffer[i] == 0x0) {
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
		fprintf(stderr, "[warning] file was incompatible and therefore gifpeek may have missed some data, recommended that you view this file in a hex editor to get more information\n");

    return GIF_FILE_SUCCESS;
}