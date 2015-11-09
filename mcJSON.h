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

#ifndef mcJSON__h
#define mcJSON__h

#include "buffer/buffer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* mcJSON Types: */
typedef enum mcJSON_Type {
	mcJSON_False = 0,
	mcJSON_True = 1,
	mcJSON_NULL = 2,
	mcJSON_Number = 3,
	mcJSON_String = 4,
	mcJSON_Array = 5,
	mcJSON_Object = 6
} mcJSON_Type;

#define mcJSON_IsReference 256
#define mcJSON_StringIsConst 512

/* The mcJSON structure: */
typedef struct mcJSON {
	struct mcJSON *next, *prev; /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
	struct mcJSON *child; /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

	mcJSON_Type type; /* The type of the item, as above. */

	buffer_t * valuestring; /* The item's string, if type==mcJSON_String */
	int valueint; /* The item's number, if type==mcJSON_Number */
	double valuedouble; /* The item's number, if type==mcJSON_Number */

	buffer_t * name; /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} mcJSON;

typedef struct mcJSON_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} mcJSON_Hooks;

/* Supply malloc, realloc and free functions to mcJSON */
extern void mcJSON_InitHooks(mcJSON_Hooks* hooks);


/* Supply a block of JSON, and this returns a mcJSON object you can interrogate. Call mcJSON_Delete when finished. */
extern mcJSON *mcJSON_Parse(buffer_t *json);
/* Render a mcJSON entity to text for transfer/storage. Free the char* when finished. */
extern buffer_t *mcJSON_Print(mcJSON *item);
/* Render a mcJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
extern buffer_t *mcJSON_PrintUnformatted(mcJSON *item);
/* Render a mcJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. format = false gives unformatted, = true gives formatted */
extern buffer_t *mcJSON_PrintBuffered(mcJSON *item, const size_t prebuffer, bool format);
/* Delete a mcJSON entity and all subentities. */
extern void mcJSON_Delete(mcJSON *c);

/* Returns the number of items in an array (or object). */
extern size_t mcJSON_GetArraySize(mcJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
extern mcJSON *mcJSON_GetArrayItem(mcJSON *array, size_t item);
/* Get item "string" from object. */
extern mcJSON *mcJSON_GetObjectItem(mcJSON *object,const char *string);

/* These calls create a mcJSON item of the appropriate type. */
extern mcJSON *mcJSON_CreateNull(void);
extern mcJSON *mcJSON_CreateTrue(void);
extern mcJSON *mcJSON_CreateFalse(void);
extern mcJSON *mcJSON_CreateBool(int b);
extern mcJSON *mcJSON_CreateNumber(double num);
extern mcJSON *mcJSON_CreateString(const char *string);
extern mcJSON *mcJSON_CreateArray(void);
extern mcJSON *mcJSON_CreateObject(void);

/* These utilities create an Array of count items. */
extern mcJSON *mcJSON_CreateIntArray(const int *numbers, size_t count);
extern mcJSON *mcJSON_CreateDoubleArray(const double *numbers, size_t count);
extern mcJSON *mcJSON_CreateStringArray(const char **strings, size_t count);

/* Append item to the specified array/object. */
extern void mcJSON_AddItemToArray(mcJSON *array, mcJSON *item);
extern void mcJSON_AddItemToObject(mcJSON *object,const char *string,mcJSON *item);
extern void mcJSON_AddItemToObjectCS(mcJSON *object,const char *string,mcJSON *item);	/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the mcJSON object */
/* Append reference to item to the specified array/object. Use this when you want to add an existing mcJSON to a new mcJSON, but don't want to corrupt your existing mcJSON. */
extern void mcJSON_AddItemReferenceToArray(mcJSON *array, mcJSON *item);
extern void mcJSON_AddItemReferenceToObject(mcJSON *object, const char *string, mcJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
extern mcJSON *mcJSON_DetachItemFromArray(mcJSON *array, size_t index);
extern void mcJSON_DeleteItemFromArray(mcJSON *array, size_t index);
extern mcJSON *mcJSON_DetachItemFromObject(mcJSON *object,const char *string);
extern void mcJSON_DeleteItemFromObject(mcJSON *object,const char *string);

/* Update array items. */
extern void mcJSON_InsertItemInArray(mcJSON *array, size_t index, mcJSON *newitem);	/* Shifts pre-existing items to the right. */
extern void mcJSON_ReplaceItemInArray(mcJSON *array, size_t index, mcJSON *newitem);
extern void mcJSON_ReplaceItemInObject(mcJSON *object, const char *string, mcJSON *newitem);

/* Duplicate a mcJSON item */
extern mcJSON *mcJSON_Duplicate(mcJSON *item,int recurse);
/* Duplicate will create a new, identical mcJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */

extern void mcJSON_Minify(buffer_t *json);

/* Macros for creating things quickly. */
#define mcJSON_AddNullToObject(object,name)		mcJSON_AddItemToObject(object, name, mcJSON_CreateNull())
#define mcJSON_AddTrueToObject(object,name)		mcJSON_AddItemToObject(object, name, mcJSON_CreateTrue())
#define mcJSON_AddFalseToObject(object,name)		mcJSON_AddItemToObject(object, name, mcJSON_CreateFalse())
#define mcJSON_AddBoolToObject(object,name,b)	mcJSON_AddItemToObject(object, name, mcJSON_CreateBool(b))
#define mcJSON_AddNumberToObject(object,name,n)	mcJSON_AddItemToObject(object, name, mcJSON_CreateNumber(n))
#define mcJSON_AddStringToObject(object,name,s)	mcJSON_AddItemToObject(object, name, mcJSON_CreateString(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define mcJSON_SetIntValue(object,val)			((object)?(object)->valueint=(object)->valuedouble=(val):(val))
#define mcJSON_SetNumberValue(object,val)		((object)?(object)->valueint=(object)->valuedouble=(val):(val))

#ifdef __cplusplus
}
#endif

#endif
