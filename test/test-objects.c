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
	buffer_create_from_string(sunday, "Sunday");
	buffer_create_from_string(monday, "Monday");
	buffer_create_from_string(tuesday, "Tuesday");
	buffer_create_from_string(wednesday, "Wednesday");
	buffer_create_from_string(thursday, "Thursday");
	buffer_create_from_string(friday, "Friday");
	buffer_create_from_string(saturday, "Saturday");
	buffer_t *strings[7] = {
		sunday,
		monday,
		tuesday,
		wednesday,
		thursday,
		friday,
		saturday
	};
	/* Our matrix: */
	int numbers[3][3] = {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}};
	/* Our "gallery" item: */
	int ids[4] = {116, 943, 234, 38793};
	/* Our array of "records": */
	buffer_create_from_string(zip_buffer, "zip");
	buffer_create_from_string(empty_string_buffer, "");
	buffer_create_from_string(san_francisco_buffer, "SAN FRANCISCO");
	buffer_create_from_string(ca_buffer, "CA");
	buffer_create_from_string(zip_number, "94107");
	buffer_create_from_string(us_buffer, "US");
	buffer_create_from_string(sunnyvale_buffer, "SUNNYVALE");
	buffer_create_from_string(zip_number2, "94085");
	struct record fields[2] = {
		{
			zip_buffer,
			37.7668,
			-1.223959e+2,
			empty_string_buffer,
			san_francisco_buffer,
			ca_buffer,
			zip_number,
			us_buffer
		},
		{
			zip_buffer,
			37.371991,
			-1.22026e+2,
			empty_string_buffer,
			sunnyvale_buffer,
			ca_buffer,
			zip_number2,
			us_buffer
		}
	};

	/* Here we construct some JSON standards, from the JSON site. */

	/* Our "Video" datatype: */
	root = mcJSON_CreateObject(NULL);
	buffer_create_from_string(jack_buffer, "Jack (\"Bee\") Nimble");
	buffer_create_from_string(name_buffer, "name");
	mcJSON_AddItemToObject(root, name_buffer, mcJSON_CreateString(jack_buffer, NULL), NULL);
	buffer_create_from_string(format_buffer, "format");
	mcJSON_AddItemToObject(root, format_buffer, fmt=mcJSON_CreateObject(NULL), NULL);
	buffer_create_from_string(rect_buffer, "rect");
	buffer_create_from_string(type_buffer, "type");
	mcJSON_AddStringToObject(fmt, type_buffer, rect_buffer, NULL);
	buffer_create_from_string(width_buffer, "width");
	mcJSON_AddNumberToObject(fmt, width_buffer, 1920, NULL);
	buffer_create_from_string(height_buffer, "height");
	mcJSON_AddNumberToObject(fmt, height_buffer, 1080, NULL);
	buffer_create_from_string(interlace, "interlace");
	mcJSON_AddFalseToObject (fmt, interlace, NULL);
	buffer_create_from_string(fps_buffer, "frame rate");
	mcJSON_AddNumberToObject(fmt, fps_buffer, 24, NULL);

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
	buffer_create_from_string(image_buffer, "Image");
	mcJSON_AddItemToObject(root, image_buffer, img = mcJSON_CreateObject(NULL), NULL);
	buffer_create_from_string(Width_buffer, "Width");
	mcJSON_AddNumberToObject(img, Width_buffer, 800, NULL);
	buffer_create_from_string(Height_buffer, "Height");
	mcJSON_AddNumberToObject(img, Height_buffer, 600, NULL);
	buffer_create_from_string(title_buffer, "Title");
	buffer_create_from_string(view_buffer, "View from 15th Floor");
	mcJSON_AddStringToObject(img, title_buffer, view_buffer, NULL);
	buffer_create_from_string(thumbnail_buffer, "Thumbnail");
	mcJSON_AddItemToObject(img, thumbnail_buffer, thm = mcJSON_CreateObject(NULL), NULL);
	buffer_create_from_string(url_buffer, "Url");
	buffer_create_from_string(url, "http:/*www.example.com/image/481989943");
	mcJSON_AddStringToObject(thm, url_buffer, url, NULL);
	mcJSON_AddNumberToObject(thm, Height_buffer, 125, NULL);
	buffer_create_from_string(hundred_buffer, "100");
	mcJSON_AddStringToObject(thm, Width_buffer, hundred_buffer, NULL);
	buffer_create_from_string(ids_buffer, "IDs");
	mcJSON_AddItemToObject(img, ids_buffer, mcJSON_CreateIntArray(ids, 4, NULL), NULL);

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
		buffer_create_from_string(precision_buffer, "precision");
		mcJSON_AddStringToObject(fld, precision_buffer, fields[i].precision, NULL);
		buffer_create_from_string(latitude_buffer, "Latitude");
		mcJSON_AddNumberToObject(fld, latitude_buffer, fields[i].lat, NULL);
		buffer_create_from_string(longitude_buffer, "Longitude");
		mcJSON_AddNumberToObject(fld, longitude_buffer, fields[i].lon, NULL);
		buffer_create_from_string(address_buffer, "Address");
		mcJSON_AddStringToObject(fld, address_buffer, fields[i].address, NULL);
		buffer_create_from_string(city_buffer, "City");
		mcJSON_AddStringToObject(fld, city_buffer, fields[i].city, NULL);
		buffer_create_from_string(state_buffer, "State");
		mcJSON_AddStringToObject(fld, state_buffer, fields[i].state, NULL);
		buffer_create_from_string(zip_buffer, "Zip");
		mcJSON_AddStringToObject(fld, zip_buffer, fields[i].zip, NULL);
		buffer_create_from_string(country_buffer, "Country");
		mcJSON_AddStringToObject(fld, country_buffer, fields[i].country, NULL);
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
	buffer_create_from_string(number_buffer, "number");
	mcJSON_AddNumberToObject(root, number_buffer, INFINITY, NULL);

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
	buffer_create_from_string(a_buffer, "a");
	mcJSON_AddNumberToObject(root, a_buffer, 1, NULL);
	buffer_create_from_string(A_buffer, "A");
	mcJSON_AddNumberToObject(root, A_buffer, 2, NULL);

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

	mcJSON *one = mcJSON_GetObjectItem(root, a_buffer);
	mcJSON *two = mcJSON_GetObjectItem(root, A_buffer);
	if ((one == NULL) || (one->valueint != 1)
			|| (two == NULL) || (two->valueint != 2)) {
		fprintf(stderr, "ERROR: Failed to get item from object.\n");
		return EXIT_FAILURE;
	}
	mcJSON_Delete(root);

	/* test creation of hex strings */
	buffer_create_from_string(newline_buffer, "\r\n");
	root = mcJSON_CreateHexString(newline_buffer, NULL);
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
		buffer_create_with_existing_array(json_buffer, (unsigned char*)json[i], strlen(json[i]) + 1);
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
