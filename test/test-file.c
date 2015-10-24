/*
 * mcJSON, a modified version of cJSON, a simple JSON parser and generator.
 *  Copyright (C) 2009 Dave Gamble
 *  Copyright (C) 2015  Max Bruckner (FSMaxB)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
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
int dofile(char *input, char *output) {
	//read the file
	FILE *input_file = fopen(input, "rb");
	if (input_file == NULL) {
		fprintf(stderr, "ERROR: Failed to open file '%s'\n", input);
		return 0;
	}
	fseek(input_file, 0, SEEK_END);
	int status;
	status = ftell(input_file);
	if (status == -1) {
		fprintf(stderr, "ERROR: Failed to get length of file '%s'\n", input);
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
		fprintf(stderr, "Error occured while reading file '%s'!\n", input);
		fclose(input_file);
		free(data);
		return 0;
	}

	//create output file
	FILE *output_file = NULL;
	if (output != NULL) { //only if filename is given
		output_file = fopen(output, "w");
		if (output_file == NULL) {
			fprintf(stderr, "ERROR: Failed to open file '%s'!\n", output);
			fclose(input_file);
			free(data);
			return 0;
		}
	}

	//nullterminate the string
	data[length] = '\0';
	fclose(input_file);


	//now do the tests
	status = doit(data, output_file);
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
