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
		return 0;
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
		if (!(c->type & mcJSON_IsReference) && c->valuestring) {
			mcJSON_free(c->valuestring);
		}
		if (!(c->type & mcJSON_StringIsConst) && c->string) {
			mcJSON_free(c->string);
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

static int pow2gt(int x) {
	--x;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

typedef struct {
	char *buffer;
	int length;
	int offset;
} printbuffer;

static char* ensure(printbuffer *p, int needed) {
	char *newbuffer;
	int newsize;
	if ((p == NULL) || (p->buffer == NULL)) {
		return 0;
	}
	needed += p->offset;
	if (needed <= p->length) {
		return p->buffer + p->offset;
	}

	newsize = pow2gt(needed);
	newbuffer = (char*)mcJSON_malloc(newsize);
	if (newbuffer == NULL) {
		mcJSON_free(p->buffer);
		p->length=0;
		p->buffer=0;
		return 0;
	}
	if (newbuffer) {
		memcpy(newbuffer, p->buffer, p->length);
	}
	mcJSON_free(p->buffer);
	p->length = newsize;
	p->buffer = newbuffer;
	return newbuffer + p->offset;
}

static int update(printbuffer *p) {
	char *str;
	if ((p == NULL) || (p->buffer == NULL)) {
		return 0;
	}
	str = p->buffer + p->offset;
	return p->offset + strlen(str);
}

/* Render the number nicely from the given item into a string. */
static char *print_number(mcJSON *item,printbuffer *p) {
	char *str = 0;
	double d = item->valuedouble;
	if (d == 0) {
		if (p) {
			str = ensure(p, 2);
		} else {
			str = (char*)mcJSON_malloc(2); /* special case for 0. */
		}
		if (str) {
			strcpy(str, "0");
		}
	} else if ((fabs(((double)item->valueint) - d) <= DBL_EPSILON) && (d <= INT_MAX) && (d >= INT_MIN)) {
		if (p) {
			str = ensure(p, 21);
		} else {
			str = (char*)mcJSON_malloc(21); /* 2^64+1 can be represented in 21 chars. */
		}
		if (str) {
			sprintf(str, "%d", item->valueint);
		}
	} else {
		if (p) {
			str = ensure(p, 64);
		} else {
			str = (char*)mcJSON_malloc(64); /* This is a nice tradeoff. */
		}
		if (str) {
			if ((fpclassify(d) != FP_ZERO) && (!isnormal(d))) {
				sprintf(str,"null");
			} else if ((fabs(floor(d) - d) <= DBL_EPSILON) && (fabs(d) < 1.0e60)) {
				sprintf(str, "%.0f", d);
			} else if ((fabs(d) < 1.0e-6) || (fabs(d) > 1.0e9)) {
				sprintf(str, "%e", d);
			} else {
				sprintf(str,"%f",d);
			}
		}
	}
	return str;
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
	char *ptr2;
	char *out;
	int len = 0;
	unsigned uc;
	unsigned uc2;
	if (*str != '\"') { /* not a string! */
		ep = str;
		return 0;
	}

	while ((*ptr != '\"') && *ptr && ++len) {
		if (*ptr++ == '\\') { /* Skip escaped quotes. */
			ptr++;
		}
	}
	const char * end_ptr = ptr;

	out = (char*)mcJSON_malloc(len + 1); /* This is how long we need for the string, roughly. */
	if (out == NULL) {
		return 0;
	}

	ptr = str + 1;
	ptr2 = out;
	while ((*ptr != '\"') && *ptr && (ptr <= end_ptr)) {
		if (*ptr != '\\') {
			*ptr2++ = *ptr++;
		} else {
			ptr++;
			switch (*ptr)
			{
				case 'b':
					*ptr2++ = '\b';
					break;
				case 'f':
					*ptr2++ = '\f';
					break;
				case 'n':
					*ptr2++ = '\n';
					break;
				case 'r':
					*ptr2++ = '\r';
					break;
				case 't':
					*ptr2++ = '\t';
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
						ptr2 += len;
					}

					switch (len) {
						case 4:
							*--ptr2 = ((uc | 0x80) & 0xBF);
							uc >>= 6;
						case 3:
							*--ptr2 = ((uc | 0x80) & 0xBF);
							uc >>= 6;
						case 2:
							*--ptr2 = ((uc | 0x80) & 0xBF);
							uc >>= 6;
						case 1:
							*--ptr2 = (uc | firstByteMark[len]);
					}
					ptr2 += len;
					break;
				default:
					*ptr2++ = *ptr;
					break;
			}
			ptr++;
		}
	}
	*ptr2 = 0;
	if (*ptr == '\"') {
		ptr++;
	}
	item->valuestring = out;
	item->type = mcJSON_String;
	return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str, printbuffer *p) {
	const char *ptr;
	char *ptr2;
	char *out;
	int len = 0;
	int flag = 0;
	unsigned char token;

	for (ptr = str; *ptr != '\0'; ptr++) {
		flag |= (((*ptr > 0) && (*ptr < 32)) || (*ptr == '\"') || (*ptr == '\\')) ? 1 : 0;
	}
	if (!flag) {
		len = ptr - str;
		if (p) {
			out = ensure(p, len + 3);
		} else {
			out = (char*)mcJSON_malloc(len + 3);
		}
		if (out == NULL) {
			return 0;
		}
		ptr2 = out;
		*ptr2++ = '\"';
		strcpy(ptr2, str);
		ptr2[len] = '\"';
		ptr2[len + 1] = 0;
		return out;
	}

	if (str == NULL) {
		if (p) {
			out = ensure(p, 3);
		} else {
			out = (char*)mcJSON_malloc(3);
		}
		if (out == NULL) {
			return 0;
		}
		strcpy(out, "\"\"");
		return out;
	}
	ptr = str;
	while ((token = *ptr) && ++len) {
		if (strchr("\"\\\b\f\n\r\t", token)) {
			len++;
		} else if (token < 32) {
			len += 5;
		}
		ptr++;
	}

	if (p) {
		out = ensure(p, len + 3);
	} else {
		out = (char*)mcJSON_malloc(len + 3);
	}
	if (out == NULL) {
		return 0;
	}

	ptr2 = out;
	ptr = str;
	*ptr2++ = '\"';
	while (*ptr != '\0') {
		if (((unsigned char)*ptr > 31) && (*ptr != '\"') && (*ptr != '\\')) {
			*ptr2++ = *ptr++;
		} else {
			*ptr2++ = '\\';
			switch (token = *ptr++) {
				case '\\':
					*ptr2++ = '\\';
					break;
				case '\"':
					*ptr2++ = '\"';
					break;
				case '\b':
					*ptr2++ = 'b';
					break;
				case '\f':
					*ptr2++ = 'f';
					break;
				case '\n':
					*ptr2++ = 'n';
					break;
				case '\r':
					*ptr2++ = 'r';
					break;
				case '\t':
					*ptr2++ = 't';
					break;
				default: /* escape and print */
					sprintf(ptr2, "u%04x", token);
					ptr2 += 5;
					break;
			}
		}
	}
	*ptr2++ = '\"';
	*ptr2++ = 0;
	return out;
}
/* Invote print_string_ptr (which is useful) on an item. */
static char *print_string(mcJSON *item, printbuffer *p) {
	return print_string_ptr(item->valuestring, p);
}

/* Predeclare these prototypes. */
static const char *parse_value(mcJSON *item, const char *value);
static char *print_value(mcJSON *item, int depth, int fmt, printbuffer *p);
static const char *parse_array(mcJSON *item, const char *value);
static char *print_array(mcJSON *item, int depth, int fmt, printbuffer *p);
static const char *parse_object(mcJSON *item, const char *value);
static char *print_object(mcJSON *item, int depth, int fmt, printbuffer *p);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in) {
	while (in && *in && ((unsigned char)*in <= 32)) {
		in++;
	}
	return in;
}

/* Parse an object - create a new root, and populate. */
mcJSON *mcJSON_ParseWithOpts(const char *value, const char **return_parse_end, int require_null_terminated) {
	const char *end = 0;
	mcJSON *c = mcJSON_New_Item();
	ep = 0;
	if (c == NULL) { /* memory fail */
		return 0;
	}

	end = parse_value(c, skip(value));
	if (end == NULL) { /* parse failure. ep is set. */
		mcJSON_Delete(c);
		return 0;
	}

	/* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
	if (require_null_terminated) {
		end = skip(end);
		if (*end) {
			mcJSON_Delete(c);
			ep = end;
			return 0;
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
	return print_value(item, 0, 1, 0);
}
char *mcJSON_PrintUnformatted(mcJSON *item) {
	return print_value(item, 0, 0, 0);
}

char *mcJSON_PrintBuffered(mcJSON *item, int prebuffer, int fmt) {
	printbuffer p;
	p.buffer = (char*)mcJSON_malloc(prebuffer);
	p.length = prebuffer;
	p.offset = 0;
	return print_value(item,0,fmt,&p);
}


/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(mcJSON *item, const char *value) {
	if (value == NULL) { /* Fail on null. */
		return 0;
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
	return 0; /* failure. */
}

/* Render a value to text. */
static char *print_value(mcJSON *item, int depth, int fmt, printbuffer *p) {
	char *out = 0;
	if (!item) {
		return 0;
	}
	if (p) {
		switch ((item->type) & 255) {
			case mcJSON_NULL:
				out = ensure(p, 5);
				if (out) {
					strcpy(out, "null");
				}
				break;
			case mcJSON_False:
				out = ensure(p, 6);
				if (out) {
					strcpy(out, "false");
				}
				break;
			case mcJSON_True:
				out = ensure(p, 5);
				if (out) {
					strcpy(out, "true");
				}
				break;
			case mcJSON_Number:
				out = print_number(item, p);
				break;
			case mcJSON_String:
				out = print_string(item, p);
				break;
			case mcJSON_Array:
				out = print_array(item, depth, fmt, p);
				break;
			case mcJSON_Object:
				out = print_object(item, depth, fmt, p);
				break;
		}
	} else {
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
				out = print_number(item, 0);
				break;
			case mcJSON_String:
				out = print_string(item, 0);
				break;
			case mcJSON_Array:
				out = print_array(item, depth, fmt, 0);
				break;
			case mcJSON_Object:
				out = print_object(item, depth, fmt, 0);
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
		return 0;
	}

	item->type = mcJSON_Array;
	value = skip(value + 1);
	if (*value == ']') { /* empty array. */
		return value + 1;
	}

	child = mcJSON_New_Item();
	item->child = child;
	if (item->child == NULL) { /* memory fail */
		return 0;
	}
	value = skip(parse_value(child, skip(value))); /* skip any spacing, get the value. */
	if (value == NULL) {
		return 0;
	}

	while (*value == ',') {
		mcJSON *new_item;
		if ((new_item = mcJSON_New_Item()) == NULL) { /* memory fail */
			return 0;
		}
		child->next = new_item;
		new_item->prev = child;
		child = new_item;
		value = skip(parse_value(child, skip(value + 1)));
		if (value == NULL) { /* memory fail */
			return 0;
		}
	}

	if (*value == ']') { /* end of array */
		return value + 1;
	}
	ep = value;
	return 0; /* malformed. */
}

/* Render an array to text */
static char *print_array(mcJSON *item, int depth, int fmt, printbuffer *p) {
	char **entries;
	char *out=0;
	char *ptr;
	char *ret;
	int len = 5;
	mcJSON *child = item->child;
	int numentries = 0;
	int i = 0;
	int fail = 0;
	size_t tmplen = 0;

	/* How many entries in the array? */
	while (child) {
		numentries++;
		child = child->next;
	}
	/* Explicitly handle numentries==0 */
	if (numentries == 0) {
		if (p) {
			out = ensure(p, 3);
		} else {
			out = (char*)mcJSON_malloc(3);
		}
		if (out) {
			strcpy(out, "[]");
		}
		return out;
	}

	if (p) {
		/* Compose the output array. */
		i = p->offset;
		ptr = ensure(p, 1);
		if (ptr == NULL) {
			return 0;
		}
		*ptr = '[';
		p->offset++;
		child = item->child;
		while (child && !fail) {
			print_value(child, depth + 1, fmt, p);
			p->offset = update(p);
			if (child->next) {
				len = fmt ? 2 : 1;
				ptr = ensure(p, len + 1);
				if (ptr == NULL) {
					return 0;
				}
				*ptr++ = ',';
				if (fmt) {
					*ptr++ = ' ';
				}
				*ptr = 0;
				p->offset += len;
			}
			child = child->next;
		}
		ptr = ensure(p, 2);
		if (ptr == NULL) {
			return 0;
		}
		*ptr++ = ']';
		*ptr = 0;
		out = (p->buffer) + i;
	} else {
		/* Allocate an array to hold the values for each */
		entries = (char**)mcJSON_malloc(numentries * sizeof(char*));
		if (entries == NULL) {
			return 0;
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
				fail = 1;
			}
			child = child->next;
		}

		/* If we didn't fail, try to malloc the output string */
		if (!fail) {
			out = (char*)mcJSON_malloc(len);
		}
		/* If that fails, we fail. */
		if (out == NULL) {
			fail = 1;
		}

		/* Handle failure. */
		if (fail) {
			for (i = 0; i < numentries; i++) {
				if (entries[i]) {
					mcJSON_free(entries[i]);
				}
			}
			mcJSON_free(entries);
			return 0;
		}

		/* Compose the output array. */
		*out = '[';
		ptr = out + 1;
		*ptr = 0;
		for (i = 0; i < numentries; i++) {
			tmplen = strlen(entries[i]);
			memcpy(ptr, entries[i], tmplen);
			ptr += tmplen;
			if (i != (numentries - 1)) {
				*ptr++ = ',';
				if (fmt) {
					*ptr++ = ' ';
				}
				*ptr = 0;
			}
			mcJSON_free(entries[i]);
		}
		mcJSON_free(entries);
		*ptr++ = ']';
		*ptr++ = 0;
	}
	return out;
}

/* Build an object from the text. */
static const char *parse_object(mcJSON *item, const char *value) {
	mcJSON *child;
	if (*value != '{') { /* not an object! */
		ep = value;
		return 0;
	}

	item->type = mcJSON_Object;
	value = skip(value + 1);
	if (*value=='}') { /* empty object. */
		return value + 1;
	}

	child = mcJSON_New_Item();
	item->child = child;
	if (item->child == NULL) {
		return 0;
	}
	value = skip(parse_string(child, skip(value)));
	if (value == NULL) {
		return 0;
	}
	child->string = child->valuestring;
	child->valuestring = 0;
	if (*value != ':') { /* fail! */
		ep = value;
		return 0;
	}
	value = skip(parse_value(child, skip(value + 1))); /* skip any spacing, get the value. */
	if (value == NULL) {
		return 0;
	}

	while (*value == ',') {
		mcJSON *new_item;
		if ((new_item = mcJSON_New_Item()) == NULL) { /* memory fail */
			return 0;
		}
		child->next = new_item;
		new_item->prev = child;
		child = new_item;
		value = skip(parse_string(child, skip(value + 1)));
		if (value == NULL) {
			return 0;
		}
		child->string = child->valuestring;
		child->valuestring = 0;
		if (*value != ':') { /* fail! */
			ep = value;
			return 0;
		}
		value = skip(parse_value(child, skip(value + 1))); /* skip any spacing, get the value. */
		if (value == NULL) {
			return 0;
		}
	}

	if (*value == '}') { /* end of object */
		return value + 1;
	}
	ep = value;
	return 0; /* malformed. */
}

/* Render an object to text. */
static char *print_object(mcJSON *item, int depth, int fmt, printbuffer *p) {
	char **entries = 0;
	char **names = 0;
	char *out = 0;
	char *ptr;
	char *ret;
	char *str;
	int len = 7;
	int i = 0;
	int j;
	mcJSON *child = item->child;
	int numentries = 0;
	int fail = 0;
	size_t tmplen = 0;
	/* Count the number of entries. */
	while (child) {
		numentries++;
		child = child->next;
	}
	/* Explicitly handle empty object case */
	if (numentries == 0) {
		if (p) {
			out = ensure(p,fmt ? depth + 4 : 3);
		} else {
			out = (char*)mcJSON_malloc(fmt ? depth + 4 : 3);
		}
		if (out == NULL) {
			return 0;
		}
		ptr = out;
		*ptr++ = '{';
		if (fmt) {
			*ptr++ = '\n';
			for (i = 0; i < depth - 1; i++) {
				*ptr++ = '\t';
			}
		}
		*ptr++ = '}';
		*ptr++ = '\0';
		return out;
	}
	if (p) {
		/* Compose the output: */
		i = p->offset;
		len = fmt ? 2 : 1;
		ptr = ensure(p, len + 1);
		if (ptr == NULL) {
			return 0;
		}
		*ptr++ = '{';
		if (fmt) {
			*ptr++ = '\n';
		}
		*ptr = 0;
		p->offset += len;
		child = item->child;
		depth++;
		while (child) {
			if (fmt) {
				ptr = ensure(p, depth);
				if (ptr == NULL) {
					return 0;
				}
				for (j=0;j<depth;j++) {
					*ptr++='\t';
				}
				p->offset += depth;
			}
			print_string_ptr(child->string, p);
			p->offset = update(p);

			len = fmt ? 2 : 1;
			ptr = ensure(p, len);
			if (ptr == NULL) {
				return 0;
			}
			*ptr++ = ':';
			if (fmt) {
				*ptr++ = '\t';
			}
			p->offset += len;

			print_value(child, depth, fmt, p);
			p->offset = update(p);

			len = (fmt ? 1 : 0) + (child->next ? 1 : 0);
			ptr = ensure(p, len + 1);
			if (ptr == NULL) {
				return 0;
			}
			if (child->next) {
				*ptr++ = ',';
			}
			if (fmt) {
				*ptr++ = '\n';
			}
			*ptr = 0;
			p->offset += len;
			child = child->next;
		}
		ptr = ensure(p, fmt ? (depth + 1) : 2);
		if (ptr == NULL) {
			return 0;
		}
		if (fmt) {
			for (i = 0; i < depth - 1; i++) {
				*ptr++ = '\t';
			}
		}
		*ptr++ = '}';
		*ptr = 0;
		out = (p->buffer) + i;
	} else {
		/* Allocate space for the names and the objects */
		entries = (char**)mcJSON_malloc(numentries * sizeof(char*));
		if (entries == NULL) {
			return 0;
		}
		names = (char**)mcJSON_malloc(numentries * sizeof(char*));
		if (names == NULL) {
			mcJSON_free(entries);
			return 0;
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
			str = print_string_ptr(child->string, 0);
			names[i] = str;
			ret = print_value(child, depth, fmt, 0);
			entries[i++] = ret;
			if (str && ret) {
				len += strlen(ret) + strlen(str) + 2 + (fmt ? 2 + depth : 0);
			} else {
				fail = 1;
			}
			child = child->next;
		}

		/* Try to allocate the output string */
		if (!fail) {
			out = (char*)mcJSON_malloc(len);
		}
		if (out == NULL) {
			fail = 1;
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
			return 0;
		}

		/* Compose the output: */
		*out = '{';
		ptr = out + 1;
		if (fmt) {
			*ptr++ = '\n';
		}
		*ptr = 0;
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
			*ptr = 0;
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
		*ptr++ = '}';*ptr++=0;
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
	while (c && mcJSON_strcasecmp(c->string, string)) {
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
		return 0;
	}
	memcpy(ref, item, sizeof(mcJSON));
	ref->string = 0;
	ref->type |= mcJSON_IsReference;
	ref->next = ref->prev = 0;
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
	if (item->string) {
		mcJSON_free(item->string);
	}
	item->string = mcJSON_strdup(string);
	mcJSON_AddItemToArray(object, item);
}
void mcJSON_AddItemToObjectCS(mcJSON *object, const char *string, mcJSON *item) {
	if (item == NULL) {
		return;
	}
	if (!(item->type & mcJSON_StringIsConst) && item->string) {
		mcJSON_free(item->string);
	}
	item->string = (char*)string;
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
		return 0;
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
	c->next = 0;
	c->prev = 0;
	return c;
}
void mcJSON_DeleteItemFromArray(mcJSON *array, int which) {
	mcJSON_Delete(mcJSON_DetachItemFromArray(array, which));
}
mcJSON *mcJSON_DetachItemFromObject(mcJSON *object, const char *string) {
	int i = 0;
	mcJSON *c = object->child;
	while (c && mcJSON_strcasecmp(c->string, string)) {
		i++;
		c = c->next;
	}
	if (c) {
		return mcJSON_DetachItemFromArray(object, i);
	}
	return 0;
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
	c->prev = 0;
	c->next = 0;
	mcJSON_Delete(c);
}
void   mcJSON_ReplaceItemInObject(mcJSON *object, const char *string, mcJSON *newitem) {
	int i = 0;
	mcJSON *c = object->child;
	while (c && mcJSON_strcasecmp(c->string, string)) {
		i++;
		c = c->next;
	}
	if (c) {
		newitem->string = mcJSON_strdup(string);
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
		item->valuestring = mcJSON_strdup(string);
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
	mcJSON *n = 0;
	mcJSON *p = 0;
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
	mcJSON *n = 0;
	mcJSON *p = 0;
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
	mcJSON *n = 0;
	mcJSON *p = 0;
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
	mcJSON *n = 0;
	mcJSON *p = 0;
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
	mcJSON *nptr = 0;
	mcJSON *newchild;
	/* Bail on bad ptr */
	if (item == NULL) {
		return 0;
	}
	/* Create new item */
	newitem = mcJSON_New_Item();
	if (newitem == NULL) {
		return 0;
	}
	/* Copy over all vars */
	newitem->type = item->type & (~mcJSON_IsReference);
	newitem->valueint = item->valueint;
	newitem->valuedouble = item->valuedouble;
	if (item->valuestring) {
		newitem->valuestring = mcJSON_strdup(item->valuestring);
		if (newitem->valuestring == NULL) {
			mcJSON_Delete(newitem);
			return 0;
		}
	}
	if (item->string) {
		newitem->string = mcJSON_strdup(item->string);
		if (newitem->string == NULL) {
			mcJSON_Delete(newitem);
			return 0;
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
			return 0;
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
	*into = 0; /* and null-terminate. */
}
