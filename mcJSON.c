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

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "mcJSON.h"

#include <assert.h>

/* error pointer, points to the position
   in the input where the error happened */
static const char *ep;

const char *mcJSON_GetErrorPtr(void) {
	return ep;
}

static int mcJSON_strcasecmp(const char *s1,const char *s2) {
	if (s1 == NULL) {
		return (s1 == s2) ? 0 : 1;
	}
	if (s2 == NULL) {
		return 1;
	}
	for(; tolower(*s1) == tolower(*s2); ++s1, ++s2) {
		if(*s1 == 0) {
			return 0;
		}
	}
	return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*mcJSON_malloc)(size_t sz) = malloc;
static void (*mcJSON_free)(void *ptr) = free;

static char* mcJSON_strdup(const char* str) {
	size_t len;
	char* copy;

	len = strlen(str) + 1;
	if ((copy = (char*)mcJSON_malloc(len)) == NULL) {
		return NULL;
	}
	memcpy(copy, str, len);
	return copy;
}

void mcJSON_InitHooks(mcJSON_Hooks* hooks) {
	if (hooks == NULL) { /* Reset hooks */
		mcJSON_malloc = malloc;
		mcJSON_free = free;
		return;
	}

	mcJSON_malloc = (hooks->malloc_fn) ? hooks->malloc_fn : malloc;
	mcJSON_free = (hooks->free_fn) ? hooks->free_fn : free;
}

/* Internal constructor. */
static mcJSON *mcJSON_New_Item(void) {
	mcJSON* node = (mcJSON*)mcJSON_malloc(sizeof(mcJSON));
	if (node) memset(node, 0, sizeof(mcJSON));
	return node;
}

/* Delete a mcJSON structure. */
void mcJSON_Delete(mcJSON *c) {
	mcJSON *next;
	while (c) {
		next = c->next;
		if (!(c->type & mcJSON_IsReference) && c->child) {
			mcJSON_Delete(c->child);
		}
		if (!(c->type & mcJSON_IsReference) && (c->valuestring != NULL) && (c->valuestring->content != NULL)) {
			buffer_destroy_from_heap(c->valuestring);
		}
		if (!(c->type & mcJSON_StringIsConst) && (c->string != NULL) && (c->string->content != NULL)) {
			buffer_destroy_from_heap(c->string);
		}
		mcJSON_free(c);
		c = next;
	}
}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number(mcJSON *item, const char *num) {
	char *endptr;
	double number = strtod(num, &endptr);

	item->valuedouble = number;
	item->valueint = (int)number;
	item->type = mcJSON_Number;
	return endptr;
}

static size_t pow2gt(size_t x) {
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

/* ensure that the buffer is big enough */
static char* ensure(buffer_t *buffer, size_t needed) {
	if ((buffer == NULL) || (buffer->content == NULL)) {
		return NULL;
	}
	needed += buffer->position;

	/* necessary for now as the functions working with the buffer
	   don't update the length, TODO make this better */
	buffer->content_length = buffer->buffer_length;

	int status = buffer_grow_on_heap(buffer, pow2gt(needed));
	if (status != 0) {
		return NULL;
	}

	return (char*)buffer->content + buffer->position;
}

/* update the position in the buffer (by searching the terminating null character) */
static int update(buffer_t *buffer) {
	unsigned char *str;
	if ((buffer == NULL) || (buffer->content == NULL)) {
		return 0;
	}
	str = buffer->content + buffer->position;
	return buffer->position + strlen((char*)str);
}

/* Render the number nicely from the given item into a string. */
static char *print_number(mcJSON *item, buffer_t *buffer) {
	buffer_t *output = NULL;
	double d = item->valuedouble;
	if (d == 0) { /* zero */
		if (buffer != NULL) {
			ensure(buffer, 2);
			output = buffer;
		} else {
			output = buffer_create_on_heap(2, 2); /* special case for 0. */
		}
		if (output != NULL) {
			output->content_length = output->position + 1;
			if (buffer_copy_from_raw(output, output->position, (unsigned char*)"0", 0, 2) != 0) {
				if (buffer != NULL) {
					buffer_destroy_from_heap(output);
				}
				return NULL;
			}
		} else {
			return NULL;
		}
	} else if ((fabs(((double)item->valueint) - d) <= DBL_EPSILON) && (d <= INT_MAX) && (d >= INT_MIN)) {
		/* number is an integer */
		static const size_t INT_STRING_SIZE = 21; /* 2^64+1 can be represented in 21 chars. */
		if (buffer != NULL) {
			ensure(buffer, INT_STRING_SIZE);
			output = buffer;
		} else {
			output = buffer_create_on_heap(INT_STRING_SIZE, 0);
		}
		if (output != NULL) {
			snprintf((char*)output->content + output->position, output->buffer_length - output->position, "%d", item->valueint);
		} else {
			return NULL;
		}
	} else {
		static const size_t DOUBLE_STRING_SIZE = 64; /* This is a nice tradeoff. */
		if (buffer != NULL) {
			ensure(buffer, DOUBLE_STRING_SIZE);
			output = buffer;
		} else {
			output = buffer_create_on_heap(DOUBLE_STRING_SIZE, 0);
		}
		if (output != NULL) {
			if ((fpclassify(d) != FP_ZERO) && (!isnormal(d))) {
				output->content_length = output->position + 1;
				if (buffer_copy_from_raw(output, output->position, (unsigned char*)"null", 0, 5) != 0) {
					if (buffer != NULL) {
						buffer_destroy_from_heap(output);
					}
					return NULL;
				}
			} else if ((fabs(floor(d) - d) <= DBL_EPSILON) && (fabs(d) < 1.0e60)) {
				snprintf((char*)output->content + output->position, output->buffer_length - output->position, "%.0f", d);
			} else if ((fabs(d) < 1.0e-6) || (fabs(d) > 1.0e9)) {
				snprintf((char*)output->content + output->position, output->buffer_length - output->position, "%e", d);
			} else {
				snprintf((char*)output->content + output->position, output->buffer_length - output->position, "%f", d);
			}
		} else {
			return NULL;
		}
	}

	char *out = (char*)output->content + output->position;
	if (buffer == NULL) {
		free(output); //free buffer_t struct
	}
	return out;
}

//TODO does this work with both big and little endian?
static unsigned parse_hex4(const char *str) {
	unsigned h = 0;
	for (unsigned int i = 0; i < 3; i++) {
		if ((*str >= '0') && (*str <= '9')) {
			h += (*str) - '0';
		} else if ((*str >= 'A') && (*str<='F')) {
			h += 10 + (*str) - 'A';
		} else if ((*str >= 'a') && (*str <= 'f')) {
			h += 10 + (*str) - 'a';
		} else {
			return 0;
		}
		h = h << 4;
		str++;
	}

	if ((*str >= '0') && (*str <= '9')) {
		h += (*str) - '0';
	} else if ((*str >= 'A') && (*str<='F')) {
		h += 10 + (*str) - 'A';
	} else if ((*str >= 'a') && (*str <= 'f')) {
		h+= 10 + (*str) - 'a';
	} else {
		return 0;
	}
	return h;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
static const char *parse_string(mcJSON *item, const char *str) {
	const char *ptr = str + 1;
	int len = 0;
	unsigned uc;
	unsigned uc2;
	if (*str != '\"') { /* not a string! */
		ep = str;
		return NULL;
	}

	while ((*ptr != '\"') && *ptr && ++len) {
		if (*ptr++ == '\\') { /* Skip escaped quotes. */
			ptr++;
		}
	}
	const char * end_ptr = ptr;

	buffer_t *value_out = buffer_create_on_heap(len + 1, 0); /* This is how long we need for the string, roughly. */
	if (value_out == NULL) {
		return NULL;
	}

	ptr = str + 1;
	value_out->position = 0;
	while ((*ptr != '\"') && *ptr && (ptr <= end_ptr)) {
		if (*ptr != '\\') {
			value_out->content[value_out->position] = *ptr;
			ptr++;
			value_out->position++;
		} else {
			ptr++;
			switch (*ptr)
			{
				case 'b':
					value_out->content[value_out->position] = '\b';
					value_out->position++;
					break;
				case 'f':
					value_out->content[value_out->position] = '\f';
					value_out->position++;
					break;
				case 'n':
					value_out->content[value_out->position] = '\n';
					value_out->position++;
					break;
				case 'r':
					value_out->content[value_out->position] = '\r';
					value_out->position++;
					break;
				case 't':
					value_out->content[value_out->position] = '\t';
					value_out->position++;
					break;
				case 'u': /* transcode utf16 to utf8. */
					uc = parse_hex4(ptr + 1) /* get the unicode char. */;
					ptr += 4;

					if (((uc >= 0xDC00) && (uc <= 0xDFFF)) || (uc == 0)) { /* check for invalid. */
						break;
					}

					if ((uc >= 0xD800) && (uc<=0xDBFF)) { /* UTF16 surrogate pairs. */
						if ((ptr[1] != '\\') || (ptr[2] != 'u')) { /* missing second-half of surrogate. */
							break;
						}
						uc2 = parse_hex4(ptr + 3);
						ptr += 6;
						if ((uc2 < 0xDC00) || (uc2 > 0xDFFF)) { /* invalid second-half of surrogate. */
							break;
						}
						uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
					}

					len = 4;
					if (uc < 0x80) {
						len = 1;
					} else if (uc < 0x800) {
						len = 2;
					} else if (uc < 0x10000) {
						len = 3;
						value_out->position += len;
					}

					switch (len) {
						case 4:
							value_out->position--;
							value_out->content[value_out->position] = ((uc | 0x80) & 0xBF);
							uc >>= 6;
						case 3:
							value_out->position--;
							value_out->content[value_out->position] = ((uc | 0x80) & 0xBF);
							uc >>= 6;
						case 2:
							value_out->position--;
							value_out->content[value_out->position] = ((uc | 0x80) & 0xBF);
							uc >>= 6;
						case 1:
							value_out->position--;
							value_out->content[value_out->position] = (uc | firstByteMark[len]);
					}
					value_out->position += len;
					break;
				default:
					value_out->content[value_out->position] = *ptr;
					value_out->position++;
					break;
			}
			ptr++;
		}
	}
	/* null terminate the output string */
	value_out->content[value_out->position] = '\0';
	value_out->content_length = value_out->position + 1;
	if (*ptr == '\"') {
		ptr++;
	}
	item->valuestring = value_out;
	item->type = mcJSON_String;
	return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str, buffer_t *buffer) {
	size_t start_position = 0; /* position relative to output buffer where the output starts */

	buffer_t *output = NULL;
	buffer_t *string = NULL;
	if (str == NULL) {
		string = buffer_create_with_existing_array(NULL, 0);
	} else {
		string = buffer_create_with_existing_array((unsigned char*) str, strlen(str));
	}

	/* empty string */
	if (string->content_length == 0) {
		if (buffer != NULL) {
			start_position = buffer->position;
			if (ensure(buffer, 3) == NULL) {
				buffer->content[buffer->position] = '\0';
				return NULL;
			}
			output = buffer;
		} else {
			output = buffer_create_on_heap(3, 3);
		}
		if ((output == NULL) || (output->content == NULL)) {
			return NULL;
		}

		/* fill with empty string */
		output->content_length = output->position + 1;
		if (buffer_copy_from_raw(output, output->position, (unsigned char*)"\"\"", 0, 3) != 0) {
			if (buffer == NULL) {
				buffer_destroy_from_heap(output);
			}
			return NULL;
		}

		char *out = (char*) output->content + start_position;
		if (buffer == NULL) { /* unbuffered */
			free(output); /* free the buffer_t struct */
		}
		return out;
	}

	/* get the number of additional characters needed to encode special characters */
	size_t additional_characters = 0; /* number of additional chars needed for escaping */
	for (string->position = 0; string->position < string->content_length; string->position++) {
		if (strchr("\"\\\b\f\n\r\t", string->content[string->position])) {
			/* additional space for '\\' needed */
			additional_characters++;
		} else if (string->content[string->position] < 32) {
			/* "\\uXXXX" -> 5 additional characters */
			additional_characters += 5;
		}
	}

	/* allocate output */
	if (buffer != NULL) { /* buffered */
		start_position = buffer->position;
		if (ensure(buffer, string->content_length + additional_characters + 3) == NULL) {
			buffer->content[buffer->position] = '\0';
			return NULL;
		}
		output = buffer;
	} else { /* unbuffered */
		output = buffer_create_on_heap(string->content_length + additional_characters + 3, string->content_length + additional_characters + 3);
	}
	if ((output == NULL) || (output->content == NULL)) {
		return NULL;
	}

	/* no special characters */
	if (additional_characters == 0) {
		/* start double quotes */
		output->content[output->position] = '\"';
		output->position++;

		/* copy the content */
		output->content_length = output->position + 1;
		if ((buffer_copy(output, output->position, string, 0, string->content_length)) != 0) {
			output->content[output->position] = '\0';
			if (buffer == NULL) {
				buffer_destroy_from_heap(output);
			} else {
				buffer->position = start_position;
			}
			return NULL;
		}
		output->position += string->content_length;

		/* end double quotes and terminate with '\0' */
		output->content[output->position] = '\"';
		output->position++;
		output->content[output->position] = '\0';
		output->content_length = output->position + 1;

		char *out = (char*) output->content + start_position;
		if (buffer == NULL) { /* unbuffered */
			free(output); /* free the buffer_t struct */
		}
		return out;
	}

	/* start output with double quote */
	output->content[output->position] = '\"';
	output->position++;
	for (string->position = 0; (string->position < string->content_length) && (output->position < output->buffer_length); string->position++, output->position++) {
		if ((string->content[string->position] > 31)
				&& (string->content[string->position] != '\"')
				&& (string->content[string->position] != '\\')) {
			/* normal characters, just print it to the output */
			output->content[output->position] = string->content[string->position];
		} else {
			/* special characters that need to be escaped */
			output->content[output->position] = '\\';
			output->position++;

			/* check for out of bounds in the output */
			if ((output->position + 1) > output->buffer_length) {
				output->content[output->position] = '\0';
				if (buffer == NULL) {
					buffer_destroy_from_heap(output);
				} else {
					buffer->position = start_position;
				}
				return NULL;
			}

			switch (string->content[string->position]) {
				case '\\':
					output->content[output->position] = '\\';
					break;
				case '\"':
					output->content[output->position] = '\"';
					break;
				case '\b':
					output->content[output->position] = 'b';
					break;
				case '\f':
					output->content[output->position] = 'f';
					break;
				case '\n':
					output->content[output->position] = 'n';
					break;
				case '\r':
					output->content[output->position] = 'r';
					break;
				case '\t':
					output->content[output->position] = 't';
					break;
				default: /* escape and print */
					if ((output->position + 6) > output->buffer_length) {
						output->content[output->position] = '\0';
						if (buffer == NULL) {
							buffer_destroy_from_heap(output);
						} else {
							buffer->position = start_position;
						}
						return NULL;
					}
					snprintf((char*)output->content + output->position, 6, "u%04x", string->content[string->position]);
					output->position += 4; /* not +5 because the loop does this for us. */
					break;
			}
		}
	}

	/* add closing double quote and terminating '\0' */
	if ((output->position + 2) > output->buffer_length) {
		output->content[output->position] = '\0';
		if (buffer == NULL) {
			buffer_destroy_from_heap(output);
		} else {
			buffer->position = start_position;
		}
		return NULL;
	}
	output->content[output->position] = '\"';
	output->position++;
	output->content[output->position] = '\0';
	output->content_length = output->position + 1;

	char *out = (char*) output->content + start_position;
	if (buffer == NULL) { /* unbuffered */
		free(output); /* free the buffer_t struct */
	}

	return out;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static char *print_string(mcJSON *item, buffer_t *buffer) {
	return print_string_ptr((char*)item->valuestring->content, buffer);
}

/* Predeclare these prototypes. */
static const char *parse_value(mcJSON *item, const char *value);
static char *print_value(mcJSON *item, int depth, int fmt, buffer_t *buffer);
static const char *parse_array(mcJSON *item, const char *value);
static char *print_array(mcJSON *item, int depth, int fmt, buffer_t *buffer);
static const char *parse_object(mcJSON *item, const char *value);
static char *print_object(mcJSON *item, int depth, int fmt, buffer_t *buffer);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in) {
	while (in && *in && ((unsigned char)*in <= 32)) {
		in++;
	}
	return in;
}

/* Parse an object - create a new root, and populate. */
mcJSON *mcJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated) {
	const char *end = NULL;
	mcJSON *c = mcJSON_New_Item();
	ep = NULL;
	if (c == NULL) { /* memory fail */
		return NULL;
	}

	end = parse_value(c, skip(value));
	if (end == NULL) { /* parse failure. ep is set. */
		mcJSON_Delete(c);
		return NULL;
	}

	/* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
	if (require_null_terminated) {
		end = skip(end);
		if (*end) {
			mcJSON_Delete(c);
			ep = end;
			return NULL;
		}
	}
	if (return_parse_end) {
		*return_parse_end = end;
	}
	return c;
}
/* Default options for mcJSON_Parse */
mcJSON *mcJSON_Parse(const char *value) {
	return mcJSON_ParseWithOpts(value, 0, 0);
}

/* Render a mcJSON item/entity/structure to text. */
char *mcJSON_Print(mcJSON *item) {
	return print_value(item, 0, 1, NULL);
}
char *mcJSON_PrintUnformatted(mcJSON *item) {
	return print_value(item, 0, 0, NULL);
}

char *mcJSON_PrintBuffered(mcJSON *item, const size_t prebuffer, int fmt) {
	buffer_t *buffer = buffer_create_on_heap(prebuffer, prebuffer);
	char *output = print_value(item, 0, fmt, buffer);
	free(buffer); //free the buffer_t struct
	return output;
}


/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(mcJSON *item, const char *value) {
	if (value == NULL) { /* Fail on null. */
		return NULL;
	}
	if (!strncmp(value, "null", 4)) {
		item->type = mcJSON_NULL;
		return value + 4;
	}
	if (!strncmp(value, "false", 5)) {
		item->type = mcJSON_False;
		return value + 5;
	}
	if (!strncmp(value, "true", 4)) {
		item->type = mcJSON_True;
		item->valueint = 1;
		return value + 4;
	}
	if (*value == '\"') {
		return parse_string(item,value);
	}
	if ((*value == '-') || ((*value >= '0') && (*value <= '9'))) {
		return parse_number(item, value);
	}
	if (*value == '[') {
		return parse_array(item, value);
	}
	if (*value == '{') {
		return parse_object(item, value);
	}

	ep = value;
	return NULL; /* failure. */
}

/* Render a value to text. */
static char *print_value(mcJSON *item, int depth, int fmt, buffer_t *buffer) {
	char *out = NULL;
	if (!item) {
		return NULL;
	}
	if (buffer != NULL) { /* buffered printing */
		switch ((item->type) & 255) {
			case mcJSON_NULL:
				out = ensure(buffer, 5);
				if (out) {
					strncpy(out, "null", 5);
				}
				break;
			case mcJSON_False:
				out = ensure(buffer, 6);
				if (out) {
					strncpy(out, "false", 6);
				}
				break;
			case mcJSON_True:
				out = ensure(buffer, 5);
				if (out) {
					strncpy(out, "true", 5);
				}
				break;
			case mcJSON_Number:
				out = print_number(item, buffer);
				break;
			case mcJSON_String:
				out = print_string(item, buffer);
				break;
			case mcJSON_Array:
				out = print_array(item, depth, fmt, buffer);
				break;
			case mcJSON_Object:
				out = print_object(item, depth, fmt, buffer);
				break;
		}
	} else { /* non buffered printing */
		switch ((item->type) & 255) {
			case mcJSON_NULL:
				out = mcJSON_strdup("null");
				break;
			case mcJSON_False:
				out = mcJSON_strdup("false");
				break;
			case mcJSON_True:
				out = mcJSON_strdup("true");
				break;
			case mcJSON_Number:
				out = print_number(item, NULL);
				break;
			case mcJSON_String:
				out = print_string(item, NULL);
				break;
			case mcJSON_Array:
				out = print_array(item, depth, fmt, NULL);
				break;
			case mcJSON_Object:
				out = print_object(item, depth, fmt, NULL);
				break;
		}
	}
	return out;
}

/* Build an array from input text. */
static const char *parse_array(mcJSON *item, const char *value) {
	mcJSON *child;
	if (*value != '[') { /* not an array! */
		ep = value;
		return NULL;
	}

	item->type = mcJSON_Array;
	value = skip(value + 1);
	if (*value == ']') { /* empty array. */
		return value + 1;
	}

	child = mcJSON_New_Item();
	item->child = child;
	if (item->child == NULL) { /* memory fail */
		return NULL;
	}
	value = skip(parse_value(child, skip(value))); /* skip any spacing, get the value. */
	if (value == NULL) {
		return NULL;
	}

	while (*value == ',') {
		mcJSON *new_item;
		if ((new_item = mcJSON_New_Item()) == NULL) { /* memory fail */
			return NULL;
		}
		child->next = new_item;
		new_item->prev = child;
		child = new_item;
		value = skip(parse_value(child, skip(value + 1)));
		if (value == NULL) { /* memory fail */
			return NULL;
		}
	}

	if (*value == ']') { /* end of array */
		return value + 1;
	}
	ep = value;
	return NULL; /* malformed. */
}

/* Render an array to text */
static char *print_array(mcJSON *item, int depth, int fmt, buffer_t *buffer) {
	char **entries;
	char *out=0;
	char *ptr;
	char *ret;
	int len = 5;
	mcJSON *child = item->child;
	int numentries = 0;
	int i = 0;
	bool fail = false;
	size_t tmplen = 0;

	/* How many entries in the array? */
	while (child) {
		numentries++;
		child = child->next;
	}
	/* Explicitly handle numentries==0 */
	if (numentries == 0) {
		if (buffer != NULL) {
			out = ensure(buffer, 3);
		} else {
			out = (char*)mcJSON_malloc(3);
		}
		if (out) {
			strncpy(out, "[]", 3);
		}
		return out;
	}

	if (buffer != NULL) {
		/* Compose the output array. */
		i = buffer->position;
		ptr = ensure(buffer, 1);
		if (ptr == NULL) {
			return NULL;
		}
		*ptr = '[';
		buffer->position++;
		child = item->child;
		while (child && !fail) {
			print_value(child, depth + 1, fmt, buffer);
			buffer->position = update(buffer);
			if (child->next) {
				len = fmt ? 2 : 1;
				ptr = ensure(buffer, len + 1);
				if (ptr == NULL) {
					return NULL;
				}
				*ptr++ = ',';
				if (fmt) {
					*ptr++ = ' ';
				}
				*ptr = '\0';
				buffer->position += len;
			}
			child = child->next;
		}
		ptr = ensure(buffer, 2);
		if (ptr == NULL) {
			return NULL;
		}
		*ptr++ = ']';
		*ptr = '\0';
		out = (char*) (buffer->content) + i;
	} else {
		/* Allocate an array to hold the values for each */
		entries = (char**)mcJSON_malloc(numentries * sizeof(char*));
		if (entries == NULL) {
			return NULL;
		}
		memset(entries, 0, numentries * sizeof(char*));
		/* Retrieve all the results: */
		child = item->child;
		while (child && !fail) {
			ret = print_value(child, depth + 1, fmt, 0);
			entries[i++] = ret;
			if (ret) {
				len += strlen(ret) + 2 + (fmt ? 1 : 0);
			} else {
				fail = true;
			}
			child = child->next;
		}

		/* If we didn't fail, try to malloc the output string */
		if (!fail) {
			out = (char*)mcJSON_malloc(len);
		}
		/* If that fails, we fail. */
		if (out == NULL) {
			fail = true;
		}

		/* Handle failure. */
		if (fail) {
			for (i = 0; i < numentries; i++) {
				if (entries[i]) {
					mcJSON_free(entries[i]);
				}
			}
			mcJSON_free(entries);
			return NULL;
		}

		/* Compose the output array. */
		*out = '[';
		ptr = out + 1;
		*ptr = '\0';
		for (i = 0; i < numentries; i++) {
			tmplen = strlen(entries[i]);
			memcpy(ptr, entries[i], tmplen);
			ptr += tmplen;
			if (i != (numentries - 1)) {
				*ptr++ = ',';
				if (fmt) {
					*ptr++ = ' ';
				}
				*ptr = '\0';
			}
			mcJSON_free(entries[i]);
		}
		mcJSON_free(entries);
		*ptr++ = ']';
		*ptr++ = '\0';
	}
	return out;
}

/* Build an object from the text. */
static const char *parse_object(mcJSON *item, const char *value) {
	mcJSON *child;
	if (*value != '{') { /* not an object! */
		ep = value;
		return NULL;
	}

	item->type = mcJSON_Object;
	value = skip(value + 1);
	if (*value=='}') { /* empty object. */
		return value + 1;
	}

	child = mcJSON_New_Item();
	item->child = child;
	if (item->child == NULL) {
		return NULL;
	}
	value = skip(parse_string(child, skip(value)));
	if (value == NULL) {
		return NULL;
	}
	child->string = child->valuestring;
	child->valuestring = NULL;
	if (*value != ':') { /* fail! */
		ep = value;
		return NULL;
	}
	value = skip(parse_value(child, skip(value + 1))); /* skip any spacing, get the value. */
	if (value == NULL) {
		return NULL;
	}

	while (*value == ',') {
		mcJSON *new_item;
		if ((new_item = mcJSON_New_Item()) == NULL) { /* memory fail */
			return NULL;
		}
		child->next = new_item;
		new_item->prev = child;
		child = new_item;
		value = skip(parse_string(child, skip(value + 1)));
		if (value == NULL) {
			return NULL;
		}
		child->string = child->valuestring;
		child->valuestring = NULL;
		if (*value != ':') { /* fail! */
			ep = value;
			return NULL;
		}
		value = skip(parse_value(child, skip(value + 1))); /* skip any spacing, get the value. */
		if (value == NULL) {
			return NULL;
		}
	}

	if (*value == '}') { /* end of object */
		return value + 1;
	}
	ep = value;
	return NULL; /* malformed. */
}

/* Render an object to text. */
static char *print_object(mcJSON *item, int depth, int fmt, buffer_t *buffer) {
	char **entries = NULL;
	char **names = NULL;
	char *out = NULL;
	char *ptr;
	char *ret;
	char *str;
	int len = 7;
	int i = 0;
	int j;
	mcJSON *child = item->child;
	int numentries = 0;
	bool fail = false;
	size_t tmplen = 0;
	/* Count the number of entries. */
	while (child) {
		numentries++;
		child = child->next;
	}
	/* Explicitly handle empty object case */
	if (numentries == 0) {
		if (buffer != NULL) {
			out = ensure(buffer, fmt ? depth + 4 : 3);
		} else {
			out = (char*)mcJSON_malloc(fmt ? depth + 4 : 3);
		}
		if (out == NULL) {
			return NULL;
		}
		ptr = out;
		*ptr++ = '{';
		if (fmt) {
			*ptr++ = '\n';
			for (i = 0; i < depth; i++) {
				*ptr++ = '\t';
			}
		}
		*ptr++ = '}';
		*ptr++ = '\0';
		return out;
	}
	if (buffer != NULL) {
		/* Compose the output: */
		i = buffer->position;
		len = fmt ? 2 : 1;
		ptr = ensure(buffer, len + 1);
		if (ptr == NULL) {
			return NULL;
		}
		*ptr++ = '{';
		if (fmt) {
			*ptr++ = '\n';
		}
		*ptr = '\0';
		buffer->position += len;
		child = item->child;
		depth++;
		while (child) {
			if (fmt) {
				ptr = ensure(buffer, depth);
				if (ptr == NULL) {
					return NULL;
				}
				for (j = 0; j < depth; j++) {
					*ptr++='\t';
				}
				buffer->position += depth;
			}
			print_string_ptr((char*)child->string->content, buffer);
			buffer->position = update(buffer);

			len = fmt ? 2 : 1;
			ptr = ensure(buffer, len);
			if (ptr == NULL) {
				return NULL;
			}
			*ptr++ = ':';
			if (fmt) {
				*ptr++ = '\t';
			}
			buffer->position+= len;

			print_value(child, depth, fmt, buffer);
			buffer->position= update(buffer);

			len = (fmt ? 1 : 0) + (child->next ? 1 : 0);
			ptr = ensure(buffer, len + 1);
			if (ptr == NULL) {
				return NULL;
			}
			if (child->next) {
				*ptr++ = ',';
			}
			if (fmt) {
				*ptr++ = '\n';
			}
			*ptr = '\0';
			buffer->position += len;
			child = child->next;
		}
		ptr = ensure(buffer, fmt ? (depth + 1) : 2);
		if (ptr == NULL) {
			return NULL;
		}
		if (fmt) {
			for (i = 0; i < depth - 1; i++) {
				*ptr++ = '\t';
			}
		}
		*ptr++ = '}';
		*ptr = '\0';
		out = (char*) (buffer->content) + i;
	} else {
		/* Allocate space for the names and the objects */
		entries = (char**)mcJSON_malloc(numentries * sizeof(char*));
		if (entries == NULL) {
			return NULL;
		}
		names = (char**)mcJSON_malloc(numentries * sizeof(char*));
		if (names == NULL) {
			mcJSON_free(entries);
			return NULL;
		}
		memset(entries, 0, sizeof(char*) * numentries);
		memset(names, 0, sizeof(char*) * numentries);

		/* Collect all the results into our arrays: */
		child = item->child;
		depth++;
		if (fmt) {
			len += depth;
		}
		while (child) {
			str = print_string_ptr((char*)child->string->content, 0);
			names[i] = str;
			ret = print_value(child, depth, fmt, 0);
			entries[i++] = ret;
			if (str && ret) {
				len += strlen(ret) + strlen(str) + 2 + (fmt ? 2 + depth : 0);
			} else {
				fail = true;
			}
			child = child->next;
		}

		/* Try to allocate the output string */
		if (!fail) {
			out = (char*)mcJSON_malloc(len);
		}
		if (out == NULL) {
			fail = true;
		}

		/* Handle failure */
		if (fail) {
			for (i = 0; i < numentries; i++) {
				if (names[i]) {
					mcJSON_free(names[i]);
				}
				if (entries[i]) {
					mcJSON_free(entries[i]);
				}
			}
			mcJSON_free(names);
			mcJSON_free(entries);
			return NULL;
		}

		/* Compose the output: */
		*out = '{';
		ptr = out + 1;
		if (fmt) {
			*ptr++ = '\n';
		}
		*ptr = '\0';
		for (i = 0; i < numentries; i++) {
			if (fmt) {
				for (j = 0;j < depth; j++) {
					*ptr++ = '\t';
				}
			}
			tmplen = strlen(names[i]);
			memcpy(ptr, names[i], tmplen);
			ptr += tmplen;
			*ptr++ = ':';
			if (fmt) {
				*ptr++ = '\t';
			}
			strcpy(ptr, entries[i]);
			ptr += strlen(entries[i]);
			if (i != (numentries - 1)) {
				*ptr++ = ',';
			}
			if (fmt) {
				*ptr++ = '\n';
			}
			*ptr = '\0';
			mcJSON_free(names[i]);
			mcJSON_free(entries[i]);
		}

		mcJSON_free(names);
		mcJSON_free(entries);
		if (fmt) {
			for (i = 0; i < depth - 1; i++) {
				*ptr++ = '\t';
			}
		}
		*ptr++ = '}';
		*ptr++ = '\0';
	}
	return out;
}

/* Get Array size/item / object item. */
int mcJSON_GetArraySize(mcJSON *array) {
	mcJSON *c = array->child;
	int i = 0;
	while (c) {
		i++;
		c = c->next;
	}
	return i;
}
mcJSON *mcJSON_GetArrayItem(mcJSON *array, int item) {
	mcJSON *c = array->child;
	while (c && (item>0)) {
		item--;
		c = c->next;
	}
	return c;
}
mcJSON *mcJSON_GetObjectItem(mcJSON *object, const char *string) {
	mcJSON *c = object->child;
	while (c && mcJSON_strcasecmp((char*)c->string->content, string)) {
		c = c->next;
	}
	return c;
}

/* Utility for array list handling. */
static void suffix_object(mcJSON *prev, mcJSON *item) {
	prev->next = item;
	item->prev = prev;
}
/* Utility for handling references. */
static mcJSON *create_reference(mcJSON *item) {
	mcJSON *ref = mcJSON_New_Item();
	if (ref == NULL) {
		return NULL;
	}
	memcpy(ref, item, sizeof(mcJSON));
	ref->string = NULL;
	ref->type |= mcJSON_IsReference;
	ref->next = ref->prev = NULL;
	return ref;
}

/* Add item to array/object. */
void mcJSON_AddItemToArray(mcJSON *array, mcJSON *item) {
	mcJSON *c = array->child;
	if (item == NULL) {
		return;
	}
	if (c == NULL) {
		array->child = item;
	} else {
		while (c && c->next) {
			c = c->next;
		}
		suffix_object(c, item);
	}
}
void mcJSON_AddItemToObject(mcJSON *object, const char *string, mcJSON *item) {
	if (item == NULL) {
		return;
	}
	if ((item->string != NULL) && (item->string->content != NULL)) {
		buffer_destroy_from_heap(item->string);
	}
	size_t length = strlen(string) + 1;
	item->string = buffer_create_on_heap(length, length);
	int status = buffer_clone_from_raw(item->string, (unsigned char*)string, length);
	if (status != 0) {
		//TODO proper error handling
		return;
	}
	mcJSON_AddItemToArray(object, item);
}
void mcJSON_AddItemToObjectCS(mcJSON *object, const char *string, mcJSON *item) {
	if (item == NULL) {
		return;
	}
	if (!(item->type & mcJSON_StringIsConst) && (item->string != NULL) && (item->string->content != NULL)) {
		buffer_destroy_from_heap(item->string);
	}

	size_t length = strlen(string) + 1;
	item->string = buffer_create_on_heap(length, length);
	int status = buffer_clone_from_raw(item->string, (unsigned char*)string, length);
	if (status != 0) {
		//TODO proper error handling
		return;
	}
	item->type |= mcJSON_StringIsConst;
	mcJSON_AddItemToArray(object, item);
}
void mcJSON_AddItemReferenceToArray(mcJSON *array, mcJSON *item) {
	mcJSON_AddItemToArray(array,create_reference(item));
}
void mcJSON_AddItemReferenceToObject(mcJSON *object, const char *string, mcJSON *item) {
	mcJSON_AddItemToObject(object, string, create_reference(item));
}

mcJSON *mcJSON_DetachItemFromArray(mcJSON *array, int which) {
	mcJSON *c = array->child;
	while (c && (which>0)) {
		c = c->next;
		which--;
	}
	if (c == NULL) {
		return NULL;
	}
	if (c->prev) {
		c->prev->next = c->next;
	}
	if (c->next) {
		c->next->prev = c->prev;
	}
	if (c == array->child) {
		array->child = c->next;
	}
	c->next = NULL;
	c->prev = NULL;
	return c;
}
void mcJSON_DeleteItemFromArray(mcJSON *array, int which) {
	mcJSON_Delete(mcJSON_DetachItemFromArray(array, which));
}
mcJSON *mcJSON_DetachItemFromObject(mcJSON *object, const char *string) {
	int i = 0;
	mcJSON *c = object->child;
	while (c && mcJSON_strcasecmp((char*)c->string->content, string)) {
		i++;
		c = c->next;
	}
	if (c) {
		return mcJSON_DetachItemFromArray(object, i);
	}
	return NULL;
}
void   mcJSON_DeleteItemFromObject(mcJSON *object, const char *string) {
	mcJSON_Delete(mcJSON_DetachItemFromObject(object, string));
}

/* Replace array/object items with new ones. */
void   mcJSON_InsertItemInArray(mcJSON *array, int which, mcJSON *newitem) {
	mcJSON *c = array->child;
	while (c && (which > 0)) {
		c = c->next;
		which--;
	}
	if (c == NULL) {
		mcJSON_AddItemToArray(array, newitem);
		return;
	}
	newitem->next = c;
	newitem->prev = c->prev;
	c->prev = newitem;
	if (c == array->child) {
		array->child = newitem;
	} else {
		newitem->prev->next = newitem;
	}
}
void   mcJSON_ReplaceItemInArray(mcJSON *array, int which, mcJSON *newitem) {
	mcJSON *c = array->child;
	while (c && (which>0)) {
		c = c->next;
		which--;
	}
	if (c == NULL) {
		return;
	}
	newitem->next = c->next;
	newitem->prev = c->prev;
	if (newitem->next) {
		newitem->next->prev = newitem;
	}
	if (c == array->child) {
		array->child = newitem;
	} else {
		newitem->prev->next = newitem;
	}
	c->prev = NULL;
	c->next = NULL;
	mcJSON_Delete(c);
}
void mcJSON_ReplaceItemInObject(mcJSON *object, const char *string, mcJSON *newitem) {
	int i = 0;
	mcJSON *c = object->child;
	while (c && mcJSON_strcasecmp((char*)c->string->content, string)) {
		i++;
		c = c->next;
	}
	if (c) {
		size_t length = strlen(string) + 1;
		newitem->string = buffer_create_on_heap(length, length);
		int status = buffer_clone_from_raw(newitem->string, (unsigned char*)string, length);
		if (status != 0) {
			//TODO proper error handling
			return;
		}
		mcJSON_ReplaceItemInArray(object, i, newitem);
	}
}

/* Create basic types: */
mcJSON *mcJSON_CreateNull(void) {
	mcJSON *item = mcJSON_New_Item();
	if (item) {
		item->type = mcJSON_NULL;
	}
	return item;
}
mcJSON *mcJSON_CreateTrue(void) {
	mcJSON *item = mcJSON_New_Item();
	if (item) {
		item->type = mcJSON_True;
	}
	return item;
}
mcJSON *mcJSON_CreateFalse(void) {
	mcJSON *item = mcJSON_New_Item();
	if (item) {
		item->type = mcJSON_False;
	}
	return item;
}
mcJSON *mcJSON_CreateBool(int b) {
	mcJSON *item = mcJSON_New_Item();
	if (item) {
		item->type = b ? mcJSON_True : mcJSON_False;
	}
	return item;
}
mcJSON *mcJSON_CreateNumber(double num) {
	mcJSON *item = mcJSON_New_Item();
	if (item) {
		item->type = mcJSON_Number;
		item->valuedouble = num;
		item->valueint = (int)num;
	}
	return item;
}
mcJSON *mcJSON_CreateString(const char *string) {
	mcJSON *item = mcJSON_New_Item();
	if (item) {
		item->type = mcJSON_String;
		size_t length = strlen(string) + 1;
		item->valuestring = buffer_create_on_heap(length, length);
		int status = buffer_clone_from_raw(item->valuestring, (unsigned char*)string, length);
		if (status != 0) {
			mcJSON_Delete(item);
			return NULL;
		}
	}
	return item;
}
mcJSON *mcJSON_CreateArray(void) {
	mcJSON *item = mcJSON_New_Item();
	if (item) {
		item->type = mcJSON_Array;
	}
	return item;
}
mcJSON *mcJSON_CreateObject(void) {
	mcJSON *item = mcJSON_New_Item();
	if (item) {
		item->type = mcJSON_Object;
	}
	return item;
}

/* Create Arrays: */
mcJSON *mcJSON_CreateIntArray(const int *numbers, int count) {
	int i;
	mcJSON *n = NULL;
	mcJSON *p = NULL;
	mcJSON *a = mcJSON_CreateArray();
	for (i = 0; a && (i < count); i++) {
		n = mcJSON_CreateNumber(numbers[i]);
		if(i == 0) {
			a->child = n;
		} else {
			suffix_object(p, n);
		}
		p = n;
	}
	return a;
}
mcJSON *mcJSON_CreateFloatArray(const float *numbers, int count) {
	int i;
	mcJSON *n = NULL;
	mcJSON *p = NULL;
	mcJSON *a = mcJSON_CreateArray();
	for (i = 0; a && (i < count); i++) {
		n = mcJSON_CreateNumber(numbers[i]);
		if (i == 0) {
			a->child = n;
		} else {
			suffix_object(p, n);
		}
		p = n;
	}
	return a;
}
mcJSON *mcJSON_CreateDoubleArray(const double *numbers, int count) {
	int i;
	mcJSON *n = NULL;
	mcJSON *p = NULL;
	mcJSON *a = mcJSON_CreateArray();
	for (i = 0; a && (i < count); i++) {
		n = mcJSON_CreateNumber(numbers[i]);
		if (i == 0) {
			a->child = n;
		} else {
			suffix_object(p,n);
		}
		p = n;
	}
	return a;
}
mcJSON *mcJSON_CreateStringArray(const char **strings, int count) {
	int i;
	mcJSON *n = NULL;
	mcJSON *p = NULL;
	mcJSON *a = mcJSON_CreateArray();
	for(i = 0; a && (i < count); i++) {
		n = mcJSON_CreateString(strings[i]);
		if (i == 0) {
			a->child = n;
		} else {
			suffix_object(p, n);
		}
		p = n;
	}
	return a;
}

/* Duplication */
mcJSON *mcJSON_Duplicate(mcJSON *item, int recurse) {
	mcJSON *newitem;
	mcJSON *cptr;
	mcJSON *nptr = NULL;
	mcJSON *newchild;
	/* Bail on bad ptr */
	if (item == NULL) {
		return NULL;
	}
	/* Create new item */
	newitem = mcJSON_New_Item();
	if (newitem == NULL) {
		return NULL;
	}
	/* Copy over all vars */
	newitem->type = item->type & (~mcJSON_IsReference);
	newitem->valueint = item->valueint;
	newitem->valuedouble = item->valuedouble;
	if ((item->valuestring != NULL) && (item->valuestring->content != NULL)) {
		newitem->valuestring = buffer_create_on_heap(item->valuestring->buffer_length, item->valuestring->buffer_length);
		int status = buffer_clone(newitem->valuestring, item->valuestring);
		if (status != 0) {
			mcJSON_Delete(newitem);
			return NULL;
		}
	}
	if ((item->string != NULL) && (item->string->content != NULL)) {
		newitem->string = buffer_create_on_heap(item->string->buffer_length, item->string->buffer_length);
		int status = buffer_clone(newitem->string, item->string);
		if (status != 0) {
			mcJSON_Delete(newitem);
			return NULL;
		}
	}
	/* If non-recursive, then we're done! */
	if (!recurse) {
		return newitem;
	}
	/* Walk the ->next chain for the child. */
	cptr = item->child;
	while (cptr) {
		newchild = mcJSON_Duplicate(cptr, 1); /* Duplicate (with recurse) each item in the ->next chain */
		if (newchild == NULL) {
			mcJSON_Delete(newitem);
			return NULL;
		}
		if (nptr) { /* If newitem->child already set, then crosswire ->prev and ->next and move on */
			nptr->next = newchild;
			newchild->prev = nptr;
			nptr = newchild;
		} else { /* Set newitem->child and move to it */
			newitem->child = newchild;
			nptr = newchild;
		}
		cptr = cptr->next;
	}
	return newitem;
}

void mcJSON_Minify(char *json) {
	char *into = json;
	while (*json) {
		/* Whitespace characters. */
		if (*json == ' ') {
			json++;
		} else if (*json == '\t') {
			json++;
		} else if (*json == '\r') {
			json++;
		} else if (*json == '\n') {
			json++;
		} else if ((*json == '/') && (json[1]=='/')) { /* double-slash comments, to end of line. */
			while (*json && (*json!='\n')) {
				json++;
			}
		} else if ((*json == '/') && (json[1] == '*')) { /* multiline comments. */
			while (*json && !((*json == '*') && (json[1] == '/'))) {
				json++;
			}
			json += 2;
		} else if (*json == '\"') { /* string literals, which are \" sensitive. */
			*into++ = *json++;
			while (*json && (*json != '\"')) {
				if (*json=='\\') {
					*into++ = *json++;
				}
				*into++ = *json++;
			}
			*into++ = *json++;
		} else { /* All other characters. */
			*into++ = *json++;
		}
	}
	*into = '\0'; /* and null-terminate. */
}
