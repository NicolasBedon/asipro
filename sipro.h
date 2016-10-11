/* Time-stamp: <sipro.h  12 Oct 01 11:44:27> */

/*
  Copyright 2001-2016 Nicolas Bedon 
  This file is part of ASIPRO and SIPRO.

  ASIPRO and SIPRO are free softwares: you can redistribute them and/or modify
  them under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  ASIPRO and SIPRO are distributed in the hope that they will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with ASIPRO and SIPRO.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _SIPRO_H
#define _SIPRO_H

#define SIPRO_CHARBIT 8U
#define SIPRO_REGISTERBYTESIZE 2U
#define SIPRO_REGISTERBITSIZE (SIPRO_CHARBIT*SIPRO_REGISTERBYTESIZE)

#define SIPRO_INTMIN  -32768
#define SIPRO_INTMAX   32767
#define SIPRO_UINTMAX 65535U
#define SIPRO_UINTMIN     0U

#define SIPRO_NBREGISTERS 8U
/* 
   Les indices des éléments dans le tableau sont en corélation avec
   les valeurs du type énuméré
*/
static char const * const regs[] = { "ip", "fl", "sp", "bp", "ax", "bx","cx", "dx", NULL};
typedef enum { ID_IP, ID_FL, ID_SP, ID_BP, ID_AX, ID_BX, ID_CX, ID_DX } idRegister;

#endif
