#ifndef _MALLOC_H
#define _MALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

void *malloc(unsigned int size);
void *calloc(unsigned int nmemb, unsigned int size);
void *realloc(void *ptr, unsigned int size);
void free(void *ptr);

#ifdef __cplusplus
}  /* extern C */
#endif

#endif
