/* Time-stamp: <exec.c  27 Aug 02 20:05:14> */

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
#include <stdbool.h>
#include "../sipro.h"
#include "memory.h"

#define BITPOS(reg,pos) ( ( (1U<<pos) & reg ) ? 1U : 0U)
#define ABS(x) ((x>=0) ? x : -x)

extern bool trace;
extern bool stack;

static struct
{
  unsigned int registers[SIPRO_NBREGISTERS];
} sipro;

static int shiftr_f (void);
static int shiftl_f (void);
static int and_f (void);
static int or_f (void);
static int xor_f (void);
static int not_f (void);
static int add_f (void);
static int sub_f (void);
static int mul_f (void);
static int div_f (void);
static int cp_f (void);
static int loadw_f (void);
static int storew_f (void);
static int loadb_f (void);
static int storeb_f (void);
static int const_f (void);
static int push_f (void);
static int pop_f (void);
static int cmp_f (void);
static int uless_f (void);
static int sless_f (void);
static int jmp_f (void);
static int jmpz_f (void);
static int jmpc_f (void);
static int jmpe_f (void);
static int call_f (void);
static int ret_f (void);
static int callprintfd_f (void);
static int callprintfu_f (void);
static int callprintfs_f (void);
static int callscanfd_f (void);
static int callscanfu_f (void);
static int callscanfs_f (void);
static int nop_f (void);
static int end_f (void);

/* Les trois tableaux suivants sont liés par leurs indices */

static int (*pt_fct[]) (void) =
{
shiftr_f, shiftl_f, and_f, or_f, xor_f, not_f, add_f, sub_f, mul_f, div_f,
    cp_f, loadw_f, storew_f, loadb_f, storeb_f, const_f, push_f, pop_f,
    cmp_f, uless_f, sless_f, jmp_f, jmpz_f, jmpc_f, jmpe_f, call_f, ret_f,
    callprintfd_f, callprintfu_f, callprintfs_f, callscanfd_f, callscanfu_f,
    callscanfs_f, nop_f, end_f, NULL};

static short int opcodes[] = {
  0x10, 0x11, 0x15, 0x16, 0x17, 0x1a, 0x20, 0x21, 0x22, 0x23,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x40, 0x41, 0x50, 0x51,
  0x52, 0x60, 0x61, 0x62, 0x63, 0x65, 0x66, 0x6a, 0x6b, 0x6c,
  0x6d, 0x6e, 0x6f, 0x00, 0xff, -1
};

/* Used only in trace mode */
static char const *instr[] = {
  "shiftr", "shiftl", "and", "or", "xor", "not", "add", "sub", "mul", "div",
  "cp", "loadw", "storew", "loadb", "storeb", "const", "push", "pop", "cmp",
  "uless", "sless", "jmp", "jmpz", "jmpc", "jmpe", "call", "ret",
    "callprintfd",
  "callprintfu", "callprintfs", "callscanfd", "callscanfu", "callscanfs",
  "nop", "end", NULL
};

void
setCarry (int v)
{
  switch (v)
    {
    case 0:
      sipro.registers[ID_FL] &= ~(1U << 1U);
      break;
    case 1:
      sipro.registers[ID_FL] |= (1U << 1U);
      break;
    default:
      fprintf (stderr, "Error: setCarry invoked with value %d\n", v);
      exit (EXIT_FAILURE);
      break;
    }
}

void
setError (int v)
{
  switch (v)
    {
    case 0:
      sipro.registers[ID_FL] &= ~(1U << 0U);
      break;
    case 1:
      sipro.registers[ID_FL] |= (1U << 0U);
      break;
    default:
      fprintf (stderr, "Error: setError invoked with value %d\n", v);
      exit (EXIT_FAILURE);
      break;
    }
}

void
setZero (int v)
{
  switch (v)
    {
    case 0:
      sipro.registers[ID_FL] &= ~(1U << 2U);
      break;
    case 1:
      sipro.registers[ID_FL] |= (1U << 2U);
      break;
    default:
      fprintf (stderr, "Error: setZero invoked with value %d\n", v);
      exit (EXIT_FAILURE);
      break;
    }
}

int
getCarry (void)
{
  return (sipro.registers[ID_FL] & (1U << 1U)) != 0;
}

int
getError (void)
{
  return (sipro.registers[ID_FL] & (1U << 0U)) != 0;
}

int
getZero (void)
{
  return (sipro.registers[ID_FL] & (1U << 2U)) != 0;
}

/* 
   Retourne un entier <0 si la valeur signée du registre est <0
		      >=0                                    >=0
*/
static int
registerSgn (unsigned int reg)
{
  return (reg & (1U << (SIPRO_CHARBIT * SIPRO_REGISTERBYTESIZE - 1))) ? -1 :
    1;
}

/* Retourne la valeur du registre vue comme signée dans un long int */
static long int
registerToLongInt (unsigned int reg)
{
  long int r;
  int i;
  r = (registerSgn (reg) < 0) ? -1 : 0;
  for (i = SIPRO_CHARBIT * SIPRO_REGISTERBYTESIZE - 2; i >= 0; --i)
    {
      r <<= 1;
      r |= (reg & (1U << i)) ? 1 : 0;
    }
  return r;
}

static unsigned int
longIntToRegister (long int l)
{
  return (unsigned int) (l & 0xffff);
}

static int
shiftr_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction shiftr\n",
	       regIndex);
      return 0;
    }
  setCarry ((int) (sipro.registers[regIndex] & 1U));
  setZero (0);
  setError (0);
  sipro.registers[regIndex] >>= 1U;
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
shiftl_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction shiftl\n",
	       regIndex);
      return 0;
    }
  setCarry ((int)
	    (sipro.
	     registers[regIndex] & (1U <<
				    (SIPRO_REGISTERBYTESIZE * SIPRO_CHARBIT -
				     1U))));
  setZero (0);
  setError (0);
  sipro.registers[regIndex] =
    (sipro.registers[regIndex] << 1U) & (SIPRO_UINTMAX);
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
and_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction and\n",
	       regIndex1, regIndex2);
      return 0;
    }
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[regIndex1] &= sipro.registers[regIndex2];
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
or_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction or\n",
	       regIndex1, regIndex2);
      return 0;
    }
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[regIndex1] |= sipro.registers[regIndex2];
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
xor_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction xor\n",
	       regIndex1, regIndex2);
      return 0;
    }
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[regIndex1] ^= sipro.registers[regIndex2];
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
not_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction not\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setError (0);
  sipro.registers[regIndex] = ~sipro.registers[regIndex];
  setZero (0);
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static void
add_internal (unsigned int *v1,
	      unsigned int *v2, int *carry, int *zero, int *error)
{
  unsigned int retenue = 0U;
  unsigned int resultat = 0U;
  unsigned int i;
  for (i = 0U; i < SIPRO_REGISTERBITSIZE; ++i)
    {
      resultat |= (BITPOS (*v1, i) ^ BITPOS (*v2, i) ^ retenue) << i;
      retenue = (BITPOS (*v1, i) + BITPOS (*v2, i) + retenue) > 1;
    }
  if (registerSgn (*v1) == registerSgn (*v2))
    {
      if (registerSgn (*v1) != registerSgn (resultat))
	*error = 1U;
      else
	{
	  unsigned int s;	/* Retenue entrante dans la dernière somme bit à bit */
	  s = (BITPOS (*v1, (SIPRO_REGISTERBITSIZE - 1))
	       ^ BITPOS (*v2, (SIPRO_REGISTERBITSIZE - 1)))
	    != BITPOS (resultat, (SIPRO_REGISTERBITSIZE - 1));
	  *error = (s != retenue);
	}
    }
  else
    *error = 0;
  *carry = (int) retenue;
  *zero = resultat == 0U;
  *v1 = resultat;
}

static int
add_f (void)
{
  unsigned char regIndex1, regIndex2;
  int carry;
  int zero;
  int error;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction add\n",
	       regIndex1, regIndex2);
      return 0;
    }
  add_internal (&sipro.registers[regIndex1],
		&sipro.registers[regIndex2], &carry, &zero, &error);
  setCarry (carry);
  setZero (zero);
  setError (error);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
sub_f (void)
{
  unsigned int valReg2;
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction sub\n",
	       regIndex1, regIndex2);
      return 0;
    }
  /* On mémorise la valeur du second registre pour pouvoir la restituer avant de sortir */
  valReg2 = sipro.registers[regIndex2];
  if (registerToLongInt (sipro.registers[regIndex2]) == SIPRO_INTMIN)
    {
      /* Le second registre contient la valeur minimum autorisée
         C'est aussi la seule qui n'a pas d'opposé: la soustraction ne peut donc pas se
         faire comme la somme du premier opérande et de l'opposé du second.
         On essaie d'ajouter 1 au second opérande et de retirer 1 au premier pour se
         ramener à une somme.
       */
      if (registerToLongInt (sipro.registers[regIndex1]) >= 0L)
	{
	  setZero (0);
	  setCarry (0);
	  setError (1);
	}
      else
	{
	  /* Aucune chance qu'une erreur ne se produise */
	  unsigned int v1, v2;
	  int zero, carry, error;
	  v1 = sipro.registers[regIndex1];
	  v2 = longIntToRegister (+1L);
	  add_internal (&v1, &v2, &carry, &zero, &error);
	  sipro.registers[regIndex1] = v1;
	  v1 = sipro.registers[regIndex2];
	  v2 = longIntToRegister (+1L);
	  add_internal (&v1, &v2, &carry, &zero, &error);
	  sipro.registers[regIndex2] = longIntToRegister (-(long int) v1);
	  add_internal (&sipro.registers[regIndex1],
			&sipro.registers[regIndex2], &carry, &zero, &error);
	  setCarry (carry);
	  setZero (zero);
	  setError (error);
	}
    }
  else
    {
      int zero, carry, error;
      sipro.registers[regIndex2] =
	longIntToRegister (-registerToLongInt (sipro.registers[regIndex2]));
      add_internal (&sipro.registers[regIndex1], &sipro.registers[regIndex2],
		    &carry, &zero, &error);
      setCarry (carry);
      setZero (zero);
      setError (error);
    }
  sipro.registers[regIndex2] = valReg2;
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
mul_f (void)
{
  long int v1, v2;
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction mul\n",
	       regIndex1, regIndex2);
      return 0;
    }
  v1 = registerToLongInt (sipro.registers[regIndex1]);
  v2 = registerToLongInt (sipro.registers[regIndex2]);
  v1 *= v2;
  sipro.registers[regIndex1] = longIntToRegister (v1);
  setCarry (0);
  setZero (v1 == 0L);
  setError (v1 < SIPRO_INTMIN || v1 > SIPRO_INTMAX);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
div_f (void)
{
  long int v1, v2;
  unsigned char regIndex1, regIndex2;
  int sgnPos;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction div\n",
	       regIndex1, regIndex2);
      return 0;
    }
  sgnPos =
    registerSgn (sipro.registers[regIndex1]) ==
    registerSgn (sipro.registers[regIndex2]);
  v1 = ABS (registerToLongInt (sipro.registers[regIndex1]));
  v2 = ABS (registerToLongInt (sipro.registers[regIndex2]));
  if (v2 != 0L)
    {
      v1 /= v2;
      if (!sgnPos)
	v1 = -v1;
      sipro.registers[regIndex1] = longIntToRegister (v1);
      setError (0);
    }
  else
    setError (1);
  setCarry (0);
  setZero (v1 == 0L);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
cp_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction cp\n",
	       regIndex1, regIndex2);
      return 0;
    }
  sipro.registers[regIndex1] = sipro.registers[regIndex2];
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
loadw_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction loadw\n",
	       regIndex1, regIndex2);
      return 0;
    }
  readWord (sipro.registers[regIndex2], &sipro.registers[regIndex1]);
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
storew_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction storew\n",
	       regIndex1, regIndex2);
      return 0;
    }
  writeWord (sipro.registers[regIndex2], sipro.registers[regIndex1]);
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
loadb_f (void)
{
  unsigned char b;
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction loadb\n",
	       regIndex1, regIndex2);
      return 0;
    }
  readByte (sipro.registers[regIndex2], &b);
  sipro.registers[regIndex1] = (unsigned int) b;
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
storeb_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction storeb\n",
	       regIndex1, regIndex2);
      return 0;
    }
  writeByte (sipro.registers[regIndex2],
	     (unsigned char) (sipro.registers[regIndex2] & 0xff));
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
const_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX
      && regIndex != ID_CX
      && regIndex != ID_DX && regIndex != ID_SP && regIndex != ID_BP)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction const\n",
	       regIndex);
      return 0;
    }
  if (((sipro.registers[ID_IP] + 2U) % SIPRO_REGISTERBYTESIZE) == 0U)
    {
      readWord (sipro.registers[ID_IP] + 2U, &sipro.registers[regIndex]);
      sipro.registers[ID_IP] += 4U;
    }
  else
    {
      readWord (sipro.registers[ID_IP] + 3U, &sipro.registers[regIndex]);
      sipro.registers[ID_IP] += 5U;
    }
  setCarry (0);
  setZero (0);
  setError (0);
  return 1;
}

static int
push_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX
      && regIndex != ID_CX
      && regIndex != ID_DX && regIndex != ID_BP && regIndex != ID_SP)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction push\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setError (0);
  setZero (0);
  writeWord (sipro.registers[ID_SP] + SIPRO_REGISTERBYTESIZE,
	     sipro.registers[regIndex]);
  sipro.registers[ID_SP] += SIPRO_REGISTERBYTESIZE;
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
pop_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX
      && regIndex != ID_CX
      && regIndex != ID_DX && regIndex != ID_BP && regIndex != ID_SP)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction pop\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setZero (0);
  if (sipro.registers[ID_SP] >= sipro.registers[ID_BP])
    {
      setError (0);
      sipro.registers[ID_SP] -= SIPRO_REGISTERBYTESIZE;
      readWord (sipro.registers[ID_SP] + SIPRO_REGISTERBYTESIZE,
		&sipro.registers[regIndex]);
    }
  else
    setError (1);
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
cmp_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction cmp\n",
	       regIndex1, regIndex2);
      return 0;
    }
  setCarry (sipro.registers[regIndex1] == sipro.registers[regIndex2]);
  setZero ((sipro.registers[regIndex1] == sipro.registers[regIndex2])
	   && (sipro.registers[regIndex2] == 0U));
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
uless_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction uless\n",
	       regIndex1, regIndex2);
      return 0;
    }
  setCarry (sipro.registers[regIndex1] < sipro.registers[regIndex2]);
  setZero (0);
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
sless_f (void)
{
  unsigned char regIndex1, regIndex2;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction sless\n",
	       regIndex1, regIndex2);
      return 0;
    }
  setCarry (registerToLongInt (sipro.registers[regIndex1]) <
	    registerToLongInt (sipro.registers[regIndex2]));
  setZero (0);
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
jmp_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction jmp\n",
	       regIndex);
      return 0;
    }
  sipro.registers[ID_IP] = sipro.registers[regIndex];
  return 1;
}


static int
jmpz_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction jmpz\n",
	       regIndex);
      return 0;
    }
  sipro.registers[ID_IP] =
    getZero ()? sipro.registers[regIndex] : sipro.registers[ID_IP] + 2U;
  return 1;
}

static int
jmpc_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction jmpc\n",
	       regIndex);
      return 0;
    }
  sipro.registers[ID_IP] =
    getCarry ()? sipro.registers[regIndex] : sipro.registers[ID_IP] + 2U;
  return 1;
}

static int
jmpe_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction jmpe\n",
	       regIndex);
      return 0;
    }
  sipro.registers[ID_IP] =
    getError ()? sipro.registers[regIndex] : sipro.registers[ID_IP] + 2U;
  return 1;
}

static int
call_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction call\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setError (0);
  setZero (0);
  writeWord (sipro.registers[ID_SP] + SIPRO_REGISTERBYTESIZE,
	     sipro.registers[ID_IP] + 2U);
  sipro.registers[ID_SP] += SIPRO_REGISTERBYTESIZE;
  sipro.registers[ID_IP] = sipro.registers[regIndex];
  return 1;
}

static int
ret_f (void)
{
  sipro.registers[ID_SP] -= SIPRO_REGISTERBYTESIZE;
  readWord (sipro.registers[ID_SP] + SIPRO_REGISTERBYTESIZE,
	    &sipro.registers[ID_IP]);
  return 1;
}

static int
callprintfd_f (void)
{
  unsigned char regIndex;
  unsigned int entree;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction callprintfd\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setZero (0);
  setError (0);
  readWord (sipro.registers[regIndex], &entree);
  printf ("%d", (int) registerToLongInt (entree));
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
callprintfu_f (void)
{
  unsigned char regIndex;
  unsigned int entree;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction callprintfu\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setZero (0);
  setError (0);
  readWord (sipro.registers[regIndex], &entree);
  printf ("%u", entree);
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
callprintfs_f (void)
{
  unsigned char regIndex;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction callprintfs\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setZero (0);
  setError (0);
  printf ("%s", (char *) address (sipro.registers[regIndex]));
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
callscanfd_f (void)
{
  unsigned char regIndex;
  int entree;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction callscanfd\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setZero (0);
  if ((scanf ("%d", &entree) == 1) && (SIPRO_INTMIN <= entree)
      && (entree <= SIPRO_INTMAX))
    {
      writeWord (sipro.registers[regIndex],
		 longIntToRegister ((long int) entree));
      setError (0);
    }
  else
    setError (1);
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
callscanfu_f (void)
{
  unsigned char regIndex;
  unsigned int entree;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex);
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (stderr,
	       "Error: invalid register %u for instruction callscanfu\n",
	       regIndex);
      return 0;
    }
  setCarry (0);
  setZero (0);
  if ((scanf ("%u", &entree) == 1) && (entree <= SIPRO_UINTMAX))
    {
      writeWord (sipro.registers[regIndex], entree);
      setError (0);
    }
  else
    setError (1);
  sipro.registers[ID_IP] += 2U;
  return 1;
}

static int
callscanfs_f (void)
{
  unsigned char regIndex1, regIndex2;
  int c;
  unsigned int i;
  readByte (sipro.registers[ID_IP] + 1U, &regIndex1);
  readByte (sipro.registers[ID_IP] + 2U, &regIndex2);
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_SP
       && regIndex1 != ID_BP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_SP && regIndex2 != ID_BP))
    {
      fprintf (stderr,
	       "Error: invalid registers %u and %u for instruction callscanfs\n",
	       regIndex1, regIndex2);
      return 0;
    }
  for (i = 0U;
       (i < sipro.registers[regIndex2]) && ((c = getchar ()) != EOF)
       && (c != '\n'); ++i)
    writeByte (sipro.registers[regIndex1] + i, (unsigned char) c);
  writeByte (sipro.registers[regIndex1] + i, '\0');
  setCarry (0);
  setZero (i == 0U);
  setError (0);
  sipro.registers[ID_IP] += 3U;
  return 1;
}

static int
nop_f (void)
{
  setCarry (0);
  setZero (0);
  setError (0);
  sipro.registers[ID_IP] += 1U;
  return 1;
}

static int
end_f (void)
{
  return 1;
}

static void
printSIPRO ()
{
  unsigned int i;
  for (i = 0U; regs[i] != NULL; ++i)
    fprintf (stderr, "Register %s = %u (%d) [%x]\n",
	     regs[i], sipro.registers[i],
	     (int) registerToLongInt (sipro.registers[i]),
	     sipro.registers[i]);
  fprintf (stderr, "\n");
  if (stack) {
    for (int i = 10; i >= -10; --i)
      {
	unsigned int j;
	if (((int) registerToLongInt (sipro.registers[ID_BP])) + 2 * i >= 0)
	  {
	    readWord (sipro.registers[ID_BP] + 2 * i, &j);
	    fprintf (stderr, "bp+%d (%d): %u\n", 2 * i,
		     sipro.registers[ID_BP] + 2 * i, j);
	  }
      }
    fprintf (stderr, "\n");
  }
}

/* Execute la prochaine instruction */
static int
execute ()
{
  unsigned i;
  do
    {
      unsigned char opcode;
      readByte (sipro.registers[ID_IP], &opcode);
      for (i = 0U; opcodes[i] != -1 && opcodes[i] != opcode; ++i)
	;
      if (opcodes[i] == -1)
	{
	  fprintf (stderr, "Error: opcode %u unknown !\n", opcode);
	  return 0;
	}
      if (trace) {
	fprintf (stderr, "Execution of instruction IP=%u %x -> %x %s\n",
		 sipro.registers[ID_IP], sipro.registers[ID_IP],
		 (unsigned int) opcode, instr[i]);
	printSIPRO ();
      }
      if (!(*(pt_fct[i])) ())
	{
	  fprintf (stderr,
		   "Error: execution of instruction %u failed !\n",
		   opcode);
	  return 0;
	}
    }
  while (pt_fct[i] != end_f);
  return 1;
}

/* Démarre le programme */
int
startProg ()
{
  sipro.registers[ID_IP] = 0U;
  return execute ();
}
