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


; Recursive Hanoi tours solver
; Nicolas Bedon

	const ax,debut
	jmp ax

:chaineA
@string "A"
:chaineB
@string "B"
:chaineC
@string "C"
:chaine1
@string "Moving "
:chaine2
@string " to "
:chaine3
@string "\n"

:hanoi
	push sp
	push bp
	push ax
	push bx
	push cx
	push dx
; Récupération des arguments
	cp dx,sp
	const cx,14
	sub dx,cx
; On met dans ax le nombre de disques
	loadw ax,dx
	const cx,2
	sub dx,cx
; On met dans bx l'adresse du poteau source
	loadw bx,dx
	sub dx,cx
	push bp
	cp bp,cx
; On met dans cx l'adresse du poteau destination
	loadw cx,dx
	sub dx,bp
	pop bp
; On met dans dx l'adresse du poteau temporaire
	loadw dx,dx
; Les arguments sont récupérés, c'est parti
; Vérification du cas de base
	push dx
	push cx
	const cx,casDeBase
	const dx,1
	cmp ax,dx
	jmpc cx
	const dx,casGeneral
	jmp dx
:casDeBase
	pop cx
	pop dx
	const dx,chaine1
	callprintfs dx
	callprintfs bx
	const dx,chaine2
	callprintfs dx
	callprintfs cx
	const dx,chaine3
	callprintfs dx
	const dx,fin
	jmp dx
:casGeneral
	pop cx
	pop dx
; Premier appel récursif
	push cx
	push dx
	push bx
	push bx
	const bx,1
	sub ax,bx
	pop bx
	push ax
	const ax,hanoi
	call ax
	pop ax
	pop bx
	pop dx
	pop cx
; Second appel récursif, avec 1 comme nombre de disques
	push dx
	push cx
	push bx
	const bx,1
	push bx
	const bx,hanoi
	call bx
	pop bx
	pop bx
	pop cx
	pop dx
; Troisième appel récursif
	push bx
	push cx
	push dx
	push ax
	const ax,hanoi
	call ax
	pop ax
	pop dx
	pop cx
	pop bx
:fin
	pop dx
	pop cx
	pop bx
	pop ax
	pop bp
	pop sp
	ret

; Main code
:debut
	const bp,pile
	const sp,pile
	const ax,2
	sub sp,ax
; On va déplacer de A vers C en passant par B
	const dx,chaineB
	const cx,chaineC
	const bx,chaineA
; On va déplacer 3 anneaux
	const ax,3
	push dx
	push cx
	push bx
	push ax
	const ax,hanoi
	call ax
	pop ax
	pop bx
	pop cx
	pop dx
	end

; Stack area
:pile
@int 0
