; Copyright 2001-2023 Nicolas Bedon 
; This file is part of ASIPRO.
;
; ASIPRO is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
; 
; ASIPRO is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with ASIPRO.  If not, see <http://www.gnu.org/licenses/>.


; Hello World !

; Skip data area
	const ax,start
	jmp ax

; Beginning of data area
:sHelloWorld
@string "Hello, world !\n"
; End of data area

; Main code
:start
; Preparation of stack area (unuseful for this example)
	const bp,stack
	const sp,stack
	const ax,2
	sub sp,ax
; End of preparation of stack area
	const dx,sHelloWorld
	callprintfs dx
	end

; Stack area
:stack
@int 0
