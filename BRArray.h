//
//  BRArray.h
//
//  Created by Aaron Voisine on 11/14/15.
//  Copyright (c) 2015 breadwallet LLC.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BRArray_h
#define BRArray_h

#include <stdlib.h>
#include <string.h>

// growable arrays with type checking

#define array_new(array, capacity) do {\
    (array) = (void *)((size_t *)calloc(1, (capacity)*sizeof(*(array)) + sizeof(size_t)*2) + 2);\
    array_capacity(array) = (capacity);\
    array_count(array) = 0;\
} while (0)

#define array_capacity(array) (((size_t *)(array))[-2])

#define array_set_capacity(array, capacity) do {\
    (array) = (void *)((size_t *)realloc((size_t *)(array) - 2, (capacity)*sizeof(*(array)) + sizeof(size_t)*2) + 2);\
    if ((capacity) > array_capacity(array))\
        memset((array) + array_capacity(array), 0, ((capacity) - array_capacity(array))*sizeof(*(array)));\
    array_capacity(array) = (capacity);\
} while (0)

#define array_count(array) (((size_t *)(array))[-1])

#define array_set_count(array, count) do {\
    if ((count) > array_capacity(array))\
        array_set_capacity(array, count);\
    if ((count) < array_count(array))\
        memset((array) + (count), 0, (array_count(array) - (count))*sizeof(*(array)));\
    array_count(array) = (count);\
} while (0)

#define array_last(array) ((array)[array_count(array) - 1])

#define array_idx(array, item) ((item) - (array))

#define array_add(array, item) do {\
    if (array_count(array) + 1 > array_capacity(array))\
        array_set_capacity(array, (array_capacity(array) + 1)*3/2);\
    (array)[array_count(array)++] = (item);\
} while (0)

#define array_add_array(array, other_array, count) do {\
    if (array_count(array) + (count) > array_capacity(array))\
        array_set_capacity(array, (array_count(array) + (count))*3/2);\
    memcpy((array) + array_count(array), (other_array), sizeof(*(array))*(count));\
    array_count(array) += (count);\
} while (0)

#define array_insert(array, idx, item) do {\
    if (array_count(array) + 1 > array_capacity(array))\
        array_set_capacity(array, (array_capacity(array) + 1)*3/2);\
    memmove((array) + (idx) + 1, (array) + (idx), (array_count(array)++ - (idx))*sizeof(*(array)));\
    (array)[(idx)] = (item);\
} while (0)

#define array_insert_array(array, idx, other_array, count) do {\
    if (array_count(array) + (count) > array_capacity(array))\
        array_set_capacity(array, (array_count(array) + (count))*3/2;\
    memmove((array) + (idx) + (count), (array) + (idx), (array_count(array) - (idx))*sizeof(*(array)));\
    memcpy((array) + (idx), (other_array), sizeof(*(array))*(count));\
    array_count(array) += (count);\
} while (0)

#define array_rm(array, idx) do {\
    memmove((array) + (idx), (array) + (idx) + 1, (--array_count(array) - (idx))*sizeof(*(array)));\
    memset((array) + array_count(array), 0, sizeof(*(array)));\
} while (0)

#define array_rm_last(array) do {\
    if (array_count(array) > 0)\
        memset((array) + --array_count(array), 0, sizeof(*(array)));\
} while(0)

#define array_rm_range(array, idx, len) do {\
    memmove((array) + (idx), (array) + (idx) + (len), (array_count(array) - ((idx) + (len)))*sizeof(*(array)));\
    array_count(array) -= (len);\
    memset((array) + array_count(array), 0, (len)*sizeof(*(array)));\
} while(0)

#define array_clear(array) do {\
    memset((array), 0, array_count(array)*sizeof(*(array)));\
    array_count(array) = 0;\
} while (0)

#define array_free(array) free((size_t *)(array) - 2)

#endif // BRArray_h
