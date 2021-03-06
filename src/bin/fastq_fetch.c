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

#include "ngs_fastq.h"

typedef struct _CallbackData CallbackData;

struct _CallbackData
{
  char       *input_path;
  char       *output_path;

  gchar      *querries_path;
  gchar      *querries_string;
  gchar     **querries_array;
  GHashTable *querries_hash;
};

static int  iter_func      (FastqSeq       *fastq,
                            CallbackData   *data);

static void parse_args     (CallbackData   *data,
                            int            *argc,
                            char         ***argv);

static void load_queries   (CallbackData   *data);

static void print_querries (CallbackData   *data);

static void free_querries  (CallbackData   *data);

int
main (int    argc,
      char **argv)
{
  CallbackData data;
  GError      *error = NULL;

  parse_args (&data, &argc, &argv);

  iter_fastq (data.input_path,
              (FastqIterFunc)iter_func,
              &data,
              &error);
  if (error)
    {
      g_printerr ("[ERROR] Iterating sequences failed: %s\n", error->message);
      g_error_free (error);
      error = NULL;
    }
  else
    print_querries (&data);

  free_querries (&data);

  return 0;
}

static void
parse_args (CallbackData   *data,
            int            *argc,
            char         ***argv)
{
  GOptionEntry entries[] =
    {
      {"search", 's', 0, G_OPTION_ARG_STRING, &data->querries_string, "list of coma-separated querries", NULL},
      {"input",  'i', 0, G_OPTION_ARG_STRING, &data->querries_path,   "file containing one querry per line", NULL},
      {"out",    'o', 0, G_OPTION_ARG_FILENAME, &data->output_path, "Output file", NULL},
      {NULL}
    };
  GError         *error = NULL;
  GOptionContext *context;

  data->output_path     = "-";

  data->querries_path   = NULL;
  data->querries_string = NULL;
  data->querries_array  = NULL;
  data->querries_hash   = NULL;

  context = g_option_context_new ("FILE - Retrieves reads from a fastq file");
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

  data->input_path = (*argv)[1];
  load_queries (data);
}

static void
load_queries (CallbackData *data)
{
  char **tmp;

  if (!data->querries_string && !data->querries_path)
    {
      g_printerr ("[ERROR] Neither -s and -i options were provided. "
                  "One must be provided.");
      exit (1);
    }
  if (data->querries_string && data->querries_path)
    {
      g_printerr ("[ERROR] Both -s and -i options were provided. "
                  "Only one must be provided.");
      exit (1);
    }

  if (data->querries_string)
    data->querries_array = g_strsplit (data->querries_string,  ",", -1);

  if (data->querries_path)
    {
      GIOChannel *channel;
      GError     *error   = NULL;
      gchar      *content = NULL;

      channel = g_io_channel_new_file (data->querries_path, "r", &error);
      if (error)
        {
          g_printerr ("[ERROR] Opening querries file failed: %s\n", error->message);
          exit (1);
        }
      g_io_channel_read_to_end (channel,
                                &content,
                                NULL,
                                &error);
      if (error)
        {
          g_printerr ("[ERROR] Reading querries file failed: %s\n", error->message);
          exit (1);
        }
      g_io_channel_shutdown (channel, FALSE, &error);
      if (error)
        {
          g_printerr ("[ERROR] Closing querries file failed: %s\n", error->message);
          exit (1);
        }
      data->querries_array = g_strsplit (content, "\n", -1);
      g_free (content);
    }

  data->querries_hash = g_hash_table_new_full (g_str_hash,
                                               g_str_equal,
                                               NULL,
                                               (GDestroyNotify)fastq_seq_free);
  for (tmp = data->querries_array; *tmp; tmp++)
    g_hash_table_insert (data->querries_hash, *tmp, NULL);
}

static int
iter_func (FastqSeq     *fastq,
           CallbackData *data)
{
  if (g_hash_table_lookup_extended (data->querries_hash, fastq->name, NULL, NULL))
    g_hash_table_insert (data->querries_hash, fastq->name, fastq_seq_copy (fastq));

  return 1;
}

static void
print_querries (CallbackData *data)
{
  GIOChannel *channel;
  char      **tmp;
  GError     *error      = NULL;
  int         use_stdout = 1;

  if (data->output_path[0] == '-' && data->output_path[1] == '\0')
    channel = g_io_channel_unix_new (STDOUT_FILENO);
  else
    {
      use_stdout = 0;
      channel    = g_io_channel_new_file (data->output_path, "w", &error);
      if (error)
        {
          g_printerr ("[ERROR] Opening output file `%s' failed: %s\n",
                      data->output_path,
                      error->message);
          exit (1);
        }
    }

  for (tmp = data->querries_array; *tmp; tmp++)
    {
      FastqSeq *seq;

      seq = g_hash_table_lookup (data->querries_hash, *tmp);
      if (seq)
        {
          GString *buffer;

          buffer = g_string_sized_new (512);
          buffer = g_string_append_c (buffer, '@');
          buffer = g_string_append (buffer, seq->name);
          buffer = g_string_append_c (buffer, '\n');

          buffer = g_string_append (buffer, seq->seq);
          buffer = g_string_append_c (buffer, '\n');

          buffer = g_string_append_c (buffer, '+');
          buffer = g_string_append (buffer, seq->name);
          buffer = g_string_append_c (buffer, '\n');

          buffer = g_string_append (buffer, seq->qual);
          buffer = g_string_append_c (buffer, '\n');

          g_io_channel_write_chars (channel,
                                    buffer->str,
                                    -1,
                                    NULL,
                                    &error);
          g_string_free (buffer, TRUE);
          if (error)
            {
              g_printerr ("[ERROR] Writing sequence failed: %s\n",
                          error->message);
              g_error_free (error);
              error = NULL;
              break;
            }
        }
    }

  if (!use_stdout)
    {
      g_io_channel_shutdown (channel, TRUE, &error);
      if (error)
        {
          g_printerr ("[ERROR] Closing output file `%s' failed: %s\n",
                      data->output_path,
                      error->message);
          g_error_free (error);
          error = NULL;
        }
    }
  g_io_channel_unref (channel);
}

static void
free_querries (CallbackData *data)
{
  if (data->querries_array)
    g_strfreev (data->querries_array);
  if (data->querries_hash)
    g_hash_table_destroy (data->querries_hash);
}

/* vim:ft=c:expandtab:sw=4:ts=4:sts=4:cinoptions={.5s^-2n-2(0:
 */
