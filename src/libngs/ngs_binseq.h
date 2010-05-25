/**
 *
 */

#ifndef __NGS_BINSEQ_H__
#define __NGS_BINSEQ_H__

#include <glib.h>

#define BITS_PER_BYTE   8
#define BITS_PER_NUC    2
#define NUCS_PER_BYTE   (BITS_PER_BYTE / BITS_PER_NUC)

typedef enum
{
  NUC_A  = 0,
  NUC_C  = 1,
  NUC_G  = 2,
  NUC_T  = 3,
  NUC_NB = 4
}
Nucleotide;

/**********/
/* BinSeq */
/**********/

typedef struct _BinSeq BinSeq;

struct _BinSeq
{
  char              *name;
  char              *seq;
  unsigned long int  size; /* Size in nucleotides */
};


BinSeq* bin_seq_new  (char              *name,
                      char              *seq,
                      unsigned long int  size);

void    bin_seq_free (BinSeq            *bin);

char*   char_to_bin  (char              *seq,
                      unsigned long int  size);

char*   bin_to_char  (char              *seq,
                      unsigned long int  size);

#endif /* __NGS_BINSEQ_H__ */

/* vim:ft=c:expandtab:sw=4:ts=4:sts=4:cinoptions={.5s^-2n-2(0:
 */
