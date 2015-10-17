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
#include "../cJSON.h"

/* Parse text to JSON, then render back to text, and print! */
int doit(char *text) {
	char *out;
	cJSON *json;

	json = cJSON_Parse(text);
	if (json == NULL) {
		fprintf(stderr, "Error before: [%s]\n", cJSON_GetErrorPtr());
		return 0;
	} else {
		out = cJSON_Print(json);
		cJSON_Delete(json);
		printf("%s\n", out);
		free(out);
	}
	return 1;
}

/* Read a file, parse, render back, etc. */
int dofile(char *filename) {
	FILE *file;
	size_t length;
	char *data;

	//read the file
	file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "ERROR: Failed to open file '%s'\n", filename);
		fclose(file);
		return 0;
	}
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	data = (char*)malloc(length + 1);
	size_t read_length = fread(data, 1, length, file);
	if ((read_length != length) || (ferror(file) != 0)) {
		fprintf(stderr, "Error occured while reading file '%s'!\n", filename);
		fclose(file);
		free(data);
		return 0;
	}
	//nullterminate the string
	data[length] = '\0';
	fclose(file);
	int status = doit(data);
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

/* Create a bunch of objects as demonstration. */
void create_objects() {
	/* declare a few. */
	cJSON *root;
	cJSON *fmt;
	cJSON *img;
	cJSON *thm;
	cJSON *fld;
	char *out;
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
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "name", cJSON_CreateString("Jack (\"Bee\") Nimble"));
	cJSON_AddItemToObject(root, "format", fmt=cJSON_CreateObject());
	cJSON_AddStringToObject(fmt, "type", "rect");
	cJSON_AddNumberToObject(fmt, "width", 1920);
	cJSON_AddNumberToObject(fmt, "height", 1080);
	cJSON_AddFalseToObject (fmt, "interlace");
	cJSON_AddNumberToObject(fmt, "frame rate", 24);

	/* Print to text, Delete the cJSON, print it, release the string. */
	out = cJSON_Print(root);
	cJSON_Delete(root);
	printf("%s\n", out);
	free(out);

	/* Our "days of the week" array: */
	root = cJSON_CreateStringArray(strings, 7);

	out = cJSON_Print(root);
	cJSON_Delete(root);
	printf("%s\n", out);
	free(out);

	/* Our matrix: */
	root = cJSON_CreateArray();
	for (i = 0; i < 3; i++) {
		cJSON_AddItemToArray(root, cJSON_CreateIntArray(numbers[i], 3));
	}

	/*cJSON_ReplaceItemInArray(root,1,cJSON_CreateString("Replacement")); */

	out = cJSON_Print(root);
	cJSON_Delete(root);
	printf("%s\n",out);
	free(out);


	/* Our "gallery" item: */
	root = cJSON_CreateObject();
	cJSON_AddItemToObject(root, "Image", img = cJSON_CreateObject());
	cJSON_AddNumberToObject(img, "Width", 800);
	cJSON_AddNumberToObject(img, "Height", 600);
	cJSON_AddStringToObject(img, "Title", "View from 15th Floor");
	cJSON_AddItemToObject(img, "Thumbnail", thm = cJSON_CreateObject());
	cJSON_AddStringToObject(thm, "Url", "http:/*www.example.com/image/481989943");
	cJSON_AddNumberToObject(thm, "Height", 125);
	cJSON_AddStringToObject(thm, "Width", "100");
	cJSON_AddItemToObject(img, "IDs", cJSON_CreateIntArray(ids, 4));

	out = cJSON_Print(root);
	cJSON_Delete(root);
	printf("%s\n", out);
	free(out);

	/* Our array of "records": */

	root = cJSON_CreateArray();
	for (i = 0; i < 2; i++) {
		cJSON_AddItemToArray(root, fld = cJSON_CreateObject());
		cJSON_AddStringToObject(fld, "precision", fields[i].precision);
		cJSON_AddNumberToObject(fld, "Latitude", fields[i].lat);
		cJSON_AddNumberToObject(fld, "Longitude", fields[i].lon);
		cJSON_AddStringToObject(fld, "Address", fields[i].address);
		cJSON_AddStringToObject(fld, "City", fields[i].city);
		cJSON_AddStringToObject(fld, "State", fields[i].state);
		cJSON_AddStringToObject(fld, "Zip", fields[i].zip);
		cJSON_AddStringToObject(fld, "Country", fields[i].country);
	}

	/*	cJSON_ReplaceItemInObject(cJSON_GetArrayItem(root,1),"City",cJSON_CreateIntArray(ids,4)); */

	out = cJSON_Print(root);
	cJSON_Delete(root);
	printf("%s\n", out);
	free(out);

	root = cJSON_CreateObject();
	cJSON_AddNumberToObject(root, "number", 1.0/0.0);
	out=cJSON_Print(root);
	cJSON_Delete(root);
	printf("%s\n", out);
	free(out);
}

int main (void) {
	/* a bunch of json: */
	char text1[] = "{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}";
	char text2[] = "[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]";
	char text3[] = "[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n	]\n";
	char text4[] = "{\n		\"Image\": {\n			\"Width\":  800,\n			\"Height\": 600,\n			\"Title\":  \"View from 15th Floor\",\n			\"Thumbnail\": {\n				\"Url\":    \"http:/*www.example.com/image/481989943\",\n				\"Height\": 125,\n				\"Width\":  \"100\"\n			},\n			\"IDs\": [116, 943, 234, 38793]\n		}\n	}";
	char text5[] = "[\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.7668,\n	 \"Longitude\": -122.3959,\n	 \"Address\":   \"\",\n	 \"City\":      \"SAN FRANCISCO\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94107\",\n	 \"Country\":   \"US\"\n	 },\n	 {\n	 \"precision\": \"zip\",\n	 \"Latitude\":  37.371991,\n	 \"Longitude\": -122.026020,\n	 \"Address\":   \"\",\n	 \"City\":      \"SUNNYVALE\",\n	 \"State\":     \"CA\",\n	 \"Zip\":       \"94085\",\n	 \"Country\":   \"US\"\n	 }\n	 ]";

	/* Process each json textblock by parsing, then rebuilding: */
	doit(text1);
	doit(text2);
	doit(text3);
	doit(text4);
	doit(text5);

	/* Parse standard testfiles: */
	dofile("test-data/test1");
	dofile("test-data/test2");
	dofile("test-data/test3");
	dofile("test-data/test4");
	dofile("test-data/test5");

	/* Now some samplecode for building objects concisely: */
	create_objects();

	return 0;
}
