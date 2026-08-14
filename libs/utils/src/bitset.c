#include "hypha/bitset.h"

#define BITSET_WORD_BITS       64ULL
#define BITSET_WORD_INDEX(bit) ((bit) / BITSET_WORD_BITS)
#define BITSET_BIT_OFFSET(bit) ((bit) % BITSET_WORD_BITS)

void InitBitSet(BitSet* bs, size_t total_bits) {
  if (total_bits == 0)
    return;

  bs->num_bits = total_bits;
  bs->num_words = (total_bits + BITSET_WORD_BITS - 1) / BITSET_WORD_BITS;
  bs->words = (uint64_t*)calloc(bs->num_words, sizeof(uint64_t));
}

void FreeBitSet(BitSet* bs) {
  if (!bs)
    return;

  free(bs->words);
}

void BitSetSet(BitSet* bs, size_t idx) {
  if (idx >= bs->num_bits)
    return;

  bs->words[BITSET_WORD_INDEX(idx)] |= (1ULL << BITSET_BIT_OFFSET(idx));
}

void BitSetReset(BitSet* bs, size_t idx) {
  if (idx >= bs->num_bits)
    return;

  bs->words[BITSET_WORD_INDEX(idx)] &= ~(1ULL << BITSET_BIT_OFFSET(idx));
}

bool BitSetTest(const BitSet* bs, size_t idx) {
  if (idx >= bs->num_bits)
    return false;

  uint64_t word = bs->words[BITSET_WORD_INDEX(idx)];
  return (word & (1ULL << BITSET_BIT_OFFSET(idx))) != 0;
}

void BitSetFlip(BitSet* bs, size_t idx) {
  if (idx >= bs->num_bits)
    return;

  bs->words[BITSET_WORD_INDEX(idx)] ^= (1ULL << BITSET_BIT_OFFSET(idx));
}

void BitSetSetAll(BitSet* bs) {
  memset(bs->words, 0xFF, bs->num_words * sizeof(uint64_t));
}

void BitSetClearAll(BitSet* bs) {
  memset(bs->words, 0, bs->num_words * sizeof(uint64_t));
}

size_t BitSetCount(const BitSet* bs) {
  size_t count = 0;
  for (size_t i = 0; i < bs->num_words; i++) {
    count += __builtin_popcountll(bs->words[i]);
  }

  size_t extra_bits = bs->num_bits % BITSET_WORD_BITS;
  if (extra_bits > 0 && bs->num_words > 0) {
    size_t padding = BITSET_WORD_BITS - extra_bits;
    uint64_t trailing_mask = bs->words[bs->num_words - 1] >> extra_bits;
    count -= __builtin_popcountll(trailing_mask);
  }

  return count;
}

void BitSetPrint(const BitSet* bs) {
  for (size_t i = 0; i < bs->num_bits; i++) {
    printf("%d", BitSetTest(bs, i) ? 1 : 0);
    if ((i + 1) % 8 == 0)
      printf(" ");
  }

  printf("\n");
}
