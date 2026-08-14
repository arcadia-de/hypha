#ifndef HYPHA_BITSET_H
#define HYPHA_BITSET_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint64_t* words;
  size_t num_bits;
  size_t num_words;
} BitSet;

void InitBitSet(BitSet* bs, const size_t num_bits);
void FreeBitSet(BitSet* bs);
void BitSetSet(BitSet* bs, size_t index);
void BitSetReset(BitSet* bs, size_t index);
bool BitSetTest(const BitSet* bs, size_t index);
void BitSetFlip(BitSet* bs, size_t index);
void BitSetSetAll(BitSet* bs);
void BitSetClearAll(BitSet* bs);
void BitSetPrint(const BitSet* bs);
size_t BitSetCount(const BitSet* bs);

static inline BitSet* NewBitSet(const size_t num_bits) {
  BitSet* bs = (BitSet*)malloc(sizeof(BitSet));
  if (bs)
    InitBitSet(bs, num_bits);
  return bs;
}

#endif
