/* Time-stamp: <memory.c  13 Oct 01 21:15:13> */

/*
  Copyright 2001-2016 Nicolas Bedon 
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
#include "../sipro.h"

/* Taille de la mémoire */
#define MEMSIZE 32768

static unsigned char memory[MEMSIZE];

void
readByte (unsigned int address, unsigned char *byte)
{
  if (address >= MEMSIZE)
    {
      fprintf (stderr, "Bus error !");
      exit (EXIT_FAILURE);
    }
  *byte = memory[address];
}

void
writeByte (unsigned int address, unsigned char byte)
{
  if (address >= MEMSIZE)
    {
      fprintf (stderr, "Bus error !");
      exit (EXIT_FAILURE);
    }
  memory[address] = byte;
}

/* A function for reading/writing a word verify the alignment of the address.
   The address must be a multiple of the size of a word, in bytes.
   If incorrect, the function writes a message on the standard error
   (bus error, segmentation violation), and stop
   the processor.
   Byte1 is the byte with the lowest address 
*/
void
readWord (unsigned int address, unsigned int *word)
{
  if (address >= MEMSIZE || (address % SIPRO_REGISTERBYTESIZE) != 0)
    {
      fprintf (stderr, "Bus error !");
      exit (EXIT_FAILURE);
    }
  *word =
    (memory[address] << SIPRO_CHARBIT) | (unsigned int) memory[address + 1U];
}

void
writeWord (unsigned int address, unsigned int word)
{
  if (address >= MEMSIZE || (address % SIPRO_REGISTERBYTESIZE) != 0)
    {
      fprintf (stderr, "Bus error !");
      exit (EXIT_FAILURE);
    }
  memory[address] = (unsigned char) (word >> SIPRO_CHARBIT);
  memory[address + 1U] = (unsigned char) (word & 0xff);
}

void
loadMemoryFromFile (FILE * input)
{
  int b;
  unsigned int a = 0U;
  while ((!feof (input)) && ((b = fgetc (input)) != EOF))
    {
      writeByte (a, (unsigned char) b);
      ++a;
    }
}

/* Récupérer l'adresse REELLE d'une cellule mémoire de l'émulateur */
void *
address (unsigned int a)
{
  return &(memory[a]);
}

void
dumpMemory (void)
{
  unsigned int i;
  for (i = 0U; i < MEMSIZE; i += 16U)
    fprintf (stderr,
	     "%08x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
	     i, memory[i + 1], memory[i], memory[i + 3], memory[i + 2],
	     memory[i + 5], memory[i + 4], memory[i + 7], memory[i + 6],
	     memory[i + 9], memory[i + 8], memory[i + 11], memory[i + 10],
	     memory[i + 13], memory[i + 12], memory[i + 15], memory[i + 14]);
  fprintf (stderr, "\n");
}
