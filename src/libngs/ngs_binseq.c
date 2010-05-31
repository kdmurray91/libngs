/**
 *
 */

#include <stdlib.h>
#include <string.h>

#include "ngs_binseq.h"


typedef enum
{
  NUC_A  = 0,
  NUC_C  = 1,
  NUC_G  = 2,
  NUC_T  = 3,
  NUC_NB = 4
}
Nucleotide;

unsigned char char_to_bin_table[128] =
{
  ['a'] = NUC_A,
  ['c'] = NUC_C,
  ['g'] = NUC_G,
  ['t'] = NUC_T,
  ['A'] = NUC_A,
  ['C'] = NUC_C,
  ['G'] = NUC_G,
  ['T'] = NUC_T,
};

static char bin_to_char_table[NUC_NB] =
{
  [NUC_A] = 'A',
  [NUC_C] = 'C',
  [NUC_G] = 'G',
  [NUC_T] = 'T'
};

BinSeq*
bin_seq_new  (const char        *name,
              const char        *seq,
              unsigned long int  size)
{
  BinSeq *bin;

  bin       = g_slice_new0   (BinSeq);
  bin->name = g_strdup (name);
  bin->size = size;
  bin->seq  = char_to_bin (seq, size);

  return bin;
}

void
bin_seq_free (BinSeq *bin)
{
  if (bin)
    {
      if (bin->name)
        g_free (bin->name);
      if (bin->seq)
        g_free (bin->seq);
      g_slice_free (BinSeq, bin);
    }
}

unsigned char*
char_to_bin (const char        *seq,
             unsigned long int  size)
{
  size_t         byte_size;
  unsigned char *new_seq;

  byte_size = (size + NUCS_PER_BYTE - 1) / NUCS_PER_BYTE;
  new_seq   = g_malloc0 (byte_size);
  new_seq   = char_to_bin_prealloc (new_seq, seq, size);

  return new_seq;
}

unsigned char*
char_to_bin_prealloc (unsigned char     *dest,
                      const char        *src,
                      unsigned long int  size)
{
  unsigned long int i;

  for (i = 0; i < size; i++)
    {
      const unsigned long int address = i / NUCS_PER_BYTE;
      const unsigned int      offset  = (i % NUCS_PER_BYTE) * BITS_PER_NUC;
      dest[address]                  |= char_to_bin_table[(unsigned int)src[i]] << offset;
    }
  return dest;
}

char*
bin_to_char (const unsigned char *seq,
             unsigned long int    size)
{
  char *new_seq;

  /* '\0' terminated */
  new_seq = g_malloc (size + 1);
  new_seq = bin_to_char_prealloc (new_seq, seq, size);
  new_seq[size] = '\0';

  return new_seq;
}

char*  
bin_to_char_prealloc  (char                *dest,
                       const unsigned char *src,
                       unsigned long int    size)
{
  unsigned long int i;

  for (i = 0; i < size; i++)
    {
      const unsigned long int address = i / NUCS_PER_BYTE;
      const unsigned int      offset  = (i % NUCS_PER_BYTE) * BITS_PER_NUC;
      const int               val     = (src[address] >> offset) & 3;
      dest[i]                         = bin_to_char_table[val];
    }
  return dest;
}

unsigned char*
bin_revcomp_mult4 (const unsigned char *seq,
                   unsigned long int    size)
{
  unsigned char *revcomp;

  revcomp = g_malloc0 (size / NUCS_PER_BYTE);
  revcomp = bin_revcomp_prealloc_mult4 (revcomp, seq, size);

  return revcomp;
}

unsigned char*
bin_revcomp_prealloc_mult4 (unsigned char       *dest,
                            const unsigned char *src,
                            unsigned long int    size)
{
  unsigned long int       i;
  const unsigned long int n_bytes = size / NUCS_PER_BYTE;

  for (i = 0; i < n_bytes; i++)
    dest[n_bytes - i - 1] = revcomp_table[src[i]];

  return dest;
}

unsigned char*
bin_revcomp_inplace_mult4 (unsigned char     *seq,
                           unsigned long int  size)
{
  unsigned long int       i;
  const unsigned long int maxi = (size / NUCS_PER_BYTE + 1) / 2;

  for (i = 0; i < maxi; i++)
    {
      const unsigned long int comp_idx = size - i - 1;
      const unsigned char     tmp      = revcomp_table[seq[i]];

      seq[i]        = revcomp_table[seq[comp_idx]];
      seq[comp_idx] = tmp;
    }

  return seq;
}

/* vim:ft=c:expandtab:sw=4:ts=4:sts=4:cinoptions={.5s^-2n-2(0:
 */
