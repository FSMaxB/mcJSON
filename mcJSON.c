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
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "mcJSON.h"

#include <assert.h>

static void *(*mcJSON_malloc)(size_t sz) = malloc;
static void (*mcJSON_free)(void *ptr) = free;

void *allocate(const size_t size, mempool_t * const pool) {
	if (pool == NULL) { /* no mempool is used, do normal malloc */
		return mcJSON_malloc(size);
	}

	if ((pool->position + size) >= pool->buffer_length) { /* not enough space */
		return NULL;
	}

	void *pointer = (void*)(pool->content + pool->position);
	pool->position += size;

	return pointer;
}

void mcJSON_InitHooks(const mcJSON_Hooks * const hooks) {
	if (hooks == NULL) { /* Reset hooks */
		mcJSON_malloc = malloc;
		mcJSON_free = free;
		return;
	}

	mcJSON_malloc = (hooks->malloc_fn != NULL) ? hooks->malloc_fn : malloc;
	mcJSON_free = (hooks->free_fn != NULL) ? hooks->free_fn : free;
}

/* Internal constructor. */
static mcJSON *mcJSON_New_Item(mempool_t * const pool) {
	mcJSON* node = (mcJSON*)allocate(sizeof(mcJSON), pool);
	if (node) {
		memset(node, 0, sizeof(mcJSON));
	}

	return node;
}

/* Delete a mcJSON structure. */
void mcJSON_Delete(mcJSON *item) {
	mcJSON *next;
	while (item != NULL) {
		next = item->next;
		if (!(item->type & mcJSON_IsReference) && (item->child != NULL)) {
			mcJSON_Delete(item->child);
		}
		if (!(item->type & mcJSON_IsReference) && (item->valuestring != NULL) && (item->valuestring->content != NULL)) {
			buffer_destroy_from_heap(item->valuestring);
		}
		if (!(item->type & mcJSON_StringIsConst) && (item->name != NULL) && (item->name->content != NULL)) {
			buffer_destroy_from_heap(item->name);
		}
		mcJSON_free(item);
		item = next;
	}
}

/* Parse the input text to generate a number, and populate the result into item. */
static buffer_t *parse_number(mcJSON * const item, buffer_t * const input) {
	char *end_pointer;
	double number = strtod((char*)input->content + input->position, &end_pointer);
	input->position = ((unsigned char*)end_pointer) - input->content;

	item->valuedouble = number;
	item->valueint = (int)number;
	item->type = mcJSON_Number;

	return input;
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
static char* ensure(buffer_t * const buffer, size_t needed) {
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

/* allocate a buffer for printing */
buffer_t *printbuffer_allocate(const size_t size, buffer_t * const buffer) {
	/* buffered printing */
	if (buffer != NULL) {
		if (ensure(buffer, size) == NULL) { /* allocation failed */
			if (buffer->content != NULL) {
				/* if content exists, terminate with '\0', just to make sure */
				buffer->content[buffer->position] = '\0';
			}
		}
		return buffer;
	}

	/* allocate new memory (unbuffered printing) */
	return buffer_create_on_heap(size, size);
}

/* allocate a molch_buffer for parsing, inside a mempool_t if it exists */
buffer_t *parsebuffer_allocate(const size_t buffer_length, const size_t content_length, mempool_t * const pool) {
	if (pool == NULL) { /* unbuffered parsing */
		return buffer_create_on_heap(buffer_length, content_length);
	}

	/* buffered parsing */
	buffer_t *buffer = (buffer_t*) allocate(sizeof(buffer_t), pool);
	if (buffer == NULL) {
		return NULL;
	}

	unsigned char *content = NULL;
	if (buffer_length != 0) {
		content = (unsigned char*)allocate(buffer_length, pool);
		if (content == NULL) {
			return NULL;
		}
	}

	return buffer_init_with_pointer(buffer, content, buffer_length, content_length);
}


/* Render the number nicely from the given item into a string. */
static buffer_t *print_number(mcJSON * const item, buffer_t * const buffer) {
	buffer_t *output = NULL;
	double d = item->valuedouble;
	if (d == 0) { /* zero */
		output = printbuffer_allocate(2, buffer);
		if (output == NULL) {
			return NULL;
		}

		output->content_length = output->position + 1;
		if (buffer_copy_from_raw(output, output->position, (unsigned char*)"0", 0, 2) != 0) {
			output->content[output->position] = '\0';
			if (buffer == NULL) {
				buffer_destroy_from_heap(output);
			}
			return NULL;
		}
		output->position++;
	} else if ((fabs(((double)item->valueint) - d) <= DBL_EPSILON) && (d <= INT_MAX) && (d >= INT_MIN)) {
		/* number is an integer */
		static const size_t INT_STRING_SIZE = 21; /* 2^64+1 can be represented in 21 chars. */
		output = printbuffer_allocate(INT_STRING_SIZE, buffer);
		if (output == NULL) {
			return NULL;
		}

		output->position += snprintf((char*)output->content + output->position, output->buffer_length - output->position, "%d", item->valueint);
	} else {
		static const size_t DOUBLE_STRING_SIZE = 64; /* This is a nice tradeoff. */
		output = printbuffer_allocate(DOUBLE_STRING_SIZE, buffer);
		if (output == NULL) {
			return NULL;
		}
		if ((fpclassify(d) != FP_ZERO) && (!isnormal(d))) {
			output->content_length = output->position + 1;
			if (buffer_copy_from_raw(output, output->position, (unsigned char*)"null", 0, 5) != 0) {
				if (output->content != NULL) {
					output->content[output->position] = '\0';
				}
				if (buffer == NULL) {
					buffer_destroy_from_heap(output);
				}
				return NULL;
			}
			output->position += 4;
		} else if ((fabs(floor(d) - d) <= DBL_EPSILON) && (fabs(d) < 1.0e60)) {
			output->position += snprintf((char*)output->content + output->position, output->buffer_length - output->position, "%.0f", d);
		} else if ((fabs(d) < 1.0e-6) || (fabs(d) > 1.0e9)) {
			output->position += snprintf((char*)output->content + output->position, output->buffer_length - output->position, "%e", d);
		} else {
			output->position += snprintf((char*)output->content + output->position, output->buffer_length - output->position, "%f", d);
		}
	}

	output->content_length = output->position + 1;
	return output;
}

/* parse a 4 character hexadecimal number, return INT_MAX on failure */
static unsigned int parse_hex4(buffer_t * const input) {
	if ((input->position + 4) >= input->content_length) {
		return INT_MAX;
	}
	unsigned int number;
	size_t characters_read = sscanf("%4x", (char*)input->content + input->position, &number);
	if (characters_read != 4) {
		return INT_MAX;
	}
	input->position += characters_read;

	return number;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static buffer_t *parse_string(mcJSON * const item, buffer_t * const input, mempool_t * const pool) {
	if (input->content[input->position] != '\"') { /* not a string! */
		return NULL;
	}

	input->position++;


	/* find the position of the closing '"' and get the length of the string */
	size_t length = 0;
	size_t end_position;
	for (end_position = input->position;
			(end_position < input->content_length)
			&& (input->content[end_position] != '\"')
			&& (input->content[end_position] != '\0');
			end_position++, length++) {
		if (input->content[end_position] == '\\') { /* Skip escaped quotes */
			end_position++;
		}
	}

	buffer_t *value_out = parsebuffer_allocate(length + 1, 0, pool);
	if (value_out == NULL) {
		return NULL;
	}

	for (;
		(input->position < end_position)
		&& (input->position < input->content_length)
		&& (input->content[input->position] != '\"')
		&& (input->content[input->position] != '\0');
		value_out->position++, input->position++) {
		if (input->content[input->position] != '\\') { /* regular character */
			value_out->content[value_out->position] = input->content[input->position];
		} else { /* special character */
			input->position++; /* skip initial '\\' */
			switch (input->content[input->position]) {
				case 'b':
					value_out->content[value_out->position] = '\b';
					break;
				case 'f':
					value_out->content[value_out->position] = '\f';
					break;
				case 'n':
					value_out->content[value_out->position] = '\n';
					break;
				case 'r':
					value_out->content[value_out->position] = '\r';
					break;
				case 't':
					value_out->content[value_out->position] = '\t';
					break;
				case 'u': {/* transcode utf16 to utf8. */
						/* FIXME: I don't quite understand this code yet, therefore I can't refactor it */
						if ((input->position + 4) >= input->content_length) {
							buffer_destroy_from_heap(value_out);
							return NULL;
						}
						unsigned uc;
						unsigned uc2;
						input->position++;
						uc = parse_hex4(input) /* get the unicode char. */;
						input->position += 4;

						if (((uc >= 0xDC00) && (uc <= 0xDFFF)) || (uc == 0)) { /* check for invalid */
							break;
						}

						if ((uc >= 0xD800) && (uc<=0xDBFF)) { /* UTF16 surrogate pairs. */
							if ((input->content[input->position + 1] != '\\') || (input->content[input->position + 2] != 'u')) { /* missing second-half of surrogate. */
								break;
							}
							if ((input->position + 6) >= input->content_length) {
								buffer_destroy_from_heap(value_out);
								return NULL;
							}
							uc2 = parse_hex4(input);
							if ((uc2 < 0xDC00) || (uc2 > 0xDFFF)) { /* invalid second-half of surrogate. */
								break;
							}
							uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
						}

						length = 4;
						if (uc < 0x80) {
							length = 1;
						} else if (uc < 0x800) {
							length = 2;
						} else if (uc < 0x10000) {
							length = 3;
							value_out->position += length;
						}

						static const unsigned char firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};
						switch (length) {
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
								value_out->content[value_out->position] = (uc | firstByteMark[length]);
						}
						value_out->position += length - 1;
					}
					break;
				default:
					value_out->content[value_out->position] = input->content[input->position];
					break;
			}
		}
	}
	/* null terminate the output string */
	value_out->content[value_out->position] = '\0';
	value_out->content_length = value_out->position + 1;
	if (input->content[input->position] == '\"') {
		input->position++;
	}
	item->valuestring = value_out;
	item->type = mcJSON_String;

	return input;
}

/* Render the cstring provided to an escaped version that can be printed. */
static buffer_t *print_string_ptr(buffer_t * const string, buffer_t * const buffer) {
	buffer_t *output = NULL;

	/* empty string */
	if ((string == NULL) || (string->content_length == 0)) {
		output = printbuffer_allocate(3, buffer);
		if (output == NULL) {
			return NULL;
		}
		if (output->content == NULL) {
			buffer_destroy_from_heap(output);
			return NULL;
		}

		/* fill with empty string */
		output->content_length = output->position + 1;
		if (buffer_copy_from_raw(output, output->position, (unsigned char*)"\"\"", 0, 3) != 0) {
			output->content[output->position] = '\0';
			if (buffer == NULL) {
				buffer_destroy_from_heap(output);
			}
			return NULL;
		}
		output->position += 2;

		output->content_length = output->position + 1;

		return output;
	}

	/* get the number of additional characters needed to encode special characters */
	size_t additional_characters = 0; /* number of additional chars needed for escaping */
	for (string->position = 0; string->position < (string->content_length - 1); string->position++) {
		if (strchr("\"\\\b\f\n\r\t", string->content[string->position])) {
			/* additional space for '\\' needed */
			additional_characters++;
		} else if (string->content[string->position] < 32) {
			/* "\\uXXXX" -> 5 additional characters */
			additional_characters += 5;
		}
	}

	/* allocate output */
	output = printbuffer_allocate(string->content_length + additional_characters + 3, buffer);
	if (output == NULL) {
		return NULL;
	}
	if (output->content == NULL) {
		buffer_destroy_from_heap(output);
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
			}
			return NULL;
		}
		output->position += string->content_length - 1;

		/* end double quotes and terminate with '\0' */
		output->content[output->position] = '\"';
		output->position++;
		output->content[output->position] = '\0';
		output->content_length = output->position + 1;

		return output;
	}

	/* start output with double quote */
	output->content[output->position] = '\"';
	output->position++;
	for (string->position = 0; (string->position < (string->content_length - 1)) && (output->position < output->buffer_length); string->position++, output->position++) {
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
		}
		return NULL;
	}
	output->content[output->position] = '\"';
	output->position++;
	output->content[output->position] = '\0';
	output->content_length = output->position + 1;

	return output;
}

/* Invoke print_string_ptr (which is useful) on an item. */
static buffer_t *print_string(mcJSON * const item, buffer_t * const buffer) {
	return print_string_ptr(item->valuestring, buffer);
}

/* Predeclare these prototypes. */
static buffer_t *parse_value(mcJSON * const item, buffer_t * const input, mempool_t * const pool);
static buffer_t *print_value(mcJSON * const item, const size_t depth, const bool format, buffer_t * const buffer);
static buffer_t *parse_array(mcJSON * const item, buffer_t * const input, mempool_t * const pool);
static buffer_t *print_array(mcJSON * const item, const size_t depth, const bool format, buffer_t * const buffer);
static buffer_t *parse_object(mcJSON * const item, buffer_t * const input, mempool_t * const pool);
static buffer_t *print_object(mcJSON * const item, const size_t depth, const bool format, buffer_t * const buffer);

/* Utility to jump whitespace and cr/lf */
static buffer_t *skip(buffer_t * const input) {
	if ((input == NULL) || (input->content == NULL)) {
		return NULL;
	}
	while ((input->content[input->position] != '\0') && (input->content[input->position] <= 32)) {
		input->position++;
	}
	return input;
}

/* Parse an object - create a new root, and populate.
 * This supports buffered parsing, a big chunk of memory
 * is allocated once and the json tree is parsed into it.
 * The size needs to be large enough otherwise allocation
 * will fail at some point. */
mcJSON *mcJSON_ParseWithBuffer(buffer_t * const json, mempool_t * const pool){
	json->position = 0; /* TODO could later be replaced with a position parameter */

	mcJSON *root = mcJSON_New_Item(pool);
	if (root == NULL) { /* memory fail */
		return NULL;
	}


	/* now parse */
	if (parse_value(root, skip(json), pool) == NULL) {
		if (pool == NULL) {
			mcJSON_Delete(root);
		} else {
			buffer_destroy_from_heap(pool);
		}
		return NULL;
	}

	return root;
}

mcJSON *mcJSON_ParseBuffered(buffer_t * const input_string, const size_t buffer_size) {
	mempool_t *pool = buffer_create_on_heap(buffer_size, buffer_size);
	if (pool == NULL) {
		return NULL;
	}

	mcJSON *json = mcJSON_ParseWithBuffer(input_string, pool);
	if (json != NULL) {
		mcJSON_free(pool); /* free the pool description, not the content */
	}

	return json;
}

/* Default options for mcJSON_Parse */
mcJSON *mcJSON_Parse(buffer_t * const json) {
	return mcJSON_ParseWithBuffer(json, NULL);
}

/* Render a mcJSON item/entity/structure to text. */
buffer_t *mcJSON_Print(mcJSON * const item) {
	return print_value(item, 0, true, NULL);
}
buffer_t *mcJSON_PrintUnformatted(mcJSON * const item) {
	return print_value(item, 0, false, NULL);
}

buffer_t *mcJSON_PrintBuffered(mcJSON * const item, const size_t prebuffer, const bool format) {
	buffer_t *buffer = buffer_create_on_heap(prebuffer, prebuffer);
	if (buffer == NULL) {
		return NULL;
	}
	if (print_value(item, 0, format, buffer) == NULL) {
		buffer_destroy_from_heap(buffer);
		return NULL;
	}
	return buffer;
}

/* Parser core - when encountering text, process appropriately. */
static buffer_t *parse_value(mcJSON * const item, buffer_t * const input, mempool_t * const pool) {
	if ((input == NULL) || (input->content == NULL)) {
		return NULL;
	}

	if (buffer_compare_to_raw_partial(input, input->position, (unsigned char*)"null", sizeof("null"), 0, sizeof("null") - 1) == 0) {
		item->type = mcJSON_NULL;
		input->position += sizeof("null") - 1;
		return input;
	}
	if (buffer_compare_to_raw_partial(input, input->position, (unsigned char*)"false", sizeof("false"), 0, sizeof("false") - 1) == 0) {
		item->type = mcJSON_False;
		input->position += sizeof("false") - 1;
		return input;
	}
	if (buffer_compare_to_raw_partial(input, input->position, (unsigned char*)"true", sizeof("true"), 0, sizeof("true") - 1) == 0) {
		item->type = mcJSON_True;
		input->position += sizeof("true") - 1;
		return input;
	}
	if (input->content[input->position] == '\"') {
		return parse_string(item, input, pool);
	}
	if ((input->content[input->position] == '-') || ((input->content[input->position] >= '0') && (input->content[input->position] <= '9'))) {
		return parse_number(item, input);
	}
	if (input->content[input->position] == '[') {
		return parse_array(item, input, pool);
	}
	if (input->content[input->position] == '{') {
		return parse_object(item, input, pool);
	}

	return NULL; /* failure. */
}

/* Render a value to text. */
static buffer_t *print_value(mcJSON * const item, const size_t depth, const bool format, buffer_t * const buffer) {
	if (item == NULL) {
		return NULL;
	}

	/* buffered printing */
	if (buffer != NULL) {
		switch (item->type) {
			case mcJSON_NULL:
				if (printbuffer_allocate(5, buffer) == NULL) {
					return NULL;
				}
				if (buffer_copy_from_raw(buffer, buffer->position, (unsigned char*)"null", 0, 5) != 0) {
					if (buffer->content != NULL) {
						buffer->content[buffer->position] = '\0';
					}
					return NULL;
				}
				buffer->position += 4;
				break;
			case mcJSON_False:
				if (printbuffer_allocate(6, buffer) == NULL) {
					return NULL;
				}
				if (buffer_copy_from_raw(buffer, buffer->position, (unsigned char*)"false", 0, 6) != 0) {
					if (buffer->content != NULL) {
						buffer->content[buffer->position] = '\0';
					}
					return NULL;
				}
				buffer->position += 5;
				break;
			case mcJSON_True:
				if (printbuffer_allocate(5, buffer) == NULL) {
					return NULL;
				}
				if (buffer_copy_from_raw(buffer, buffer->position, (unsigned char*)"true", 0, 5) != 0) {
					if (buffer->content != NULL) {
						buffer->content[buffer->position] = '\0';
					}
					return NULL;
				}
				buffer->position += 4;
				break;
			case mcJSON_Number:
				if(print_number(item, buffer) == NULL) {
					return NULL;
				}
				break;
			case mcJSON_String:
				if (print_string(item, buffer) == NULL) {
					return NULL;
				}
				break;
			case mcJSON_Array:
				if (print_array(item, depth, format, buffer) == NULL) {
					return NULL;
				}
				break;
			case mcJSON_Object:
				if (print_object(item, depth, format, buffer) == NULL) {
					return NULL;
				}
				break;
		}
		return buffer;
	}

	/* non buffered printing */
	switch (item->type) {
		case mcJSON_NULL:
			return buffer_create_from_string_on_heap("null");
		case mcJSON_False:
			return buffer_create_from_string_on_heap("false");
		case mcJSON_True:
			return buffer_create_from_string_on_heap("true");
		case mcJSON_Number:
			return print_number(item, NULL);
		case mcJSON_String:
			return print_string(item, NULL);
		case mcJSON_Array:
			return print_array(item, depth, format, NULL);
		case mcJSON_Object:
			return print_object(item, depth, format, NULL);
		default:
			return NULL;
	}
}

/* Build an array from input text. */
static buffer_t *parse_array(mcJSON * const item, buffer_t * const input, mempool_t * const pool) {
	if ((input == NULL) || (input->content == NULL)) {
		return NULL;
	}
	if (input->content[input->position] != '[') { /* not an array! */
		return NULL;
	}

	item->type = mcJSON_Array;
	input->position++;
	skip(input);
	if (input->content[input->position] == ']') { /* empty array. */
		input->position++;
		return input;
	}

	mcJSON *child = mcJSON_New_Item(pool);
	item->child = child;
	if (item->child == NULL) { /* memory fail */
		return NULL;
	}
	if(skip(parse_value(child, skip(input), pool)) == NULL) {
		return NULL;
	}

	while ((input->position < input->content_length) && (input->content[input->position] == ',')) {
		mcJSON *new_item = mcJSON_New_Item(pool);
		if (new_item == NULL) { /* memory fail */
			return NULL;
		}
		child->next = new_item;
		new_item->prev = child;
		input->position++;
		if (skip(parse_value(new_item, skip(input), pool)) == NULL) {
			return NULL;
		}
		child = new_item;
	}

	if (input->content[input->position] == ']') { /* end of array */
		input->position++;
		return input;
	}

	return NULL; /* malformed. */
}

/* Render an array to text */
static buffer_t *print_array(mcJSON * const item, const size_t depth, const bool format, buffer_t * const buffer) {
	if (item == NULL) {
		return NULL;
	}

	buffer_t *output = NULL;

	/* How many entries in the array? */
	size_t numentries = mcJSON_GetArraySize(item);

	/* Explicitly handle numentries == 0 */
	if (numentries == 0) { /* empty array */
		output = printbuffer_allocate(3, buffer);
		if (output == NULL) {
			return NULL;
		}

		/* fill with empty array */
		output->content_length = output->position + 1;
		if (buffer_copy_from_raw(output, output->position, (unsigned char*)"[]", 0, 3) != 0) {
			if (output->content != NULL) {
				output->content[output->position] = '\0';
			}
			if (buffer == NULL) {
				buffer_destroy_from_heap(output);
			}
			return NULL;
		}
		output->position += 2;

		output->content_length = output->position + 1;

		return output;
	}

	/* buffered */
	if (buffer != NULL) {
		/* allocate the buffer */
		if (printbuffer_allocate(1, buffer) == NULL) {
			return NULL;
		}

		/* start with opening brace */
		buffer->content[buffer->position] = '[';
		buffer->position++;

		mcJSON *child = item->child;
		bool fail = false;
		while ((child != NULL) && !fail) {
			if (print_value(child, depth + 1, format, buffer) == NULL) {
				buffer->content[buffer->position] = '\0';
				return NULL;
			}

			if (child->next != NULL) {
				size_t length = format ? 2 : 1; /* place for one space needed if format */
				if (printbuffer_allocate(length + 1, buffer) == NULL) {
					return NULL;
				}
				buffer->content[buffer->position] = ',';
				buffer->position++;
				if (format) {
					buffer->content[buffer->position] = ' ';
					buffer->position++;
				}
				buffer->content[buffer->position] = '\0';
			}
			child = child->next;
		}
		if (printbuffer_allocate(2, buffer) == NULL) {
			return NULL;
		}
		buffer->content[buffer->position] = ']';
		buffer->position++;
		buffer->content[buffer->position] = '\0';

		buffer->content_length = buffer->position + 1;

		return buffer;
	}

	/* unbuffered */
	/* Allocate an array to hold the values for each */
	buffer_t **entries = (buffer_t**)allocate(numentries * sizeof(buffer_t*), NULL);
	if (entries == NULL) {
		return NULL;
	}
	memset(entries, 0, numentries * sizeof(buffer_t*)); /* initialize as NULL pointers */

	/* Retrieve all the results: */
	mcJSON *child = item->child;
	size_t length = 0;
	bool fail = false;
	for (size_t i = 0; (i < numentries) && (child != NULL) && !fail; child = child->next, i++) {
		entries[i] = print_value(child, depth + 1, format, NULL);
		if (entries[i] == NULL) {
			fail = true;
			break;
		}

		length += entries[i]->content_length + 1 + (format ? 1 : 0);
	}

	/* If we didn't fail, try to alloc the output string */
	if (!fail) {
		output = buffer_create_on_heap(length, length);
	}
	/* If that fails, we fail. */
	if ((output == NULL) || (output->content == NULL)) {
		fail = true;
	}

	/* Compose the output array. */
	if (!fail) {
		output->content[output->position] = '[';
		output->position++;
		output->content[output->position] = '\0';
		for (size_t i = 0; i < numentries; i++) {
			if (buffer_copy(output, output->position, entries[i], 0, entries[i]->content_length - 1) != 0) {
				if (output->content != NULL) {
					output->content[output->position] = '\0';
				}
				fail = true;
				break;
			}
			output->position += entries[i]->content_length - 1;
			if (i != (numentries - 1)) {
				output->content[output->position] = ',';
				output->position++;
				if (format) {
					output->content[output->position] = ' ';
					output->position++;
				}
				output->content[output->position] = '\0';
			}
			buffer_destroy_from_heap(entries[i]);
			entries[i] = NULL;
		}
	}

	/* Handle failure. */
	if (fail) {
		for (size_t i = 0; i < numentries; i++) {
			if (entries[i] != NULL) {
				buffer_destroy_from_heap(entries[i]);
			}
		}
		mcJSON_free(entries);
		if (output != NULL) {
			buffer_destroy_from_heap(output);
		}
		return NULL;
	}

	mcJSON_free(entries);
	output->content[output->position] = ']';
	output->position++;
	output->content[output->position] = '\0';

	output->content_length = output->position + 1;

	return output;
}

/* Build an object from the text. */
static buffer_t *parse_object(mcJSON * const item, buffer_t * const input, mempool_t * const pool) {
	if ((input == NULL) || (input->content == NULL)) {
		return NULL;
	}
	if (input->content[input->position] != '{') { /* not an object! */
		return NULL;
	}

	item->type = mcJSON_Object;
	input->position++;
	skip(input);
	if (input->content[input->position] == '}') { /* empty object. */
		input->position++;
		return input;
	}

	mcJSON *child = mcJSON_New_Item(pool);
	item->child = child;
	if (item->child == NULL) {
		return NULL;
	}

	/* parse first key-value pair */
	if (skip(parse_string(child, skip(input), pool)) == NULL) {
		return NULL;
	}
	child->name = child->valuestring; /* string was parsed to ->valuestring, but it was actually a name */
	child->valuestring = NULL;
	if (input->content[input->position] != ':') { /* fail! */
		return NULL;
	}
	input->position++;
	if (skip(parse_value(child, skip(input), pool)) == NULL) {
		return NULL;
	}

	while ((input->position < input->buffer_length) && (input->content[input->position] == ',')) {
		mcJSON *new_item = mcJSON_New_Item(pool);
		if (new_item == NULL) { /* memory fail */
			return NULL;
		}
		child->next = new_item;
		new_item->prev = child;
		child = new_item;
		input->position++;
		if (skip(parse_string(child, skip(input), pool)) == NULL) {
			return NULL;
		}
		child->name = child->valuestring;
		child->valuestring = NULL;
		if (input->content[input->position] != ':') { /* fail! */
			return NULL;
		}
		input->position++;
		if (skip(parse_value(child, skip(input), pool)) == NULL) {
			return NULL;
		}
	}

	if (input->content[input->position] == '}') { /* end of object */
		input->position++;
		return input;
	}

	return NULL; /* malformed. */
}

/* Render an object to text. */
static buffer_t *print_object(mcJSON * const item, size_t depth, const bool format, buffer_t * const buffer) {
	if (item == NULL) {
		return NULL;
	}

	buffer_t *output = NULL;
	bool fail = false;

	/* Count the number of entries. */
	size_t numentries = mcJSON_GetArraySize(item);

	/* Explicitly handle empty object case */
	if (numentries == 0) {
		/* '{' + '}' + '\0' + format: '\n' + depth */
		size_t length = format ? depth + 4 : 3;
		output = printbuffer_allocate(length, buffer);
		if (output == NULL) {
			return NULL;
		}
		if (output->content == NULL) {
			buffer_destroy_from_heap(output);
			return NULL;
		}

		output->content[output->position] = '{';
		output->position++;
		if (format) {
			output->content[output->position] = '\n';
			output->position++;
			for (size_t i = 0; i < depth; i++) {
				output->content[output->position + i] = '\t';
			}
			output->position += depth;
		}
		output->content[output->position] = '}';
		output->position++;
		output->content[output->position] = '\0';

		output->content_length = output->position + 1;

		return output;
	}

	/* buffered */
	if (buffer != NULL) {
		/* allocate memory */
		if (printbuffer_allocate(format ? 3 : 2, buffer) == NULL) {
			return NULL;
		}

		buffer->content[buffer->position] = '{';
		buffer->position++;
		if (format) {
			buffer->content[buffer->position] = '\n';
			buffer->position++;
		}
		buffer->content[buffer->position] = '\0';

		mcJSON *child = item->child;
		depth++;
		while (child != NULL) {
			if (format) {
				if (printbuffer_allocate(depth, buffer) == NULL) {
					return NULL;
				}
				for (size_t i = 0; i < depth; i++) {
					buffer->content[buffer->position + i] = '\t';
				}
				buffer->position += depth;
			}

			if(print_string_ptr(child->name, buffer) == NULL) {
				if (buffer->content != NULL) {
					buffer->content[buffer->position] = '\0';
				}
				return NULL;
			}

			if (printbuffer_allocate(format ? 2 : 1, buffer) == NULL) {
				return NULL;
			}
			buffer->content[buffer->position] = ':';
			buffer->position++;
			if (format) {
				buffer->content[buffer->position] = '\t';
				buffer->position++;
			}

			if (print_value(child, depth, format, buffer) == NULL) {
				if (buffer->content != NULL) {
					buffer->content[buffer->position] = '\0';
				}
				return NULL;
			}

			size_t length = (format ? 1 : 0) + ((child->next != NULL) ? 1 : 0); /* '\t'? ','? */
			if (printbuffer_allocate(length + 1, buffer) == NULL) {
				return NULL;
			}
			if (child->next != NULL) {
				buffer->content[buffer->position] = ',';
				buffer->position++;
			}
			if (format) {
				buffer->content[buffer->position] = '\n';
				buffer->position++;
			}
			buffer->content[buffer->position] = '\0';
			child = child->next;
		}
		if (printbuffer_allocate(format ? (depth + 1) : 2, buffer) == NULL) { /* (depth - 1) * '\t' + '}' + '\0' */
			return NULL;
		}
		if (format) {
			for (size_t i = 0; i < (depth - 1); i++) {
				buffer->content[buffer->position + i] = '\t';
			}
			buffer->position += depth - 1;
		}
		buffer->content[buffer->position] = '}';
		buffer->position++;
		buffer->content[buffer->position] = '\0';

		buffer->content_length = buffer->position + 1;

		return buffer;
	}

	/* unbuffered */
	/* Allocate space for the names and the objects */
	buffer_t **entries = NULL;
	entries = (buffer_t**)allocate(numentries * sizeof(buffer_t*), NULL);
	if (entries == NULL) {
		return NULL;
	}
	buffer_t **names = NULL;
	names = (buffer_t**)allocate(numentries * sizeof(buffer_t*), NULL);
	if (names == NULL) {
		mcJSON_free(entries);
		return NULL;
	}
	memset(entries, 0, sizeof(buffer_t*) * numentries);
	memset(names, 0, sizeof(buffer_t*) * numentries);

	/* Collect all the results into our arrays: */
	mcJSON *child = item->child;
	depth++;
	/* length = '{' + '}' + '\0' */
	size_t length = 3; /* TODO: Why was this 7 */
	if (format) {
		/* indentation for closing '}' + '\n' */
		length += (depth - 1) + 1;
	}
	for (size_t i = 0; (i < numentries) && (child != NULL) && !fail; child = child->next, i++) {
		names[i] = print_string_ptr(child->name, NULL);
		if (names[i] == NULL) {
			fail = true;
			break;
		}
		entries[i] = print_value(child, depth, format, NULL);
		if (entries[i] == NULL) {
			fail = true;
			break;
		}

		/* strlen(name) + ':' + strlen(entry) + ',' + (format ? '\t' + '\n' + depth) */
		length += names[i]->content_length + entries[i]->content_length + (format ? 2 + depth : 0);
	}
	length--; /* last entry has no ',' */

	/* Try to allocate the output string */
	if (!fail) {
		output = buffer_create_on_heap(length, length);
	}
	if ((output == NULL) || (output->content == NULL)) {
		fail = true;
	}

	/* Compose the output: */
	if (!fail) {
		output->content[output->position] = '{';
		output->position++;
		if (format) {
			output->content[output->position] = '\n';
			output->position++;
		}
		output->content[output->position] = '\0';
		for (size_t i = 0; i < numentries; i++) {
			if (format) { /* indentation */
				for (size_t j = 0; j < depth; j++) {
					output->content[output->position + j] = '\t';
				}
				output->position += depth;
			}
			if (buffer_copy(output, output->position, names[i], 0, names[i]->content_length - 1) != 0) {
				if (output->content != NULL) {
					output->content[output->position] = '\0';
				}
				fail = true;
				break;
			}
			output->position += names[i]->content_length - 1;

			output->content[output->position] = ':';
			output->position++;
			if (format) {
				output->content[output->position] = '\t';
				output->position++;
			}
			if (buffer_copy(output, output->position, entries[i], 0, entries[i]->content_length - 1) != 0) {
				if (output->content != NULL) {
					output->content[output->position] = '\0';
				}
				fail = true;
				break;
			}
			output->position += entries[i]->content_length - 1;

			if (i != (numentries - 1)) {
				output->content[output->position] = ',';
				output->position++;
			}

			if (format) {
				output->content[output->position] = '\n';
				output->position++;
			}
			output->content[output->position] = '\0';

			buffer_destroy_from_heap(names[i]);
			names[i] = NULL;
			buffer_destroy_from_heap(entries[i]);
			entries[i] = NULL;
		}
	}

	/* Handle failure */
	if (fail) {
		for (size_t i = 0; i < numentries; i++) {
			if (names[i] != NULL) {
				buffer_destroy_from_heap(names[i]);
			}
			if (entries[i] != NULL) {
				buffer_destroy_from_heap(entries[i]);
			}
		}
		mcJSON_free(names);
		mcJSON_free(entries);
		if (output != NULL) {
			buffer_destroy_from_heap(output);
		}
		return NULL;
	}

	mcJSON_free(names);
	mcJSON_free(entries);
	if (format) {
		for (size_t i = 0; i < (depth - 1); i++) {
			output->content[output->position + i] = '\t';
		}
		output->position += depth - 1;
	}
	output->content[output->position] = '}';
	output->position++;
	output->content[output->position] = '\0';

	output->content_length = output->position + 1;

	return output;
}

/* Get Array size/item / object item. */
size_t mcJSON_GetArraySize(const mcJSON * const array) {
	mcJSON *child = array->child;
	size_t size;
	for (size = 0; child != NULL; child = child->next, size++) {}
	return size;
}

mcJSON *mcJSON_GetArrayItem(const mcJSON * const array, size_t index) {
	mcJSON *child = array->child;
	while ((child != NULL) && (index > 0)) {
		index--;
		child = child->next;
	}
	return child;
}

mcJSON *mcJSON_GetObjectItem(const mcJSON * const object, const buffer_t * const string) {
	mcJSON *child = object->child;
	while ((child != NULL) && (buffer_compare(child->name, string) != 0)) {
		child = child->next;
	}
	return child;
}

/* Utility for array list handling. */
static void insert_into_object(mcJSON * const prev, mcJSON * const item) {
	prev->next = item;
	item->prev = prev;
}

/* Utility for handling references. */
static mcJSON *create_reference(const mcJSON * const item, mempool_t * const pool) {
	mcJSON *reference = mcJSON_New_Item(pool);

	if (reference == NULL) {
		return NULL;
	}

	memcpy(reference, item, sizeof(mcJSON));
	reference->name = NULL;
	reference->type |= mcJSON_IsReference;
	reference->next = reference->prev = NULL;

	return reference;
}

/* Add item to array/object. */
void mcJSON_AddItemToArray(mcJSON * const array, mcJSON * const item, mempool_t * const pool __attribute__((unused))) {
	mcJSON *child = array->child;

	if (item == NULL) {
		return;
	}

	if (child == NULL) {
		array->child = item;
	} else {
		while ((child != NULL) && child->next) {
			child = child->next;
		}
		insert_into_object(child, item);
	}
}

void mcJSON_AddItemToObject(mcJSON * const object, const buffer_t * const string, mcJSON * const item, mempool_t * const pool) {
	if (item == NULL) {
		return;
	}

	if ((item->name != NULL) && (item->name->content != NULL)) {
		if (pool != NULL) { /* can't destroy when using buffered parsing */
			return;
		}
		buffer_destroy_from_heap(item->name);
	}

	item->name = parsebuffer_allocate(string->content_length, string->content_length, pool);
	if (buffer_clone(item->name, string) != 0) {
		return;
	}

	mcJSON_AddItemToArray(object, item, pool);
}

/* TODO remove this? */
void mcJSON_AddItemToObjectCS(mcJSON * const object, const buffer_t * const string, mcJSON * const item, mempool_t * const pool) {
	if (item == NULL) {
		return;
	}

	if (!(item->type & mcJSON_StringIsConst) && (item->name != NULL) && (item->name->content != NULL)) {
		if (pool != NULL) { /* can't destroy when using buffered parsing */
			return;
		}
		buffer_destroy_from_heap(item->name);
	}

	item->name = parsebuffer_allocate(string->content_length, string->content_length, pool);
	int status = buffer_clone(item->name, string);
	if (status != 0) {
		//TODO proper error handling
		return;
	}
	item->type |= mcJSON_StringIsConst;
	mcJSON_AddItemToArray(object, item, pool);
}

void mcJSON_AddItemReferenceToArray(mcJSON * const array, const mcJSON * const item, mempool_t * const pool) {
	mcJSON_AddItemToArray(array, create_reference(item, pool), pool);
}
void mcJSON_AddItemReferenceToObject(mcJSON * const object, const buffer_t * const string, const mcJSON * const item, mempool_t * const pool) {
	mcJSON_AddItemToObject(object, string, create_reference(item, pool), pool);
}

/* detach child from parent */
mcJSON *detach_item(mcJSON * const parent, mcJSON * const child) {
	if (child == NULL) {
		return NULL;
	}

	if (child->prev != NULL) {
		child->prev->next = child->next;
	}

	if (child->next != NULL) {
		child->next->prev = child->prev;
	}

	if (child == parent->child) {
		parent->child = child->next;
	}

	child->next = NULL;
	child->prev = NULL;

	return child;
}

mcJSON *mcJSON_DetachItemFromArray(mcJSON * const array, const size_t index) {
	return detach_item(array, mcJSON_GetArrayItem(array, index));
}

void mcJSON_DeleteItemFromArray(mcJSON * const array, const size_t index) {
	mcJSON_Delete(mcJSON_DetachItemFromArray(array, index));
}

mcJSON *mcJSON_DetachItemFromObject(mcJSON * const object, const buffer_t * const string) {
	return detach_item(object, mcJSON_GetObjectItem(object, string));
}

void   mcJSON_DeleteItemFromObject(mcJSON * const object, const buffer_t * const string) {
	mcJSON_Delete(mcJSON_DetachItemFromObject(object, string));
}

/* insert an item into an array or object after "previous" */
void insert_item(mcJSON * const parent, mcJSON * const previous, mcJSON * const new_item, mempool_t * const pool) {
	if (previous == NULL) {
		mcJSON_AddItemToArray(parent, new_item, pool);
		return;
	}
	new_item->next = previous;
	new_item->prev = previous->prev;
	previous->prev = new_item;
	if (previous == parent->child) {
		parent->child = new_item;
	} else {
		new_item->prev->next = new_item;
	}
}

void   mcJSON_InsertItemInArray(mcJSON * const array, const size_t index, mcJSON * const new_item, mempool_t * const pool) {
	insert_item(array, mcJSON_GetArrayItem(array, index), new_item, pool);
}

void replace_item(mcJSON * const parent, mcJSON * const child, mcJSON * const new_item, mempool_t * const pool __attribute__((unused))) {
	if (child == NULL) {
		return;
	}

	new_item->next = child->next;
	new_item->prev = child->prev;

	if (new_item->next) {
		new_item->next->prev = new_item;
	}

	if (child == parent->child) {
		parent->child = new_item;
	} else {
		new_item->prev->next = new_item;
	}

	child->prev = NULL;
	child->next = NULL;

	mcJSON_Delete(child);
}

/* Replace array/object items with new ones. */
void mcJSON_ReplaceItemInArray(mcJSON * const array, const size_t index, mcJSON * const new_item, mempool_t * const pool) {
	replace_item(array, mcJSON_GetArrayItem(array, index), new_item, pool);
}

void mcJSON_ReplaceItemInObject(mcJSON * const object, const buffer_t * const string, mcJSON * const new_item, mempool_t * const pool) {
	replace_item(object, mcJSON_GetObjectItem(object, string), new_item, pool);
}

/* Create basic types: */
mcJSON *mcJSON_CreateNull(mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item) {
		item->type = mcJSON_NULL;
	}
	return item;
}
mcJSON *mcJSON_CreateTrue(mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item) {
		item->type = mcJSON_True;
	}
	return item;
}
mcJSON *mcJSON_CreateFalse(mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item) {
		item->type = mcJSON_False;
	}
	return item;
}
mcJSON *mcJSON_CreateBool(const bool b, mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item) {
		item->type = b ? mcJSON_True : mcJSON_False;
	}
	return item;
}
mcJSON *mcJSON_CreateNumber(const double num, mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item) {
		item->type = mcJSON_Number;
		item->valuedouble = num;
		item->valueint = (int)num;
	}
	return item;
}
mcJSON *mcJSON_CreateString(const buffer_t * const string, mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item) {
		item->type = mcJSON_String;
		item->valuestring = parsebuffer_allocate(string->content_length, string->content_length, pool);
		int status = buffer_clone(item->valuestring, string);
		if (status != 0) {
			if (pool != NULL) {
				mcJSON_Delete(item);
			}
			return NULL;
		}
	}
	return item;
}

mcJSON *mcJSON_CreateHexString(const buffer_t * const binary, mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item == NULL) {
		return NULL;
	}

	item->type = mcJSON_String;
	item->valuestring = parsebuffer_allocate(binary->content_length * 2 + 1, binary->content_length * 2 + 1, pool);

	if (buffer_clone_as_hex(item->valuestring, binary) != 0) {
		if (pool != NULL) {
			mcJSON_Delete(item);
		}
		return NULL;
	}

	return item;
}

mcJSON *mcJSON_CreateArray(mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item) {
		item->type = mcJSON_Array;
	}
	return item;
}
mcJSON *mcJSON_CreateObject(mempool_t * const pool) {
	mcJSON *item = mcJSON_New_Item(pool);
	if (item) {
		item->type = mcJSON_Object;
	}
	return item;
}

/* Create Arrays: */
mcJSON *mcJSON_CreateIntArray(const int * const numbers, const size_t count, mempool_t * const pool) {
	mcJSON *array = mcJSON_CreateArray(pool);
	mcJSON *child = array->child;
	for (size_t i = 0; i < count; i++) {
		mcJSON *number = mcJSON_CreateNumber((double)numbers[i], pool);
		insert_item(array, child, number, pool);
	}

	return array;
}

mcJSON *mcJSON_CreateDoubleArray(const double * const numbers, const size_t count, mempool_t * const pool) {
	mcJSON *array = mcJSON_CreateArray(pool);
	mcJSON *child = array->child;
	for (size_t i = 0; i < count; i++) {
		mcJSON *number = mcJSON_CreateNumber(numbers[i], pool);
		insert_item(array, child, number, pool);
	}

	return array;
}

mcJSON *mcJSON_CreateStringArray(const buffer_t **strings, const size_t count, mempool_t * const pool) {
	mcJSON *array = mcJSON_CreateArray(pool);
	mcJSON *child = array->child;
	for (size_t i = 0; i < count; i++) {
		mcJSON *string = mcJSON_CreateString(strings[i], pool);
		insert_item(array, child, string, pool);
	}

	return array;
}

/* Duplication */
mcJSON *mcJSON_Duplicate(const mcJSON * const item, const int recurse, mempool_t * const pool) {
	mcJSON *newitem;
	mcJSON *cptr;
	mcJSON *nptr = NULL;
	mcJSON *newchild;
	/* Bail on bad ptr */
	if (item == NULL) {
		return NULL;
	}
	/* Create new item */
	newitem = mcJSON_New_Item(pool);
	if (newitem == NULL) {
		return NULL;
	}
	/* Copy over all vars */
	newitem->type = item->type & (~mcJSON_IsReference);
	newitem->valueint = item->valueint;
	newitem->valuedouble = item->valuedouble;
	if ((item->valuestring != NULL) && (item->valuestring->content != NULL)) {
		newitem->valuestring = parsebuffer_allocate(item->valuestring->buffer_length, item->valuestring->buffer_length, pool);
		int status = buffer_clone(newitem->valuestring, item->valuestring);
		if (status != 0) {
			mcJSON_Delete(newitem);
			return NULL;
		}
	}
	if ((item->name != NULL) && (item->name->content != NULL)) {
		newitem->name = parsebuffer_allocate(item->name->buffer_length, item->name->buffer_length, pool);
		int status = buffer_clone(newitem->name, item->name);
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
		newchild = mcJSON_Duplicate(cptr, 1, pool); /* Duplicate (with recurse) each item in the ->next chain */
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

void mcJSON_Minify(buffer_t * const json) {
	json->position = 0;
	size_t write_position = json->position;
	while ((json->position < json->content_length) && (json->content[json->position] != '\0')) {
		/* Whitespace characters. */
		switch (json->content[json->position]) {
			case ' ':
			case '\t':
			case '\r':
			case '\n':
				json->position++;
				break;
			case '/':
				if ((json->position + 1) >= json->content_length) {
					break;
				}
				switch (json->content[json->position + 1]) {
					case '/': /* double-slash comment, skip to end of line */
						while ((json->position < json->content_length) && (json->content[json->position != '\0']) && (json->content[json->position != '\n'])) {
							json->position++;
						}
						break;
					case '*': /* multiline comment */
						while (((json->position + 1) < json->content_length)
								&& (json->content[json->position != '\0'])
								&& !((json->content[json->position] != '*') && (json->content[json->position] != '/'))) {
							json->position++;
						}
						if ((json->position + 1) < json->content_length) {
							json->position += 2;
						}
						break;
					default:
						json->content[write_position] = '/';
						json->position++;
						write_position++;
				}
			case '\"': /* string literal */
				json->content[write_position] = '\"';
				json->position++;
				write_position++;
				while ((json->position < json->content_length) && (json->content[json->position] != '\0') && (json->content[json->position] != '\"')) {
					if (json->content[json->position] == '\\') {
						if ((json->position + 1) < json->content_length) {
							json->content[write_position] = '\\';
							json->position++;
							write_position++;
						}
					}
					json->content[write_position] = json->content[json->position];
					json->position++;
					write_position++;
				}
				if (json->position < json->content_length) { /* write closing '"' */
					json->content[write_position] = '\"';
					json->position++;
					write_position++;
				}
				break;
			default: /* normal characters */
				json->content[write_position] = json->content[json->position];
				json->position++;
				write_position++;
		}
	}
	if (write_position < json->content_length) {
		json->content[write_position] = '\0';
		write_position++;
	}
	json->content_length = write_position;
}
