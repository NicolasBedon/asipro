/* Time-stamp: <memory.h  13 Oct 01 19:37:08> */

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

/* The interface used to access the memory.
   Those functions check if the address is correct.
   If not, they write a message on the standard error
   (bus error, segmentation violation), and stop
   the processor.
*/
#ifndef _MEMORY_H
#define _MEMORY_H

extern void readByte (unsigned int address, unsigned char *byte);
extern void writeByte (unsigned int address, unsigned char byte);

/* A function for reading/writing a word verify the alignment of the address.
   The address must be a multiple of the size of a word, in bytes.
   If incorrect, the function writes a message on the standard error
   (bus error, segmentation violation), and stop
   the processor.
*/
extern void readWord (unsigned int address, unsigned int *word);
extern void writeWord (unsigned int address, unsigned int word);

/* Load the memory from the binary reading file.
 */
extern void loadMemoryFromFile (FILE * fdi);

/* Returns the REAL address of a memory cell of SIPRO */
extern void *address (unsigned int a);

extern void dumpMemory (void);

#endif
