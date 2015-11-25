//
//  BRList.h
//
//  Created by Aaron Voisine on 11/18/15.
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

#ifndef BRList_h
#define BRList_h

#include <stdlib.h>

// singly-linked list with type checking
//
// example:
//
// int *head, *item;               // linked list of ints
// int (*compare)(void *info, void *a, void *b); // assign this to a comparator function
// ...
//
// list_new(head, 1);              // 1
// list_insert_head(head, 2);      // 2->1
// list_insert_after(head, 3);     // 2->3->1
// list_sort(head, NULL, compare); // 1->2->3 (stable merge sort)
//
// for (item = head; item; item = list_next(item)) {
//     printf("%i, ", *item);      // 1, 2, 3,
// }
//
// item = list_next(head);
// list_rm_after(item);            // 1->2
// list_rm_head(head);             // 2
// list_free(head);                // free any remaining items
//
// note:
// a head pointer initialized to NULL is equivalent to an empty linked list
// list_insert_head() can be used on it without needing to call list_new() first

#define list_new(head, value) do {\
    (head) = calloc(1, sizeof(*(head)) + sizeof(void *));\
    *(head) = (value);\
} while(0)

#define list_next(item) _list_next(item, sizeof(*(item)))

#define list_next_next(item) _list_next(_list_next(item, sizeof(*(item))), sizeof(*(item)))

#define _list_next(item, item_sz) (*((void **)((char *)(item) + (item_sz))))

#define list_insert_head(head, value) do {\
    void *_list_itm = (head);\
    list_new(head, value);\
    list_next(head) = _list_itm;\
} while(0)

#define list_insert_after(item, value) do {\
    void *_list_itm = (item), *_list_nxt = list_next(item);\
    list_new(item, value);\
    list_next(item) = _list_nxt;\
    _list_nxt = (item);\
    (item) = _list_itm;\
    list_next(item) = _list_nxt;\
} while(0)

#define list_rm_head(head) do {\
    void *_list_itm = (head);\
    (head) = list_next(head);\
    free(_list_itm);\
} while(0)

#define list_rm_after(item) do {\
    void *_list_itm = (item), *_list_nxt = NULL;\
    (item) = list_next(item);\
    if (item)\
        _list_nxt = list_next(item), free(item);\
    (item) = _list_itm;\
    list_next(item) = _list_nxt;\
} while(0)

#define list_free(head) do {\
    while (head)\
        list_rm_head(head);\
} while(0)

#define list_sort(head, info, comparator) do {\
    (head) = _list_sort((head), (info), (comparator), sizeof(*(head)));\
} while(0)

inline static void *_list_sort(void *head, void *info, int (*comparator)(void *info, void *a, void *b), size_t item_sz)
{
    if (! head || ! _list_next(head, item_sz)) return head;

    char node[item_sz + sizeof(void *)];
    void *item = node, *middle = head, *end = head, *split;
    
    while (_list_next(end, item_sz) && _list_next(_list_next(end, item_sz), item_sz)) {
        middle = _list_next(middle, item_sz);
        end = _list_next(_list_next(end, item_sz), item_sz);
    }

    split = _list_next(middle, item_sz);
    _list_next(middle, item_sz) = NULL;
    head = _list_sort(head, info, comparator, item_sz);
    split = _list_sort(split, info, comparator, item_sz);
    
    while (head && split) {
        if (comparator(info, head, split) <= 0) {
            _list_next(item, item_sz) = head;
            head = _list_next(head, item_sz);
        }
        else {
            _list_next(item, item_sz) = split;
            split = _list_next(split, item_sz);
        }
        
        item = _list_next(item, item_sz);
    }
    
    _list_next(item, item_sz) = (head) ? head : split;
    return _list_next(node, item_sz);
}

#endif // BRList_h
