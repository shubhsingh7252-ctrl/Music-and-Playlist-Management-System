/* sort.h - stable merge sort on an array of pointers (DSA: Sorting, O(n log n)) */
#ifndef SORT_H
#define SORT_H
void merge_sort(void **a, int n, int (*cmp)(const void *, const void *));
#endif
