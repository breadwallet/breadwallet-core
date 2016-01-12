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
// int (*compare)(void *info, const void *a, const void *b); // assign this to a comparator function
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
    (head) = (void *)((void **)calloc(1, sizeof(void **) + sizeof(*(head))) + 1);\
    *(head) = (value);\
} while(0)

#define list_next(item) (((void **)(item))[-1])

#define list_insert_head(head, value) do {\
    void *_list_tmp = (head);\
    list_new(head, value);\
    list_next(head) = _list_tmp;\
} while(0)

#define list_insert_after(item, value) do {\
    void *_list_tmp = (item);\
    list_new(item, value);\
    list_next(item) = list_next(_list_tmp);\
    list_next(_list_tmp) = (item);\
    (item) = _list_tmp;\
} while(0)

#define list_rm_head(head) do {\
    void *_list_tmp = (head);\
    (head) = list_next(_list_tmp);\
    free(&list_next(_list_tmp));\
} while(0)

#define list_rm_after(item) do {\
    void *_list_tmp = list_next(item);\
    if (_list_tmp)\
        list_next(item) = list_next(_list_tmp), free(&list_next(_list_tmp));\
} while(0)

#define list_free(head) do {\
    while (head)\
        list_rm_head(head);\
} while(0)

#define list_sort(head, info, comparator) do {\
    (head) = _list_sort(head, info, comparator);\
} while(0)

inline static void *_list_sort(void *head, void *info, int (*comparator)(void *info, const void *a, const void *b))
{
    if (! head || ! list_next(head)) return head;

    void *node[1];
    void *item = node + 1, *middle = head, *end = head, *split;
    
    while (list_next(end) && list_next(list_next(end))) {
        middle = list_next(middle);
        end = list_next(list_next(end));
    }

    split = list_next(middle);
    list_next(middle) = NULL;
    list_sort(head, info, comparator);
    list_sort(split, info, comparator);
    
    while (head && split) {
        if (comparator(info, head, split) <= 0) {
            list_next(item) = head;
            head = list_next(head);
        }
        else {
            list_next(item) = split;
            split = list_next(split);
        }
        
        item = list_next(item);
    }
    
    list_next(item) = (head) ? head : split;
    return node[0];
}

#endif // BRList_h
