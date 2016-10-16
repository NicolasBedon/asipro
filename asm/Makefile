
  # Copyright 2001-2016 Nicolas Bedon 
  # This file is part of ASIPRO.

  # ASIPRO is free software: you can redistribute it and/or modify
  # it under the terms of the GNU General Public License as published by
  # the Free Software Foundation, either version 3 of the License, or
  # (at your option) any later version.

  # ASIPRO is distributed in the hope that it will be useful,
  # but WITHOUT ANY WARRANTY; without even the implied warranty of
  # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  # GNU General Public License for more details.

  # You should have received a copy of the GNU General Public License
  # along with ASIPRO.  If not, see <http://www.gnu.org/licenses/>.


SHELL=/bin/sh

CCOPTS=-DDEBUG -g -pg -std=c11 -pedantic -Wall -Wpointer-arith -pg
CC=gcc

BIN=asipro
OBJS=analyse.o asm.o

$(BIN): $(OBJS)
	$(CC) $(CCOPTS) $(OBJS) -o $(BIN)

%.o:%.c
	$(CC) $(CCOPTS) $< -c

clean:
	-rm $(BIN) $(OBJS) gmon.out
