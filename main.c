// MIT License
// 
// Copyright (c) 2022 Harry Stanton
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// gifmetadata
// version 0.0.1
//
// Harry Stanton <harry@harrystanton.com>
// https://github.com/harrego/gifmetadata
//
// designed to
//     1. be fast
//     2. be grep able
//     3. use little memory
//     4. be conservative with libraries
//
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

void extension_callback(struct extension_info *extension) {
	printf("extension callback recv a block: ");
	switch (extension->type) {
		case plain_text:
			printf("- plain text");
			break;
		case application:
			printf("- application");
			break;
		case application_subblock:
			printf("- application_sublock");
			break;
		case comment:
			printf("- comment");
			if (extension->buffer != NULL)
				printf(": %s", extension->buffer);
			break;
		default:
			printf("- ??????? idk");
	}
	printf("\n");
}

int main(int argc, char **argv) {

	struct cli_args *args = parse_args(argc, argv);
	
	if (args->dev_flag) {
		printf("[dev] dev flag active\n");
		if (args->verbose_flag) {
			printf("[dev] verbose flag active\n");
		}
	}
	
	if (args->filename == NULL) {
		fprintf(stderr, "[error] you never specified a file to open\n");
		return 1;
	}

	FILE *fileptr;
	long filelen;
	
	if (access(args->filename, F_OK) != 0) {
		fprintf(stderr, "[error] file '%s' cannot be accessed\n", args->filename);
		free(args->filename);
		return 1;
	}
	
	fileptr = fopen(args->filename, "rb");
	
	read_gif_file(fileptr, &extension_callback, NULL, args->verbose_flag, args->dev_flag);
	
	if (args->dev_flag)
		printf("[dev] finished reading image\n");
		
	// TODO: put this back into gif.c
	// if (state != trailer)
		// fprintf(stderr, "[warning] file was incompatible and therefore gifmetadata may have missed some data, recommended that you view this file in a hex editor to get more information\n");
	
	fclose(fileptr);
	if (args->filename != NULL)
		free(args->filename);
	free(args);

	return 0;
}