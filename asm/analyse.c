/* Time-stamp: <analyse.c  13 Feb 02 17:12:51> */

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
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include "../sipro.h"

/* Taille maximale d'une ligne */
#define MAXLINE 2048U

/* Nombre max. de symboles */
#define MAXSYMBOLES 10000U

static int shiftr_f (const char *ligne,
		     unsigned int nligne, unsigned int *ncellule);
static int shiftl_f (const char *ligne,
		     unsigned int nligne, unsigned int *ncellule);
static int and_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int or_f (const char *ligne,
		 unsigned int nligne, unsigned int *ncellule);
static int xor_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int not_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int add_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int sub_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int mul_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int div_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int cp_f (const char *ligne,
		 unsigned int nligne, unsigned int *ncellule);
static int loadw_f (const char *ligne,
		    unsigned int nligne, unsigned int *ncellule);
static int storew_f (const char *ligne,
		     unsigned int nligne, unsigned int *ncellule);
static int loadb_f (const char *ligne,
		    unsigned int nligne, unsigned int *ncellule);
static int storeb_f (const char *ligne,
		     unsigned int nligne, unsigned int *ncellule);
static int const_f (const char *ligne,
		    unsigned int nligne, unsigned int *ncellule);
static int push_f (const char *ligne,
		   unsigned int nligne, unsigned int *ncellule);
static int pop_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int cmp_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int uless_f (const char *ligne,
		    unsigned int nligne, unsigned int *ncellule);
static int sless_f (const char *ligne,
		    unsigned int nligne, unsigned int *ncellule);
static int jmp_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int jmpz_f (const char *ligne,
		   unsigned int nligne, unsigned int *ncellule);
static int jmpc_f (const char *ligne,
		   unsigned int nligne, unsigned int *ncellule);
static int jmpe_f (const char *ligne,
		   unsigned int nligne, unsigned int *ncellule);
static int call_f (const char *ligne,
		   unsigned int nligne, unsigned int *ncellule);
static int ret_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int callprintfd_f (const char *ligne,
			  unsigned int nligne, unsigned int *ncellule);
static int callprintfu_f (const char *ligne,
			  unsigned int nligne, unsigned int *ncellule);
static int callprintfs_f (const char *ligne,
			  unsigned int nligne, unsigned int *ncellule);
static int callscanfd_f (const char *ligne,
			 unsigned int nligne, unsigned int *ncellule);
static int callscanfu_f (const char *ligne,
			 unsigned int nligne, unsigned int *ncellule);
static int callscanfs_f (const char *ligne,
			 unsigned int nligne, unsigned int *ncellule);
static int nop_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);
static int end_f (const char *ligne,
		  unsigned int nligne, unsigned int *ncellule);


/* 
   Les deux tableaux suivants ont des correspondances pour les indices
   Ils doivent terminer par des pointeurs NULL
*/
static char const *instr[] = {
  "shiftr", "shiftl", "and", "or", "xor", "not", "add", "sub", "mul", "div",
  "cp", "loadw", "storew", "loadb", "storeb", "const", "push", "pop", "cmp",
  "uless", "sless", "jmp", "jmpz", "jmpc", "jmpe", "call", "ret",
  "callprintfd",
  "callprintfu", "callprintfs", "callscanfd", "callscanfu", "callscanfs",
  "nop", "end", NULL
};

static int (*pt_fct[]) (const char *, unsigned int, unsigned int *) =
{
shiftr_f, shiftl_f, and_f, or_f, xor_f, not_f, add_f, sub_f, mul_f, div_f,
    cp_f, loadw_f, storew_f, loadb_f, storeb_f, const_f, push_f, pop_f,
    cmp_f, uless_f, sless_f, jmp_f, jmpz_f, jmpc_f, jmpe_f, call_f, ret_f,
    callprintfd_f, callprintfu_f, callprintfs_f, callscanfd_f, callscanfu_f,
    callscanfs_f, nop_f, end_f, NULL};

/**********************************************************
 La table des symboles 
 Un symbole commence toujours par un caractère alphabétique
 (isalpha())
 **********************************************************
 */
#define SYMBOLE_NOTDEFINED LONG_MAX

typedef struct
{
  char *nom;
  long int valeur;
} Symbole;

static Symbole symboleTable[MAXSYMBOLES];

/* Indice de la première position non occupée dans la table */
static unsigned nextSymbole = 0U;

/* Numéro de la passe */
static char symbolePasse = 1;
/**********************************************************
 Fin de la table des symboles 
 **********************************************************
 */

/* Les trois flots dans lesquels on travaille */
static FILE *input, *output, *erroutput;

static int
findRegisterIndex (const char *name, idRegister * r)
{
  for (*r = 0U; regs[*r] != NULL && strcmp (name, regs[*r]); ++*r)
    ;
  return regs[*r] != NULL;
}

static int
outputBytes (const unsigned char *buf,
	     unsigned n, FILE * fdo, unsigned int *ncellules)
{
  long int i;
  /* Si des symboles ont été introduits dans la table et qu'ils n'ont pas 
     encore été résolus, c'est le moment de le faire !
   */
  for (i = ((long int) nextSymbole) - 1L;
       i >= 0 && symboleTable[i].valeur == SYMBOLE_NOTDEFINED; --i)
    symboleTable[i].valeur = *ncellules;
  /* Maintenant on peut procéder à l'écriture, mais seulement si on
     est dans la seconde passe. Sinon on fait comme si
   */
  if (symbolePasse == 2)
    if (fwrite (buf, n, 1, fdo) != 1)
      {
	fprintf (erroutput, "Error while writing in output file\n");
	perror("");
	return 0;
      }
  *ncellules += n;
  return 1;
}

static int
codeValue (long int v, unsigned char dest[2])
{
  if (v < 0 && v < SIPRO_INTMIN)
    return 0;
  if (v >= 0 && v > SIPRO_UINTMAX)
    return 0;
  if (v >= 0)
    {				/* On garde le codage de v */
      dest[1] = v & 0x00ffUL;
      dest[0] = (v & 0xff00UL) >> 8;
    }
  else
    {				/* On va faire le codage */
      unsigned long t = 0UL;
      t = (-v) & 0xffffUL;
      t = ~t;
      t = t + 1;
      dest[1] = t & 0x00ffUL;
      dest[0] = (t & 0xff00UL) >> 8;
    }
  return 1;
}

static int
findSymbole (const char *const s, Symbole ** r)
{
  unsigned int i;
  for (i = 0U;
       i < MAXSYMBOLES && i < nextSymbole && strcmp (s, symboleTable[i].nom);
       ++i)
    ;
  if (i == MAXSYMBOLES || i == nextSymbole)
    return 0;
  *r = &symboleTable[i];
  return 1;
}

/* Ne fait rien et retourne toujours 1 lors de la seconde passe */
static int
addSymbole (const char *const s)
{
  Symbole *r;
  if (symbolePasse == 2)
    return 1;
  if (nextSymbole == MAXSYMBOLES)
    {
      fprintf (erroutput, "Error: table of symbols is full."
	       "Can not insert %s\n", s);
      return 0;
    }
  if (findSymbole (s, &r))
    {
      fprintf (erroutput, "Error: symbol %s already defined\n", s);
      return 0;
    }
  if ((symboleTable[nextSymbole].nom = malloc (strlen (s) + 1)) == NULL)
    {
      fprintf (erroutput, "Error: memory allocation (addSymbole %s)\n", s);
      exit (EXIT_FAILURE);
    }
  strcpy (symboleTable[nextSymbole].nom, s);
  symboleTable[nextSymbole].valeur = SYMBOLE_NOTDEFINED;
  ++nextSymbole;
  return 1;
}

static void
printSymboleTable (FILE * fdo)
{
  unsigned int i;
  fprintf (fdo, "Table of symbols:\n");
  for (i = 0U; i < nextSymbole; ++i)
    fprintf (fdo, "%s: %o (octal)\n", symboleTable[i].nom,
	     (int) symboleTable[i].valeur);
}

static int
shiftr_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x10, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction shiftr\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
shiftl_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x11, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction shiftl\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
and_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x15, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (erroutput, "Error: incorrect register(s): "
	       "%s line %u", ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
or_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x16, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (erroutput, "Error: incorrect registrer(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
xor_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x17, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
not_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x1a, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction not\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
add_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x20, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
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
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
sub_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x21, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
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
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
mul_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x22, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
div_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x23, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
cp_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x30, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_BP
       && regIndex1 != ID_SP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_BP && regIndex2 != ID_SP))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
loadw_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x31, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_BP
       && regIndex1 != ID_SP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_BP && regIndex2 != ID_SP))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
storew_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x32, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_BP
       && regIndex1 != ID_SP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_BP && regIndex2 != ID_SP))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
loadb_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x33, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_BP
       && regIndex1 != ID_SP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_BP && regIndex2 != ID_SP))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
storeb_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x34, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX
       && regIndex1 != ID_BP
       && regIndex1 != ID_SP)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX
	  && regIndex2 != ID_CX
	  && regIndex2 != ID_DX && regIndex2 != ID_BP && regIndex2 != ID_SP))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
const_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x35, 0, 0, 0, 0 };
  long int valeur;
  char *sep, *ana;
  int trou = *ncellule % 2 != 0;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX
      && regIndex != ID_CX
      && regIndex != ID_DX && regIndex != ID_SP && regIndex != ID_BP)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction const\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  /* La valeur peut être soit une valeur, soit un label */
  if (isalpha ((int) (*(sep + 1))))
    {
      Symbole *r;
      int a = findSymbole (sep + 1, &r);
      if ((!a) || (r->valeur == SYMBOLE_NOTDEFINED))
	{
	  if (symbolePasse == 2)
	    {
	      fprintf (erroutput, "Error: %s line %u"
		       "Symbol %s not found in symbols table,"
		       "or incorrect value (%ld)\n",
		       ligne, nligne, sep + 1, r->valeur);
	      *sep = ',';
	      return 0;
	    }
	  else
	    valeur = 0L;
	}
      else
	valeur = r->valeur;
      fprintf (stderr, "const: symbol %s has value %x\n", sep + 1,
	       (int) valeur);
    }
  else
    {
      valeur = strtol (sep + 1, &ana, 10);
      if (*ana != '\0' || (ana == sep + 1))
	{
	  fprintf (erroutput, "Error: %s line %u"
		   " : constant has an incorrect syntax\n",
		   ligne, nligne);
	  *sep = ',';
	  return 0;
	}
    }
  if (!codeValue (valeur, trou ? buf + 3 : buf + 2))
    {
      fprintf (erroutput, "Error: %s line %u"
	       " : constant %ld has an incorrect value\n",
	       ligne, nligne, valeur);
      *sep = ',';
      return 0;
    }
  fprintf (stderr, "const : write of %x %x\n", (int) buf[trou ? 3 : 2],
	   (int) buf[trou ? 4 : 3]);
  buf[1] = regIndex;
  *sep = ',';
  return outputBytes (buf, trou ? 5U : 4U, output, ncellule);
}

static int
push_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x40, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX
      && regIndex != ID_CX
      && regIndex != ID_DX && regIndex != ID_SP && regIndex != ID_BP)
    {
      fprintf (erroutput, "Error: registrer %s line %u"
	       " can not be used with instruction push\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
pop_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x41, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX
      && regIndex != ID_CX
      && regIndex != ID_DX && regIndex != ID_SP && regIndex != ID_BP)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction pop\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
cmp_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x50, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
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
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
uless_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x51, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
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
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
sless_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x52, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
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
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
jmp_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x60, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: registrer %s line %u"
	       " can not be used with instruction jmp\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
jmpz_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x61, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction jmpz\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
jmpc_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x62, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction jmpc\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
jmpe_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x63, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction jmpe\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
call_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x65, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction call\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
ret_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  unsigned char buf[] = { 0x66 };
  return outputBytes (buf, 1, output, ncellule);
}

static int
callprintfd_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x6a, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction callprintfd\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
callprintfu_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x6b, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction callprintfu\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
callprintfs_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x6c, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction callprintfs\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
callscanfd_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x6d, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction callscanfd\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
callscanfu_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex;
  unsigned char buf[] = { 0x6e, 0 };
  if (!(findRegisterIndex (ligne, &regIndex)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      return 0;
    }
  if (regIndex != ID_AX
      && regIndex != ID_BX && regIndex != ID_CX && regIndex != ID_DX)
    {
      fprintf (erroutput, "Error: register %s line %u"
	       " can not be used with instruction callscanfu\n",
	       ligne, nligne);
      return 0;
    }
  buf[1] = regIndex;
  return outputBytes (buf, 2, output, ncellule);
}

static int
callscanfs_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  idRegister regIndex1, regIndex2;
  unsigned char buf[] = { 0x6F, 0, 0 };
  char *sep;
  if ((sep = strchr (ligne, ',')) == NULL)
    {
      fprintf (erroutput, "Syntax error line %u: %s\n", nligne, ligne);
      return 0;
    }
  *sep = '\0';
  if (!(findRegisterIndex (ligne, &regIndex1)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if (!(findRegisterIndex (sep + 1, &regIndex2)))
    {
      fprintf (erroutput, "Error: unknown "
	       "register: %s line %u\n", ligne, nligne);
      *sep = ',';
      return 0;
    }
  if ((regIndex1 != ID_AX
       && regIndex1 != ID_BX
       && regIndex1 != ID_CX
       && regIndex1 != ID_DX)
      || (regIndex2 != ID_AX
	  && regIndex2 != ID_BX && regIndex2 != ID_CX && regIndex2 != ID_DX))
    {
      fprintf (erroutput, "Error: incorrect register(s): %s line %u\n",
	       ligne, nligne);
      *sep = ',';
      return 0;
    }
  *sep = ',';
  buf[1] = regIndex1;
  buf[2] = regIndex2;
  return outputBytes (buf, 3, output, ncellule);
}

static int
nop_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  unsigned char buf[] = { 0x00 };
  return outputBytes (buf, 1U, output, ncellule);
}

static int
end_f (const char *ligne, unsigned int nligne, unsigned int *ncellule)
{
  unsigned char buf[] = { 0xff };
  return outputBytes (buf, 1U, output, ncellule);
}

static int
traiteConstante (const char *ligne,
		 unsigned int nligne, unsigned int *ncellule)
{
  char *sep, *p;
  sep = strchr (ligne, ' ');
  if (sep == NULL)
    {
      fprintf (erroutput, "Error: constant %s undefined line %u\n",
	       ligne, nligne);
      return 0;
    }
  *sep = '\0';
  if (!strcmp (ligne, "string"))
    {				/* Cas des chaînes de caractères */
      int r;
      if ((*(sep + 1) != '\"') || (*(sep + 1 + strlen (sep + 1) - 1) != '\"'))
	{
	  fprintf (erroutput,
		   "Error: delimitors of constant strings "
		   "must be \"\n");
	  *sep = ' ';
	  return 0;
	}
      *(sep + 1 + strlen (sep + 1) - 1) = '\0';
      /* Il faut réaliser l'alignement */
      if ((*ncellule) % 2 == 1)
	{
	  if (symbolePasse == 2)
	    if (fwrite ("\0", 1, 1, output) != 1)
	      {
		fprintf (erroutput,
			 "Error while writing in output file\n");
		return 0;
	      }
	  ++*ncellule;
	}
      for (r = 1, p = sep + 2;
	   (r == 1) && (p < sep + 2 + strlen (sep + 2) + 1); ++p)
	{
	  char c;
	  if (*p == '\\')
	    switch (*++p)
	      {
	      case 'n':
		c = '\n';
		break;
	      case 't':
		c = '\t';
		break;
	      case 'r':
		c = '\r';
		break;
	      default:
		{
		  fprintf (erroutput,
			   "Error: unknown despecialized character\n");
		  return 0;
		}
	      }
	  else
	    c = *p;
	  r = outputBytes ((unsigned char *) &c, 1, output, ncellule);
	}
      *sep = ' ';
      *(sep + 2 + strlen (sep + 2)) = '\"';
      return r;
    }
  else if (!strcmp (ligne, "int"))
    {				/* Cas des entiers */
      char *ana;
      unsigned char buf[2];
      long int valeur = strtol (sep + 1, &ana, 10);
      if (*ana != '\0' || (ana == sep + 1))
	{
	  fprintf (erroutput, "Error: %s line %u"
		   " : syntax error in constant\n",
		   ligne, nligne);
	  *sep = ' ';
	  return 0;
	}
      if (!codeValue (valeur, buf))
	{
	  fprintf (erroutput, "Error: %s line %u"
		   " : constant %ld has an incorrect value\n",
		   ligne, nligne, valeur);
	  *sep = ' ';
	  return 0;
	}
      *sep = ' ';
      /* Il faut réaliser l'alignement */
      if ((*ncellule) % 2 == 1)
	{
	  if (symbolePasse == 2)
	    if (fwrite ("\0", 1, 1, output) != 1)
	      {
		fprintf (erroutput,
			 "Error while writing in output file\n");
		return 0;
	      }
	  ++*ncellule;
	}
      return outputBytes (buf, 2, output, ncellule);
    }
  else
    {				/* Erreur */
      fprintf (erroutput, "Error: type %s for directives "
	       "of constants placements unknown\n", ligne);
      *sep = ' ';
      return 0;
    }
}

static int
traiteInstruction (const char *ligne,
		   unsigned int nligne, unsigned int *ncellule)
{
  unsigned int i;
  char *sep;
  sep = strchr (ligne, ' ');
  if (sep != NULL)
    *sep = '\0';
  for (i = 0U; instr[i] != NULL && strcmp (ligne, instr[i]); ++i)
    ;
  if (instr[i] == NULL)
    {
      fprintf (erroutput, "Error: unknown instruction %s line %u\n", ligne,
	       nligne);
      if (sep != NULL)
	*sep = ' ';
      return 0;
    }
  if (sep != NULL)
    *sep = ' ';
  return (*pt_fct[i]) (sep + 1, nligne, ncellule);
}

static int
analyse (void)
{
  static char ligne[MAXLINE];
  unsigned int nligne = 1U;	/* Numéro de la ligne courante */
  unsigned int ncellule = 0U;	/* Nombre de cellules écrites  */
  while (!feof (input) && (fgets (ligne, MAXLINE, input) != NULL))
    {
      fprintf (erroutput, "%s", ligne);
      ligne[strlen (ligne) - 1] = '\0';
      switch (ligne[0])
	{
	case '@':		/* Directive de placement de constante */
	  if (!traiteConstante (ligne + 1, nligne, &ncellule))
	    return 0;
	  break;
	case ':':		/* Label                               */
	  if (!addSymbole (ligne + 1))
	    {
	      fprintf (erroutput,
		       "Error adding %s to symbols table\n",
		       ligne + 1);
	      return 0;
	    }
	  break;
	case ';':		/* Commentaire                         */
	case '\0':		/* Ligne vide                          */
	  break;
	case '\t':		/* Instruction                         */
	  if (!traiteInstruction (ligne + 1, nligne, &ncellule))
	    return 0;
	  break;
	default:
	  fprintf (erroutput, "Syntax error: line %u not recognized: %s\n",
		   nligne, ligne);
	  return EXIT_FAILURE;
	}
      ++nligne;
    }
  return 1;
}

int
assemblage (FILE * fdi, FILE * fdo, FILE * fde)
{
  input = fdi;
  output = fdo;
  erroutput = fde;
  symbolePasse = 1;
  if (!analyse ())
    {
      fprintf (erroutput, "Error in first pass\n");
      return 1;
    }
  symbolePasse = 2;
  rewind (fdi);
  if (!analyse ())
    {
      fprintf (erroutput, "Error in second pass\n");
      return 1;
    }
  printSymboleTable (fde);
  return 0;
}
