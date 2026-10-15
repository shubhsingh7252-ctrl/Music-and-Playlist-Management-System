#include "sort.h"
#include <stdlib.h>
#include <string.h>

static void msort(void **a, void **tmp, int lo, int hi, int (*cmp)(const void *, const void *)) {
    if (hi - lo < 2) return;
    int mid = lo + (hi - lo) / 2;
    msort(a, tmp, lo, mid, cmp);
    msort(a, tmp, mid, hi, cmp);
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) tmp[k++] = (cmp(a[i], a[j]) <= 0) ? a[i++] : a[j++];
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi) tmp[k++] = a[j++];
    memcpy(a + lo, tmp + lo, sizeof(void *) * (size_t)(hi - lo));
}
void merge_sort(void **a, int n, int (*cmp)(const void *, const void *)) {
    if (n < 2) return;
    void **tmp = malloc(sizeof(void *) * (size_t)n);
    msort(a, tmp, 0, n, cmp);
    free(tmp);
}
