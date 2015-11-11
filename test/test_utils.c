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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../mcJSON_Utils.h"

int main(int argc, char **argv) {
	if ((argc != 1) && (argc != 2)) {
		fprintf(stderr, "ERROR: Invalid arguments!\n");
		fprintf(stderr, "Usage: %s [output_file]\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE *output_file = NULL;
	if ((argc == 2) && (argv[1] != NULL)) {
		output_file = fopen(argv[1], "w");
		if (output_file == NULL) {
			fprintf(stderr, "ERROR: Failed to open file '%s'\n", argv[1]);
			return EXIT_FAILURE;
		}
	}

	int i;
	/* JSON Pointer tests: */
	mcJSON *root;
	buffer_t *json = buffer_create_from_string("{"
		"\"foo\": [\"bar\", \"baz\"],"
		"\"\": 0,"
		"\"a/b\": 1,"
		"\"c%d\": 2,"
		"\"e^f\": 3,"
		"\"g|h\": 4,"
		"\"i\\\\j\": 5,"
		"\"k\\\"l\": 6,"
		"\" \": 7,"
		"\"m~n\": 8"
	"}");

	const char *tests[12] = {"", "/foo", "/foo/0", "/", "/a~1b", "/c%d", "/e^f", "/g|h", "/i\\j", "/k\"l", "/ ", "/m~0n"};

	/* JSON Apply Patch tests: */
	const char *patches[15][3] = {
		{"{ \"foo\": \"bar\"}", "[{ \"op\": \"add\", \"path\": \"/baz\", \"value\": \"qux\" }]", "{\"baz\": \"qux\",\"foo\": \"bar\"}"},
		{"{ \"foo\": [ \"bar\", \"baz\" ] }", "[{ \"op\": \"add\", \"path\": \"/foo/1\", \"value\": \"qux\" }]", "{\"foo\": [ \"bar\", \"qux\", \"baz\" ] }"},
		{"{\"baz\": \"qux\",\"foo\": \"bar\"}", " [{ \"op\": \"remove\", \"path\": \"/baz\" }]", "{\"foo\": \"bar\" }"},
		{"{ \"foo\": [ \"bar\", \"qux\", \"baz\" ] }", "[{ \"op\": \"remove\", \"path\": \"/foo/1\" }]", "{\"foo\": [ \"bar\", \"baz\" ] }"},
		{"{ \"baz\": \"qux\",\"foo\": \"bar\"}", "[{ \"op\": \"replace\", \"path\": \"/baz\", \"value\": \"boo\" }]", "{\"baz\": \"boo\",\"foo\": \"bar\"}"},
		{"{\"foo\": {\"bar\": \"baz\",\"waldo\": \"fred\"},\"qux\": {\"corge\": \"grault\"}}", "[{ \"op\": \"move\", \"from\": \"/foo/waldo\", \"path\": \"/qux/thud\" }]", "{\"foo\": {\"bar\": \"baz\"},\"qux\": {\"corge\": \"grault\",\"thud\": \"fred\"}}"},
		{"{ \"foo\": [ \"all\", \"grass\", \"cows\", \"eat\" ] }", "[ { \"op\": \"move\", \"from\": \"/foo/1\", \"path\": \"/foo/3\" }]", "{ \"foo\": [ \"all\", \"cows\", \"eat\", \"grass\" ] }"},
		{"{\"baz\": \"qux\",\"foo\": [ \"a\", 2, \"c\" ]}", "[{ \"op\": \"test\", \"path\": \"/baz\", \"value\": \"qux\" },{ \"op\": \"test\", \"path\": \"/foo/1\", \"value\": 2 }]", ""},
		{"{ \"baz\": \"qux\" }", "[ { \"op\": \"test\", \"path\": \"/baz\", \"value\": \"bar\" }]", ""},
		{"{ \"foo\": \"bar\" }", "[{ \"op\": \"add\", \"path\": \"/child\", \"value\": { \"grandchild\": { } } }]", "{\"foo\": \"bar\",\"child\": {\"grandchild\": {}}}"},
		{"{ \"foo\": \"bar\" }", "[{ \"op\": \"add\", \"path\": \"/baz\", \"value\": \"qux\", \"xyz\": 123 }]", "{\"foo\": \"bar\",\"baz\": \"qux\"}"},
		{"{ \"foo\": \"bar\" }", "[{ \"op\": \"add\", \"path\": \"/baz/bat\", \"value\": \"qux\" }]", ""},
		{"{\"/\": 9,\"~1\": 10}", "[{\"op\": \"test\", \"path\": \"/~01\", \"value\": 10}]", ""},
		{"{\"/\": 9,\"~1\": 10}", "[{\"op\": \"test\", \"path\": \"/~01\", \"value\": \"10\"}]", ""},
		{"{ \"foo\": [\"bar\"] }", "[ { \"op\": \"add\", \"path\": \"/foo/-\", \"value\": [\"abc\", \"def\"] }]", "{\"foo\": [\"bar\", [\"abc\", \"def\"]] }"}
	};

	/* Misc tests */
	int numbers[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	const char *random = "QWERTYUIOPASDFGHJKLZXCVBNM";
	char buf[2] = {0, 0};
	mcJSON *object;
	mcJSON *nums;
	mcJSON *num6;
	mcJSON *sortme;

	printf("JSON Pointer Tests\n");
	if (output_file != NULL) {
		fprintf(output_file, "JSON Pointer Tests\n");
	}
	root = mcJSON_Parse(json);
	for (i = 0; i < 12; i++) {
		buffer_t *output = mcJSON_Print(mcJSONUtils_GetPointer(root, tests[i]));
		if (output == NULL) {
			fprintf(stderr, "ERROR: JSON Pointer Test %d failed!\n", i + 1);
			mcJSON_Delete(root);
			if (output_file != NULL) {
				fclose(output_file);
			}
			return EXIT_FAILURE;
		}
		printf("Test %d:\n%.*s\n\n", i + 1, (int) output->content_length, (char*)output->content);
		if (output_file != NULL) {
			fprintf(output_file, "Test %d:\n%.*s\n\n", i + 1, (int)output->content_length, (char*)output->content);
		}
		buffer_destroy_from_heap(output);
	}
	mcJSON_Delete(root);


	printf("JSON Apply Patch Tests\n");
	if (output_file != NULL) {
		fprintf(output_file, "JSON Apply Patch Tests\n");
	}
	for (i = 0; i < 15; i++) {
		buffer_t *object_buffer = buffer_create_with_existing_array((unsigned char*)patches[i][0], strlen(patches[i][0]) + 1);
		mcJSON *object = mcJSON_Parse(object_buffer);
		buffer_t *patch_buffer = buffer_create_with_existing_array((unsigned char*)patches[i][1], strlen(patches[i][1]) + 1);
		mcJSON *patch = mcJSON_Parse(patch_buffer);
		int err = mcJSONUtils_ApplyPatches(object, patch);
		buffer_t *output = mcJSON_Print(object);
		if (output == NULL) {
			fprintf(stderr, "ERROR: JSON Apply Patch Test %d failed!\n", i + 1);
			mcJSON_Delete(object);
			mcJSON_Delete(patch);
			if (output_file != NULL) {
				fclose(output_file);
			}
			return EXIT_FAILURE;
		}
		printf("Test %d (err %d):\n%.*s\n\n", i + 1, err, (int) output->content_length, (char*)output->content);
		if (output_file != NULL) {
			fprintf(output_file, "Test %d (err %d):\n%.*s\n\n", i + 1, err, (int)output->content_length, (char*) output->content);
		}
		buffer_destroy_from_heap(output);
		mcJSON_Delete(object);
		mcJSON_Delete(patch);
	}

	/* JSON Generate Patch tests: */
	printf("JSON Generate Patch Tests\n");
	if (output_file != NULL) {
		fprintf(output_file, "JSON Generate Patch Tests\n");
	}
	for (i = 0; i < 15; i++) {
		mcJSON *from;
		mcJSON *to;
		mcJSON *patch;
		if (!strlen(patches[i][2])) {
			continue;
		}
		buffer_t *from_buffer = buffer_create_with_existing_array((unsigned char*)patches[i][0], strlen(patches[i][0]) + 1);
		from = mcJSON_Parse(from_buffer);
		buffer_t *to_buffer = buffer_create_with_existing_array((unsigned char*)patches[i][2], strlen(patches[i][2]) + 1);
		to = mcJSON_Parse(to_buffer);
		patch = mcJSONUtils_GeneratePatches(from, to);
		buffer_t *output = mcJSON_Print(patch);
		if (output == NULL) {
			fprintf(stderr, "ERROR: JSON Generate Path Test %d failed!\n", i + 1);
			mcJSON_Delete(from);
			mcJSON_Delete(to);
			mcJSON_Delete(patch);
			if (output_file != NULL) {
				fclose(output_file);
			}
			return EXIT_FAILURE;
		}
		printf("Test %d: (patch: %s):\n%.*s\n\n", i + 1, patches[i][1], (int)output->content_length, (char*)output->content);
		if (output_file != NULL) {
			fprintf(output_file, "Test %d: (patch: %s):\n%.*s\n\n", i + 1, patches[i][1], (int)output->content_length, (char*)output->content);
		}
		buffer_destroy_from_heap(output);
		mcJSON_Delete(from);
		mcJSON_Delete(to);
		mcJSON_Delete(patch);
	}

	/* Misc tests: */
	printf("JSON Pointer construct\n");
	if (output_file != NULL) {
		fprintf(output_file, "JSON Pointer construct\n");
	}
	object = mcJSON_CreateObject(NULL);
	nums = mcJSON_CreateIntArray(numbers, 10, NULL);
	num6 = mcJSON_GetArrayItem(nums, 6);
	mcJSON_AddItemToObject(object, "numbers", nums);
	char *temp = mcJSONUtils_FindPointerFromObjectTo(object, num6);
	if (temp == NULL) {
		fprintf(stderr, "ERROR: JSON Pointer construct 1 failed!\n");
		free(temp);
		mcJSON_Delete(nums);
		mcJSON_Delete(num6);
		mcJSON_Delete(object);
		if (output_file != NULL) {
			fclose(output_file);
		}
		return EXIT_FAILURE;
	}
	printf("Pointer: [%s]\n", temp);
	if (output_file != NULL) {
		fprintf(output_file, "Pointer: [%s]\n", temp);
	}
	free(temp);
	temp = mcJSONUtils_FindPointerFromObjectTo(object, nums);
	if (temp == NULL) {
		fprintf(stderr, "ERROR: JSON Pointer construct 1 failed!\n");
		free(temp);
		mcJSON_Delete(nums);
		mcJSON_Delete(num6);
		mcJSON_Delete(object);
		if (output_file != NULL) {
			fclose(output_file);
		}
		return EXIT_FAILURE;
	}
	printf("Pointer: [%s]\n", temp);
	if (output_file != NULL) {
		fprintf(output_file, "Pointer: [%s]\n", temp);
	}
	free(temp);
	temp = mcJSONUtils_FindPointerFromObjectTo(object, object);
	if (temp == NULL) {
		fprintf(stderr, "ERROR: JSON Pointer construct 1 failed!\n");
		free(temp);
		mcJSON_Delete(nums);
		mcJSON_Delete(num6);
		mcJSON_Delete(object);
		if (output_file != NULL) {
			fclose(output_file);
		}
		return EXIT_FAILURE;
	}
	printf("Pointer: [%s]\n", temp);
	if (output_file != NULL) {
		fprintf(output_file, "Pointer: [%s]\n", temp);
	}
	free(temp);
	mcJSON_Delete(object);

	/*TODO do some cleanup */

	/* JSON Sort test: */
	sortme = mcJSON_CreateObject(NULL);
	for (i = 0; i < 26; i++) {
		buf[0] = random[i];
		mcJSON_AddItemToObject(sortme, buf, mcJSON_CreateNumber(1, NULL));
	}
	buffer_t *before = mcJSON_PrintUnformatted(sortme);
	mcJSONUtils_SortObject(sortme);
	buffer_t *after = mcJSON_PrintUnformatted(sortme);
	printf("Before: [%.*s]\nAfter: [%.*s]\n\n", (int)before->content_length, (char*)before->content, (int)after->content_length, (char*)after->content);
	if (output_file != NULL) {
		fprintf(output_file, "Before: [%.*s]\nAfter: [%.*s]\n\n", (int)before->content_length, (char*)before->content, (int)after->content_length, (char*)after->content);
	}
	buffer_destroy_from_heap(before);
	buffer_destroy_from_heap(after);
	mcJSON_Delete(sortme);

	if (output_file != NULL) {
		fclose(output_file);
	}

	return EXIT_SUCCESS;
}
