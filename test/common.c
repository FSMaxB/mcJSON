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


/* Parse text to JSON, then render back to text, and print! */
int doit(buffer_t *input_string, FILE *output_file) {
	buffer_t *output = NULL;

	mcJSON *json;
	json = mcJSON_Parse(input_string);
	if (json == NULL) {
		fprintf(stderr, "Error before: [%s]\n", mcJSON_GetErrorPtr());
		return 0;
	} else {
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
	}
	return 1;
}
