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


/* Parse text to JSON, then render back to text, and print! */
int doit(buffer_t *input_string, FILE *output_file) {
	buffer_t *output = NULL;

	mcJSON *json;
	json = mcJSON_Parse(input_string);
	if (json == NULL) {
		fprintf(stderr, "Error before: [%.*s]\n", (int)input_string->content_length, (char*)input_string->content + input_string->position);
		return 0;
	}

	output = mcJSON_Print(json);
	if (output == NULL) {
		mcJSON_Delete(json);
		return 0;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	//Do the same thing unformatted
	output = mcJSON_PrintUnformatted(json);
	if (output == NULL) {
		mcJSON_Delete(json);
		return 0;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	//Do the same thing buffered
	output = mcJSON_PrintBuffered(json, 20, 1);
	if (output == NULL) {
		mcJSON_Delete(json);
		return 0;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	mcJSON_Delete(json);

	// Test minify
	buffer_t *minify_buffer = buffer_create_on_heap(input_string->content_length, input_string->content_length);
	if (buffer_clone(minify_buffer, input_string) != 0) {
		buffer_destroy_from_heap(minify_buffer);
		return 0;
	}
	mcJSON_Minify(minify_buffer);
	printf("%.*s\n", (int)minify_buffer->content_length, (char*)minify_buffer->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)minify_buffer->content_length, (char*)minify_buffer->content);
	}
	buffer_destroy_from_heap(minify_buffer);

	return 1;
}
