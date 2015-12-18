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
#include <string.h>
#include <math.h>

#include "common.h"
#include "../mcJSON.h"

/* Used by some code below as an example datatype. */
struct record {
	buffer_t *precision;
	double lat;
	double lon;
	buffer_t *address;
	buffer_t *city;
	buffer_t *state;
	buffer_t *zip;
	buffer_t *country;
};

/* Create a bunch of objects as demonstration. */
int create_objects(FILE *output_file) {
	/* declare a few. */
	mcJSON *root;
	mcJSON *fmt;
	mcJSON *img;
	mcJSON *thm;
	mcJSON *fld;
	int i;
	/* Our "days of the week" array: */
	buffer_t *strings[7] = {
		buffer_create_from_string("Sunday"),
		buffer_create_from_string("Monday"),
		buffer_create_from_string("Tuesday"),
		buffer_create_from_string("Wednesday"),
		buffer_create_from_string("Thursday"),
		buffer_create_from_string("Friday"),
		buffer_create_from_string("Saturday")
	};
	/* Our matrix: */
	int numbers[3][3] = {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}};
	/* Our "gallery" item: */
	int ids[4] = {116, 943, 234, 38793};
	/* Our array of "records": */
	struct record fields[2] = {
		{
			buffer_create_from_string("zip"),
			37.7668,
			-1.223959e+2,
			buffer_create_from_string(""),
			buffer_create_from_string("SAN FRANCISCO"),
			buffer_create_from_string("CA"),
			buffer_create_from_string("94107"),
			buffer_create_from_string("US")
		},
		{
			buffer_create_from_string("zip"),
			37.371991,
			-1.22026e+2,
			buffer_create_from_string(""),
			buffer_create_from_string("SUNNYVALE"),
			buffer_create_from_string("CA"),
			buffer_create_from_string("94085"),
			buffer_create_from_string("US")
		}
	};

	/* buffers to put string literals into */
	buffer_t *string_literal_buffer = NULL;
	buffer_t *name_literal_buffer = NULL;

	/* Here we construct some JSON standards, from the JSON site. */

	/* Our "Video" datatype: */
	root = mcJSON_CreateObject(NULL);
	string_literal_buffer = buffer_create_from_string("Jack (\"Bee\") Nimble");
	name_literal_buffer = buffer_create_from_string("name");
	mcJSON_AddItemToObject(root, name_literal_buffer, mcJSON_CreateString(string_literal_buffer, NULL), NULL);
	name_literal_buffer = buffer_create_from_string("format");
	mcJSON_AddItemToObject(root, name_literal_buffer, fmt=mcJSON_CreateObject(NULL), NULL);
	name_literal_buffer = buffer_create_from_string("type");
	string_literal_buffer = buffer_create_from_string("rect");
	mcJSON_AddStringToObject(fmt, name_literal_buffer, string_literal_buffer, NULL);
	name_literal_buffer = buffer_create_from_string("width");
	mcJSON_AddNumberToObject(fmt, name_literal_buffer, 1920, NULL);
	name_literal_buffer = buffer_create_from_string("height");
	mcJSON_AddNumberToObject(fmt, name_literal_buffer, 1080, NULL);
	name_literal_buffer = buffer_create_from_string("interlace");
	mcJSON_AddFalseToObject (fmt, name_literal_buffer, NULL);
	name_literal_buffer = buffer_create_from_string("frame rate");
	mcJSON_AddNumberToObject(fmt, name_literal_buffer, 24, NULL);

	buffer_t *output = NULL;
	/* Print to text, Delete the mcJSON, print it, release the string. */
	output = mcJSON_Print(root);
	mcJSON_Delete(root);
	if (output == NULL) {
		return EXIT_FAILURE;
	}
	printf("%.*s\n", (int) output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	/* Our "days of the week" array: */
	root = mcJSON_CreateStringArray((const buffer_t **)strings, 7, NULL);

	output = mcJSON_Print(root);
	mcJSON_Delete(root);
	if (output == NULL) {
		return EXIT_FAILURE;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	/* Our matrix: */
	root = mcJSON_CreateArray(NULL);
	for (i = 0; i < 3; i++) {
		mcJSON_AddItemToArray(root, mcJSON_CreateIntArray(numbers[i], 3, NULL), NULL);
	}

	/*mcJSON_ReplaceItemInArray(root,1,mcJSON_CreateString("Replacement")); */

	output = mcJSON_Print(root);
	mcJSON_Delete(root);
	if (output == NULL) {
		return EXIT_FAILURE;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);


	/* Our "gallery" item: */
	root = mcJSON_CreateObject(NULL);
	name_literal_buffer = buffer_create_from_string("Image");
	mcJSON_AddItemToObject(root, name_literal_buffer, img = mcJSON_CreateObject(NULL), NULL);
	name_literal_buffer = buffer_create_from_string("Width");
	mcJSON_AddNumberToObject(img, name_literal_buffer, 800, NULL);
	name_literal_buffer = buffer_create_from_string("Height");
	mcJSON_AddNumberToObject(img, name_literal_buffer, 600, NULL);
	name_literal_buffer = buffer_create_from_string("Title");
	string_literal_buffer = buffer_create_from_string("View from 15th Floor");
	mcJSON_AddStringToObject(img, name_literal_buffer, string_literal_buffer, NULL);
	name_literal_buffer = buffer_create_from_string("Thumbnail");
	mcJSON_AddItemToObject(img, name_literal_buffer, thm = mcJSON_CreateObject(NULL), NULL);
	name_literal_buffer = buffer_create_from_string("Url");
	string_literal_buffer = buffer_create_from_string("http:/*www.example.com/image/481989943");
	mcJSON_AddStringToObject(thm, name_literal_buffer, string_literal_buffer, NULL);
	name_literal_buffer = buffer_create_from_string("Height");
	mcJSON_AddNumberToObject(thm, name_literal_buffer, 125, NULL);
	name_literal_buffer = buffer_create_from_string("Width");
	string_literal_buffer = buffer_create_from_string("100");
	mcJSON_AddStringToObject(thm, name_literal_buffer, string_literal_buffer, NULL);
	name_literal_buffer = buffer_create_from_string("IDs");
	mcJSON_AddItemToObject(img, name_literal_buffer, mcJSON_CreateIntArray(ids, 4, NULL), NULL);

	output = mcJSON_Print(root);
	mcJSON_Delete(root);
	if (output == NULL) {
		return EXIT_FAILURE;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	/* Our array of "records": */

	root = mcJSON_CreateArray(NULL);
	for (i = 0; i < 2; i++) {
		mcJSON_AddItemToArray(root, fld = mcJSON_CreateObject(NULL), NULL);
		name_literal_buffer = buffer_create_from_string("precision");
		mcJSON_AddStringToObject(fld, name_literal_buffer, fields[i].precision, NULL);
		name_literal_buffer = buffer_create_from_string("Latitude");
		mcJSON_AddNumberToObject(fld, name_literal_buffer, fields[i].lat, NULL);
		name_literal_buffer = buffer_create_from_string("Longitude");
		mcJSON_AddNumberToObject(fld, name_literal_buffer, fields[i].lon, NULL);
		name_literal_buffer = buffer_create_from_string("Address");
		mcJSON_AddStringToObject(fld, name_literal_buffer, fields[i].address, NULL);
		name_literal_buffer = buffer_create_from_string("City");
		mcJSON_AddStringToObject(fld, name_literal_buffer, fields[i].city, NULL);
		name_literal_buffer = buffer_create_from_string("State");
		mcJSON_AddStringToObject(fld, name_literal_buffer, fields[i].state, NULL);
		name_literal_buffer = buffer_create_from_string("Zip");
		mcJSON_AddStringToObject(fld, name_literal_buffer, fields[i].zip, NULL);
		name_literal_buffer = buffer_create_from_string("Country");
		mcJSON_AddStringToObject(fld, name_literal_buffer, fields[i].country, NULL);
	}

	/*	mcJSON_ReplaceItemInObject(mcJSON_GetArrayItem(root,1),"City",mcJSON_CreateIntArray(ids,4,NULL)); */

	output = mcJSON_Print(root);
	mcJSON_Delete(root);
	if (output == NULL) {
		return EXIT_FAILURE;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	root = mcJSON_CreateObject(NULL);
	name_literal_buffer = buffer_create_from_string("number");
	mcJSON_AddNumberToObject(root, name_literal_buffer, INFINITY, NULL);

	output = mcJSON_Print(root);
	mcJSON_Delete(root);
	if (output == NULL) {
		return EXIT_FAILURE;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	/* Check mcJSON_GetObjectItem and make sure it's case sensitive */
	root = mcJSON_CreateObject(NULL);
	name_literal_buffer = buffer_create_from_string("a");
	mcJSON_AddNumberToObject(root, name_literal_buffer, 1, NULL);
	name_literal_buffer = buffer_create_from_string("A");
	mcJSON_AddNumberToObject(root, name_literal_buffer, 2, NULL);

	output = mcJSON_Print(root);
	if (output == NULL) {
		mcJSON_Delete(root);
		return EXIT_FAILURE;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	name_literal_buffer = buffer_create_from_string("a");
	mcJSON *one = mcJSON_GetObjectItem(root, name_literal_buffer);
	name_literal_buffer = buffer_create_from_string("A");
	mcJSON *two = mcJSON_GetObjectItem(root, name_literal_buffer);
	if ((one == NULL) || (one->valueint != 1)
			|| (two == NULL) || (two->valueint != 2)) {
		fprintf(stderr, "ERROR: Failed to get item from object.\n");
		return EXIT_FAILURE;
	}
	mcJSON_Delete(root);

	/* test creation of hex strings */
	root = mcJSON_CreateHexString(buffer_create_from_string("\r\n"), NULL);
	if (root == NULL) {
		return EXIT_FAILURE;
	}

	output = mcJSON_Print(root);
	if (output == NULL) {
		mcJSON_Delete(root);
		return EXIT_FAILURE;
	}
	/* TODO: Make function that does this! */
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);
	mcJSON_Delete(root);

	return 0;
}

int main (int argc, char **argv) {
	if ((argc != 1) && (argc != 2)) {
		fprintf(stderr, "ERROR: Invalid arguments!\n");
		fprintf(stderr, "Usage: %s [output_file]\n", argv[0]);
		return EXIT_FAILURE;
	}

	FILE *output_file = NULL;
	if ((argc == 2) && (argv[1] != NULL)) {
		output_file = fopen(argv[1], "w");
		if (output_file == NULL) {
			fprintf(stderr, "ERROR: Failed to open file '%s'\n", argv[1]);;
			return EXIT_FAILURE;
		}
	}

	/* a bunch of json: */
	char *json[] = {
		"{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}",

		"[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]",
		"[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n	]\n",
		"{\n		\"Image\": {\n			\"Width\":  800,\n			\"Height\": 600,\n			\"Title\":  \"View from 15th Floor\",\n			\"Thumbnail\": {\n				\"Url\":    \"http:/*www.example.com/image/481989943\",\n				\"Height\": 125,\n				\"Width\":  \"100\"\n			},\n			\"IDs\": [116, 943, 234, 38793]\n		}\n	}",
	"[\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.7668,\n	 \"Longitude\": -122.3959,\n	 \"Address\":   \"\",\n	 \"City\":      \"SAN FRANCISCO\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94107\",\n	 \"Country\":   \"US\"\n	 },\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.371991,\n	 \"Longitude\": -122.026020,\n	 \"Address\":   \"\",\n	 \"City\":      \"SUNNYVALE\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94085\",\n	 \"Country\":   \"US\"\n	 }\n	 ]"
	};

	/* Process each json textblock by parsing, then rebuilding: */
	for (size_t i = 0; i < (sizeof(json) / sizeof(char*)); i++) {
		buffer_t *json_buffer = buffer_create_with_existing_array((unsigned char*)json[i], strlen(json[i]) + 1);
		int status = doit(json_buffer, output_file);
		if (status == 0) {
			fprintf(stderr, "ERROR: Failed on text %zu!\n", i);
			if (output_file != NULL) {
				fclose(output_file);
			}
			return EXIT_FAILURE;
		}
	}

	/* Now some samplecode for building objects concisely: */
	if (create_objects(output_file) != 0) {
		if (output_file != NULL) {
			fclose(output_file);
		}
		return EXIT_FAILURE;
	}

	if (output_file != NULL) {
		fclose(output_file);
	}

	return EXIT_SUCCESS;
}
