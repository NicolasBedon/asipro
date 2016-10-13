/* Time-stamp: <asm.c   7 Oct 01 12:45:12> */

/*
  Copyright 2001-2016 Nicolas Bedon 
  This file is part of ASIPRO.

  ASIPRO is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  ASIPRO is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with ASIPRO.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "analyse.h"

#define NAME_VERSION "ASIPRO V2.0"

void
usage (FILE * fdo, const char *name)
{
  fprintf (fdo, NAME_VERSION "\n"); 
  fprintf (fdo, "Usage: %s input_file output_file\n", name);
  fprintf (fdo, "\t Assembles input_file to output_file\n");
}

int
main (int argc, char *argv[])
{
  FILE *fdi, *fdo;
  if (argc != 3)
    {
      usage (stderr, argv[0]);
      return EXIT_FAILURE;
    }
  if ((fdi = fopen (argv[1], "r")) == NULL)
    {
      fprintf (stderr, "Error opening file %s: %s\n", argv[1],
	       strerror (errno));
      return EXIT_FAILURE;
    }
  if ((fdo = fopen (argv[2], "wb")) == NULL)
    {
      fprintf (stderr, "Error opening file %s: %s\n", argv[2],
	       strerror (errno));
      return EXIT_FAILURE;
    }
  const int r = assemblage (fdi, fdo, stderr);
  fclose(fdi);
  fclose(fdo);
  return r;
}
