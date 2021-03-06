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
#include <string.h>

#include "ngs_bsq.h"


typedef struct _CallbackData CallbackData;

struct _CallbackData
{
  char *source;
  char *input_path;
};

static void parse_args (CallbackData      *data,
                        int               *argc,
                        char            ***argv);

static int  iter_func  (BsqRecord         *rec,
                        CallbackData      *data);

int
main (int    argc,
      char **argv)
{
  CallbackData data;
  GError      *error = NULL;

  parse_args (&data, &argc, &argv);

  iter_bsq (data.input_path,
            (BsqIterFunc)iter_func,
            &data,
            &error);

  if (error)
    {
      g_printerr ("[ERROR] Iterating sequences failed: %s\n", error->message);
      g_error_free (error);
      error = NULL;
      return 1;
    }

  return 0;
}

static void
parse_args (CallbackData      *data,
            int               *argc,
            char            ***argv)
{
  GOptionEntry entries[] =
    {
      /* TODO Add an option to select what to print depending on the BsqMapFlag */
      /* TODO Add an option to select what to print depending on the Strand */
      {"source", 's', 0, G_OPTION_ARG_STRING, &data->source, "Read source", NULL},
      {NULL}
    };
  GError         *error = NULL;
  GOptionContext *context;

  data->source = NULL;

  context = g_option_context_new ("FILE - Converts a bsq file into a gff file");
  g_option_context_add_group (context, get_bsq_option_group ());
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

  if (!data->source)
    data->source = "bsread";
}

static int
iter_func (BsqRecord    *rec,
           CallbackData *data)
{
  int name_len;

  name_len = strlen (rec->name);
  if (rec->flag == BSQ_MAP_UM &&
      ((rec->name[name_len - 2] == '/' &&
       rec->name[name_len - 1] == '1' &&
       (rec->strand == BSQ_STRAND_W ||
        rec->strand == BSQ_STRAND_C)) ||
      (rec->name[name_len - 2] == '/' &&
       rec->name[name_len - 1] == '2' &&
       (rec->strand == BSQ_STRAND_WC ||
        rec->strand == BSQ_STRAND_CC))))
    {
      g_print ("%s\t"        /* Reference */
               "%s\t"        /* Source */
               "bsmap\t"     /* Method */
               "%ld\t"       /* From */
               "%ld\t"       /* To */
               ".\t"         /* Score */
               "%s\t"        /* Strand */
               ".\t"         /* Phase */
               "read %s\n",  /* Group */
               rec->ref,
               data->source,
               rec->loc,
               rec->loc + rec->size,
               (rec->strand == BSQ_STRAND_W ||
                rec->strand == BSQ_STRAND_C) ? "+" : "-",
               rec->name);
    }
  return 1;
}

/* vim:ft=c:expandtab:sw=4:ts=4:sts=4:cinoptions={.5s^-2n-2(0:
 */
