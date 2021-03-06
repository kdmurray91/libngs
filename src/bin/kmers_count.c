/* Copyright (C) 2010  Sylvain FORET
 *
 * This file is part of libngs.
 *
 * libngs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *                                                                       
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *                                                                       
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ngs_binseq.h"
#include "ngs_fasta.h"
#include "ngs_fastq.h"
#include "ngs_kmerhash.h"
#include "ngs_memalloc.h"
#include "ngs_utils.h"


typedef struct _CallbackData CallbackData;

typedef int (*IterCharSeqFunc) (CallbackData        *data,
                                const char          *seq,
                                unsigned long int    size);

struct _CallbackData
{
  char              *input_path;
  char              *output_path;
  GIOChannel        *output_channel;

  KmerHashTable     *htable;
  unsigned char     *tmp_kmer;

  unsigned long int  n_seqs;
  unsigned long int  freq_report;
  unsigned int       k;
  unsigned int       k_bytes;
  int                do_revcomp;
  int                is_fastq;
  int                bin_out;
  int                verbose;
};

/* Default Functions */

static void              parse_args                 (CallbackData        *data,
                                                     int                 *argc,
                                                     char              ***argv);

static int               iter_func_fasta            (FastaSeq            *fasta,
                                                     CallbackData        *data);

static int               iter_func_fastq            (FastqSeq            *fasta,
                                                     CallbackData        *data);

static int               iter_func_char             (char                *seq,
                                                     unsigned long int    size,
                                                     CallbackData        *data);

static int               iter_char_seq              (CallbackData        *data,
                                                     const char          *seq,
                                                     unsigned long int    size);

static void              print_results              (CallbackData        *data);

int
main (int    argc,
      char **argv)
{
  CallbackData  data;
  GError       *error = NULL;

  parse_args (&data, &argc, &argv);

  if (data.is_fastq)
    iter_fastq (data.input_path,
                (FastqIterFunc)iter_func_fastq,
                &data,
                &error);
  else
    iter_fasta (data.input_path,
                (FastaIterFunc)iter_func_fasta,
                &data,
                &error);
  if (error)
    {
      g_printerr ("[ERROR] Iterating sequences failed: %s\n", error->message);
      g_error_free (error);
      error = NULL;
      return 1;
    }

  print_results (&data);

  if (data.htable)
    kmer_hash_table_destroy (data.htable);
  if (data.output_path)
    g_free (data.output_path);
  if (data.tmp_kmer)
    g_free (data.tmp_kmer);

  return 0;
}

static void
parse_args (CallbackData      *data,
            int               *argc,
            char            ***argv)
{
  GOptionEntry entries[] =
    {
      {"out",      'o', 0, G_OPTION_ARG_FILENAME, &data->output_path, "Output file", NULL},
      {"kmer",     'k', 0, G_OPTION_ARG_INT,      &data->k,           "K-mer size", NULL},
      {"freqrep",  'e', 0, G_OPTION_ARG_INT,      &data->freq_report, "Verbose-report frequency", NULL},
      {"revcomp",  'r', 0, G_OPTION_ARG_NONE,     &data->do_revcomp,  "Also scan the reverse complement", NULL},
      {"fastq",    'q', 0, G_OPTION_ARG_NONE,     &data->is_fastq,    "Input is in fastq format", NULL},
      {"binout",   'b', 0, G_OPTION_ARG_NONE,     &data->bin_out,     "Write output in binary format", NULL},
      {"verbose",  'v', 0, G_OPTION_ARG_NONE,     &data->verbose,     "Verbose output", NULL},
      {NULL}
    };
  GError         *error = NULL;
  GOptionContext *context;

  data->k           = 31;
  data->do_revcomp  = 0;
  data->is_fastq    = 0;
  data->bin_out     = 0;
  data->verbose     = 0;
  data->n_seqs      = 0;
  data->freq_report = 1000000;
  data->output_path = strdup("-");
  data->htable      = NULL;
  data->tmp_kmer    = NULL;

  context = g_option_context_new ("FILE - Count the number of kmers in a fasta/fastq file");
  g_option_context_add_group (context, get_fasta_option_group ());
  g_option_context_add_group (context, get_fastq_option_group ());
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, argc, argv, &error))
    {
      g_printerr ("[ERROR] Option parsing failed: %s\n", error->message);
      exit (1);
    }
  g_option_context_free (context);

  if (*argc < 2)
    {
      g_printerr ("[ERROR] No input file provided\n");
      exit (1);
    }

  if (data->k < 1)
    {
      g_printerr ("[ERROR] The kmer size must be larger than 0\n");
      exit (1);
    }

  data->k_bytes    = (data->k + NUCS_PER_BYTE - 1) / NUCS_PER_BYTE;
  data->input_path = (*argv)[1];

  data->htable   = kmer_hash_table_new (data->k);
  data->tmp_kmer = g_malloc0 (MAX (KMER_VAL_BYTES, data->k_bytes));

  if (data->verbose)
    g_printerr ("Parsing %s with k = %d\n", data->input_path, data->k);
}

static int
iter_func_fasta (FastaSeq     *fasta,
                 CallbackData *data)
{
  return iter_func_char (fasta->seq, fasta->size, data);
}

static int
iter_func_fastq (FastqSeq     *fastq,
                 CallbackData *data)
{
  return iter_func_char (fastq->seq, fastq->size, data);
}

static int
iter_func_char (char              *seq,
                unsigned long int  size,
                CallbackData      *data)
{
  int            ret;

  ret = iter_char_seq (data, seq, size);
  if (ret != 1)
    return ret;
  if (data->do_revcomp)
    ret = iter_char_seq (data, rev_comp_in_place (seq, size), size);

  data->n_seqs++;
  if (data->verbose && data->n_seqs % data->freq_report == 0)
    g_printerr ("Parsed %ld sequences\n", data->n_seqs);

  return ret;
}

static void
print_results (CallbackData *data)
{
  GError *error = NULL;

  if (data->verbose)
    g_printerr ("Writing results to %s\n", data->output_path);

  kmer_hash_table_print (data->htable,
                         data->output_path,
                         data->bin_out,
                         &error);

  if (error)
    {
      g_printerr ("[ERROR] Closing output file `%s' failed: %s\n",
                  data->output_path,
                  error->message);
      g_error_free (error);
    }
}

#undef DIGITS_BUFF_SPACE

static int
iter_char_seq (CallbackData     *data,
               const char       *seq,
               unsigned long int size)
{
  unsigned long int       i;
  unsigned long int       j;
  unsigned long int       k;
  const unsigned long int maxi = size - data->k + 1;

  if (size < data->k)
    return 1;

  /* Find the first index of a kmer without Ns */
  for (j = 0, k = 0; j < size; j++)
    {
      if (seq[j] == 'N' || seq[j] == 'n')
        k = 0;
      else
        k++;
      if (k == data->k)
        {
          j++;
          break;
        }
    }
  if (k < data->k)
    return 1;
  for (i = j - k; i < maxi; i++)
    {
      char c;

      /* Skip over Ns */
      c = seq[i + data->k - 1];
      if (c == 'N' || c == 'n')
        {
          k = 0;
          for (j = i + data->k; j < size; j++)
            {
              if (seq[j] == 'N' || seq[j] == 'n')
                k = 0;
              else
                k++;
              if (k == data->k)
                {
                  j++;
                  break;
                }
            }
          if (k < data->k)
            return 1;
          i = j - k;
        }
      char_to_bin_prealloc (data->tmp_kmer, seq + i, data->k);
      kmer_hash_table_increment (data->htable, data->tmp_kmer);
    }
  return 1;
}

/* vim:ft=c:expandtab:sw=4:ts=4:sts=4:cinoptions={.5s^-2n-2(0:
 */
