/*
 * mcJSON, a modified version of cJSON, a simple JSON parser and generator.
 *
 * ISC License
 *
 * Copyright (C) 2015-2016 Max Bruckner (FSMaxB) <max at maxbruckner dot de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  This file incorporates work covered by the following license notice:
 *
 * |  Copyright (c) 2009 Dave Gamble
 * |
 * |  Permission is hereby granted, free of charge, to any person obtaining a copy
 * |  of this software and associated documentation files (the "Software"), to deal
 * |  in the Software without restriction, including without limitation the rights
 * |  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * |  copies of the Software, and to permit persons to whom the Software is
 * |  furnished to do so, subject to the following conditions:
 * |
 * |  The above copyright notice and this permission notice shall be included in
 * |  all copies or substantial portions of the Software.
 * |
 * |  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * |  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * |  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * |  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * |  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * |  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * |  THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "../mcJSON.h"


/* Read a file, parse, render back, etc. */
int dofile(char *input_filename, char *output_filename) {
	//read the file
	FILE *input_file = fopen(input_filename, "rb");
	if (input_file == NULL) {
		fprintf(stderr, "ERROR: Failed to open file '%s'\n", input_filename);
		return 0;
	}
	fseek(input_file, 0, SEEK_END);
	int status;
	status = ftell(input_file);
	if (status == -1) {
		fprintf(stderr, "ERROR: Failed to get length of file '%s'\n", input_filename);
		fclose(input_file);;
		return 0;
	}
	size_t length = (size_t) status;
	fseek(input_file, 0, SEEK_SET);
	char *data;
	data = (char*)malloc(length + 1);
	if (data == NULL) {
		fclose(input_file);
		return 0;
	}
	size_t read_length = fread(data, 1, length, input_file);
	if ((read_length != length) || (ferror(input_file) != 0)) {
		fprintf(stderr, "Error occured while reading file '%s'!\n", input_filename);
		fclose(input_file);
		free(data);
		return 0;
	}

	//create output file
	FILE *output_file = NULL;
	if (output_filename != NULL) { //only if filename is given
		output_file = fopen(output_filename, "w");
		if (output_file == NULL) {
			fprintf(stderr, "ERROR: Failed to open file '%s'!\n", output_filename);
			fclose(input_file);
			free(data);
			return 0;
		}
	}

	//nullterminate the string
	data[length] = '\0';
	fclose(input_file);


	//now do the tests
	buffer_create_with_existing_array(data_buffer, (unsigned char*)data, length);
	status = doit(data_buffer, output_file);
	if (output_file != NULL) {
		fclose(output_file);
	}
	free(data);

	return status;
}

/* Used by some code below as an example datatype. */
struct record {
	const char *precision;
	double lat;
	double lon;
	const char *address;
	const char *city;
	const char *state;
	const char *zip;
	const char *country;
};

int main (int argc, char **argv) {
	if ((argc != 2) && (argc != 3)) {
		fprintf(stderr, "ERROR: Invalid arguments!\n");
		fprintf(stderr, "Usage: %s input [output]\n", argv[0]);
		return EXIT_FAILURE;
	}

	int status;
	if (argc == 2) {
		status = dofile(argv[1], NULL);
	} else {
		status = dofile(argv[1], argv[2]);
	}
	if (status == 0) {
		fprintf(stderr, "ERROR: Failed to process file! (%i)\n", status);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
