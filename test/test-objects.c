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
	const char *strings[7] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
	/* Our matrix: */
	int numbers[3][3] = {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}};
	/* Our "gallery" item: */
	int ids[4] = {116, 943, 234, 38793};
	/* Our array of "records": */
	struct record fields[2] = {
		{"zip", 37.7668, -1.223959e+2, "", "SAN FRANCISCO", "CA", "94107", "US"},
		{"zip", 37.371991, -1.22026e+2, "", "SUNNYVALE", "CA", "94085", "US"}
	};

	/* Here we construct some JSON standards, from the JSON site. */

	/* Our "Video" datatype: */
	root = mcJSON_CreateObject();
	mcJSON_AddItemToObject(root, "name", mcJSON_CreateString("Jack (\"Bee\") Nimble"));
	mcJSON_AddItemToObject(root, "format", fmt=mcJSON_CreateObject());
	mcJSON_AddStringToObject(fmt, "type", "rect");
	mcJSON_AddNumberToObject(fmt, "width", 1920);
	mcJSON_AddNumberToObject(fmt, "height", 1080);
	mcJSON_AddFalseToObject (fmt, "interlace");
	mcJSON_AddNumberToObject(fmt, "frame rate", 24);

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
	root = mcJSON_CreateStringArray(strings, 7);

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
	root = mcJSON_CreateArray();
	for (i = 0; i < 3; i++) {
		mcJSON_AddItemToArray(root, mcJSON_CreateIntArray(numbers[i], 3));
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
	root = mcJSON_CreateObject();
	mcJSON_AddItemToObject(root, "Image", img = mcJSON_CreateObject());
	mcJSON_AddNumberToObject(img, "Width", 800);
	mcJSON_AddNumberToObject(img, "Height", 600);
	mcJSON_AddStringToObject(img, "Title", "View from 15th Floor");
	mcJSON_AddItemToObject(img, "Thumbnail", thm = mcJSON_CreateObject());
	mcJSON_AddStringToObject(thm, "Url", "http:/*www.example.com/image/481989943");
	mcJSON_AddNumberToObject(thm, "Height", 125);
	mcJSON_AddStringToObject(thm, "Width", "100");
	mcJSON_AddItemToObject(img, "IDs", mcJSON_CreateIntArray(ids, 4));

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

	root = mcJSON_CreateArray();
	for (i = 0; i < 2; i++) {
		mcJSON_AddItemToArray(root, fld = mcJSON_CreateObject());
		mcJSON_AddStringToObject(fld, "precision", fields[i].precision);
		mcJSON_AddNumberToObject(fld, "Latitude", fields[i].lat);
		mcJSON_AddNumberToObject(fld, "Longitude", fields[i].lon);
		mcJSON_AddStringToObject(fld, "Address", fields[i].address);
		mcJSON_AddStringToObject(fld, "City", fields[i].city);
		mcJSON_AddStringToObject(fld, "State", fields[i].state);
		mcJSON_AddStringToObject(fld, "Zip", fields[i].zip);
		mcJSON_AddStringToObject(fld, "Country", fields[i].country);
	}

	/*	mcJSON_ReplaceItemInObject(mcJSON_GetArrayItem(root,1),"City",mcJSON_CreateIntArray(ids,4)); */

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

	root = mcJSON_CreateObject();
	mcJSON_AddNumberToObject(root, "number", 1.0/0.0);

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
	root = mcJSON_CreateObject();
	mcJSON_AddNumberToObject(root, "a", 1);
	mcJSON_AddNumberToObject(root, "A", 2);

	output = mcJSON_Print(root);
	if (output == NULL) {
		return EXIT_FAILURE;
	}
	printf("%.*s\n", (int)output->content_length, (char*)output->content);
	if (output_file != NULL) {
		fprintf(output_file, "%.*s\n", (int)output->content_length, (char*)output->content);
	}
	buffer_destroy_from_heap(output);

	mcJSON *one = mcJSON_GetObjectItem(root, "a");
	mcJSON *two = mcJSON_GetObjectItem(root, "A");
	if ((one == NULL) || (one->valueint != 1)
			|| (two == NULL) || (two->valueint != 2)) {
		fprintf(stderr, "ERROR: Failed to get item from object.\n");
		return EXIT_FAILURE;
	}
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
		int status = doit(json[i], output_file);
		if (status == 0) {
			fprintf(stderr, "ERROR: Failed on text %zi!\n", i);
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
