#ifndef BITVEC_H
#define BITVEC_H

#include <stdlib.h>


/*
 * Defines an infinite bit vector structure.  Assumes all bits are clear until
 * set.  Dynamically expands when a bit is set higher than current storage can
 * represent.
 */

#define BITVEC_DEFAULT_SIZE 65536

struct bitvec {
	unsigned char *array;
	int size;
};

static inline int bitvec_init(struct bitvec *b)
{
	unsigned char *newarray;

	newarray = malloc(BITVEC_DEFAULT_SIZE);
	if (!newarray)
		return -1;
	b->array = newarray;
	b->size = BITVEC_DEFAULT_SIZE;
	return 0;
}

static inline void bitvec_destroy(struct bitvec *b)
{
	free(b->array);
}

static inline int bitvec_expand(struct bitvec *b, int min_byte)
{
	unsigned char *newarray;
	int newsize = b->size;

	while (min_byte >= newsize)
		newsize *= 2;
	newarray = realloc(b->array, newsize);
	if (!newarray)
		return -1;
	memset(newarray + b->size, '\0', newsize - b->size);
	b->array = newarray;
	b->size = newsize;
	return 0;
}

static inline int bitvec_test(struct bitvec *b, int bit)
{
	int byte = bit / 8;
	int subbit = bit % 8;

	if (byte >= b->size)
		return 0;
	return (b->array[byte] & (1 << subbit)) != 0;
}

static inline int bitvec_set(struct bitvec *b, int bit)
{
	int byte = bit / 8;
	int subbit = bit % 8;

	if (byte >= b->size && bitvec_expand(b, byte))
		return -1;
	b->array[byte] |= (1 << subbit);
	return 0;
}

static inline void bitvec_clear(struct bitvec *b, int bit)
{
	int byte = bit / 8;
	int subbit = bit % 8;

	if (byte >= b->size)
		return;
	b->array[byte] &= ~(1 << subbit);
}


#endif
