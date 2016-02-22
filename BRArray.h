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

#ifdef __cplusplus
extern "C" {
#endif

// growable arrays with type checking

#define array_new(array, capacity) do {\
    size_t _array_cap = (capacity);\
    (array) = (void *)((size_t *)calloc(1, _array_cap*sizeof(*(array)) + sizeof(size_t)*2) + 2);\
    array_capacity(array) = _array_cap;\
    array_count(array) = 0;\
} while (0)

#define array_capacity(array) (((size_t *)(array))[-2])

#define array_set_capacity(array, capacity) do {\
    size_t _array_cap = (capacity);\
    (array) = (void *)((size_t *)realloc((size_t *)(array) - 2, _array_cap*sizeof(*(array)) + sizeof(size_t)*2) + 2);\
    if (_array_cap > array_capacity(array))\
        memset((array) + array_capacity(array), 0, (_array_cap - array_capacity(array))*sizeof(*(array)));\
    array_capacity(array) = _array_cap;\
} while (0)

#define array_count(array) (((size_t *)(array))[-1])

#define array_set_count(array, count) do {\
    size_t _array_cnt = (count);\
    if (_array_cnt > array_capacity(array))\
        array_set_capacity(array, _array_cnt);\
    if (_array_cnt < array_count(array))\
        memset((array) + _array_cnt, 0, (array_count(array) - _array_cnt)*sizeof(*(array)));\
    array_count(array) = _array_cnt;\
} while (0)

#define array_idx(array, item) ((item) - (array))

#define array_add(array, item) do {\
    if (array_count(array) + 1 > array_capacity(array))\
        array_set_capacity(array, (array_capacity(array) + 1)*3/2);\
    (array)[array_count(array)++] = (item);\
} while (0)

#define array_add_array(array, other_array, count) do {\
    size_t _array_cnt = (count), _array_i = array_count(array), _array_j = 0;\
    if (_array_i + _array_cnt > array_capacity(array))\
        array_set_capacity(array, (_array_i + _array_cnt)*3/2);\
    while (_array_j < _array_cnt)\
        (array)[_array_i++] = (other_array)[_array_j++];\
    array_count(array) += _array_cnt;\
} while (0)

#define array_insert(array, idx, item) do {\
    size_t _array_idx = (idx), _array_i = ++array_count(array);\
    if (_array_i > array_capacity(array))\
        array_set_capacity(array, (array_capacity(array) + 1)*3/2);\
    while (--_array_i > _array_idx)\
        (array)[_array_i] = (array)[_array_i - 1];\
    (array)[_array_idx] = (item);\
} while (0)

#define array_insert_array(array, idx, other_array, count) do {\
    size_t _array_idx = (idx), _array_cnt = (count), _array_i = array_count(array) + _array_cnt, _array_j = 0;\
    if (_array_i > array_capacity(array))\
        array_set_capacity(array, _array_i*3/2);\
    while (_array_i > _array_idx + _array_cnt)\
        (array)[_array_i - 1] = (array)[(_array_i - 1) - _array_cnt], _array_i--;\
    while (_array_j < _array_cnt)\
        (array)[_array_idx + _array_j] = (other_array)[_array_j], _array_j++;\
    array_count(array) += _array_cnt;\
} while (0)

#define array_rm(array, idx) do {\
    size_t _array_i = (idx);\
    array_count(array)--;\
    while (_array_i < array_count(array))\
        (array)[_array_i] = (array)[_array_i + 1], _array_i++;\
    memset((array) + _array_i, 0, sizeof(*(array)));\
} while (0)

#define array_rm_last(array) do {\
    if (array_count(array) > 0)\
        memset((array) + --array_count(array), 0, sizeof(*(array)));\
} while(0)

#define array_rm_range(array, idx, len) do {\
    size_t _array_i = (idx), _array_len = (len);\
    array_count(array) -= _array_len;\
    while (_array_i < array_count(array))\
        (array)[_array_i] = (array)[_array_i + _array_len], _array_i++;\
    memset((array) + _array_i, 0, _array_len*sizeof(*(array)));\
} while(0)

#define array_clear(array) do {\
    memset((array), 0, array_count(array)*sizeof(*(array)));\
    array_count(array) = 0;\
} while (0)

#define array_free(array) free((size_t *)(array) - 2)

#ifdef __cplusplus
}
#endif

#endif // BRArray_h
