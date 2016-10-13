/* Time-stamp: <emul.c  13 Oct 01 20:21:54> */

/*
  Copyright 2016 Nicolas Bedon 
  This file is part of SIPRO.

  SIPRO is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  SIPRO is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with SIPRO.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include "memory.h"

#define NAME_VERSION "SIPRO V2.0"

extern int startProg ();

bool trace = false;
bool stack = false;

static void
usage (FILE * fdo, const char *name, const char *prog)
{
  fprintf (fdo, NAME_VERSION "\n");
  fprintf (fdo, "Usage: %s [-t] [-d] [-h] file\n", name);
  fprintf (fdo, "\t Runs <file> on SIPRO\n");
  fprintf (fdo, "\t -t: trace mode\n");
  fprintf (fdo, "\t -d: dump memory at end of execution\n");
  fprintf (fdo, "\t -s: print stack in trace mode\n");
  fprintf (fdo, "\t -h: print this message and exit\n");
}

int
main (int argc, char *argv[])
{
  FILE *fdi;
  int opt;
  bool dump = false;
  if (argc < 2)
    {
      usage (stderr, argv[0], argv[1]);
      return EXIT_FAILURE;
    }

  while ((opt = getopt(argc, argv, "tdsh")) != -1) {
    switch (opt) {
    case 't':
      trace = true;
      break;
    case 'd':
      dump = true;
      break;
    case 's':
      stack = true;
    break;
    case 'h':
    default: /* '?' */
      usage (stderr, argv[0], argv[1]);
      exit(EXIT_FAILURE);
    }
  }
	       
  if ((fdi = fopen (argv[optind], "rb")) == NULL)
    {
      fprintf (stderr, "Error opening file %s: %s\n",
	       argv[optind], strerror (errno));
      return EXIT_FAILURE;
    }
  
  loadMemoryFromFile (fdi);

  fclose (fdi);

  if (dump)
    dumpMemory ();

  return startProg () == 0;

}
