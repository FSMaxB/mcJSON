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
#include "../cJSON_Utils.h"

int main(void) {
	int i;
	/* JSON Pointer tests: */
	cJSON *root;
	const char *json= "{"
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
	"}";

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
	char *before;
	char *after;
	cJSON *object;
	cJSON *nums;
	cJSON *num6;
	cJSON *sortme;

	printf("JSON Pointer Tests\n");
	root = cJSON_Parse(json);
	for (i = 0; i < 12; i++) {
		char *output = cJSON_Print(cJSONUtils_GetPointer(root, tests[i]));
		printf("Test %d:\n%s\n\n", i + 1,output);
		free(output);
	}
	cJSON_Delete(root);


	printf("JSON Apply Patch Tests\n");
	for (i = 0; i < 15; i++) {
		cJSON *object = cJSON_Parse(patches[i][0]);
		cJSON *patch = cJSON_Parse(patches[i][1]);
		int err = cJSONUtils_ApplyPatches(object, patch);
		char *output = cJSON_Print(object);
		printf("Test %d (err %d):\n%s\n\n", i + 1, err, output);
		free(output);
		cJSON_Delete(object);
		cJSON_Delete(patch);
	}

	/* JSON Generate Patch tests: */
	printf("JSON Generate Patch Tests\n");
	for (i = 0; i < 15; i++) {
		cJSON *from;
		cJSON *to;
		cJSON *patch;
		char *out;
		if (!strlen(patches[i][2])) {
			continue;
		}
		from = cJSON_Parse(patches[i][0]);
		to = cJSON_Parse(patches[i][2]);
		patch = cJSONUtils_GeneratePatches(from, to);
		out = cJSON_Print(patch);
		printf("Test %d: (patch: %s):\n%s\n\n", i + 1, patches[i][1], out);
		free(out);
		cJSON_Delete(from);
		cJSON_Delete(to);
		cJSON_Delete(patch);
	}

	/* Misc tests: */
	printf("JSON Pointer construct\n");
	object = cJSON_CreateObject();
	nums = cJSON_CreateIntArray(numbers, 10);
	num6 = cJSON_GetArrayItem(nums, 6);
	cJSON_AddItemToObject(object, "numbers", nums);
	char *temp = cJSONUtils_FindPointerFromObjectTo(object, num6);
	printf("Pointer: [%s]\n", temp);
	free(temp);
	temp = cJSONUtils_FindPointerFromObjectTo(object, nums);
	printf("Pointer: [%s]\n", temp);
	free(temp);
	temp = cJSONUtils_FindPointerFromObjectTo(object, object);
	printf("Pointer: [%s]\n", temp);
	free(temp);
	cJSON_Delete(object);

	/* JSON Sort test: */
	sortme = cJSON_CreateObject();
	for (i = 0; i < 26; i++) {
		buf[0] = random[i];
		cJSON_AddItemToObject(sortme, buf, cJSON_CreateNumber(1));
	}
	before = cJSON_PrintUnformatted(sortme);
	cJSONUtils_SortObject(sortme);
	after = cJSON_PrintUnformatted(sortme);
	printf("Before: [%s]\nAfter: [%s]\n\n", before, after);
	free(before);
	free(after);
	cJSON_Delete(sortme);
}
