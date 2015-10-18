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

#include "mcJSON.h"

#ifndef mcJSON_UTILS__H
#define mcJSON_UTILS__H

#ifdef __cplusplus
extern "C" {
#endif

/* Implement RFC6901 (https://tools.ietf.org/html/rfc6901) JSON Pointer spec.	*/
mcJSON *mcJSONUtils_GetPointer(mcJSON *object,const char *pointer);

/* Implement RFC6902 (https://tools.ietf.org/html/rfc6902) JSON Patch spec.		*/
mcJSON* mcJSONUtils_GeneratePatches(mcJSON *from,mcJSON *to);
void mcJSONUtils_AddPatchToArray(mcJSON *array,const char *op,const char *path,mcJSON *val);	/* Utility for generating patch array entries.	*/
int mcJSONUtils_ApplyPatches(mcJSON *object,mcJSON *patches);	/* Returns 0 for success. */

/*
// Note that ApplyPatches is NOT atomic on failure. To implement an atomic ApplyPatches, use:
//int mcJSONUtils_AtomicApplyPatches(mcJSON **object, mcJSON *patches)
//{
//	mcJSON *modme=mcJSON_Duplicate(*object,1);
//	int error=mcJSONUtils_ApplyPatches(modme,patches);
//	if (!error)	{mcJSON_Delete(*object);*object=modme;}
//	else		mcJSON_Delete(modme);
//	return error;
//}
// Code not added to library since this strategy is a LOT slower.
*/

char *mcJSONUtils_FindPointerFromObjectTo(mcJSON *object,mcJSON *target);	/* Given a root object and a target object, construct a pointer from one to the other.	*/

void mcJSONUtils_SortObject(mcJSON *object);	/* Sorts the members of the object into alphabetical order.	*/

#ifdef __cplusplus
}
#endif

#endif
