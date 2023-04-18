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


; Programme de calcul de la fonction factorielle, par la méthode récursive

; Permet de passer la zone de stockage des constantes
	const ax,debut
	jmp ax

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Début de la zone de stockage des constantes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
:chaine1
@string "Rentrez votre nombre:"
:chaine2
@string "Factorielle "
:chaine3
@string " vaut "
:chaine4
@string "\n"
:chaine5
@string "Erreur !"
:valeurinit
@int 0
:resultat
@int 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Fin de la zone de stockage des constantes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;
; Début réel du code
;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Fonction de saisie
; La fonction ne modifie aucun registre, mis à part ax,
; qui au retour de la fonction contient la valeur saisie
; On commence par sauver tous les registres, de cette manière la fonction
; peux les modifier à loisirs, et restaurer les valeurs d'origine avant de
; retourner à la fonction appelante
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
:saisie
	push sp
	push bp
	push bx
	push cx
	push dx
; On affiche chaine1
	const ax,chaine1
	callprintfs ax
; On saisie une valeur, qu'on va mettre à l'adresse spécifiée par valeurinit
	const bx,erreur
	const ax,valeurinit
	callscanfu ax
; en cas d'erreur on appelle la fonction de sortie sur erreur
	jmpe bx
; on va maintenant récuperer la valeur stockée à l'adresse valeurinit (désignée par ax)
; pour la mettre dans le registre ax
	cp bx,ax
	loadw ax,bx
; On restaure les registres avant de sortir
; On fait bien attention à faire les pop
; dans l'ordre inverse des push du début de la fonction
	pop dx
	pop cx
	pop bx
	pop bp
	pop sp
; On retourne à la fonction appelante
	ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Fin de la fonction de saisie
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Fonction calculant récursivement la factorielle du mot machine 
; se trouvant sous le sommet de la pile
; L'argument se trouve sous le sommet de la pile
; En effet le sommet de la pile contient l'adresse de retour
; de la fonction
; Le résultat est mis dans ax
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
:factorielle
; On commence par calculer dans cx l'adresse de l'argument
	cp cx,sp
	const bx,2
	sub cx,bx
; C'est fait, maintenant on met la valeur de l'argument dans ax
	loadw ax,cx
; on regarde si on est dans le cas de base
	xor bx,bx
	const dx,casdebase
	cmp ax,bx
	jmpz dx
; on est dans le cas général
	const bx,1
	sub ax,bx
	push ax
	const dx,factorielle
	call dx
	pop dx
; La valeur de retour est dans ax
; On récupère de nouveau la valeur de l'argument
	cp dx,sp
	const bx,2
	sub dx,bx
	loadw bx,dx
	mul ax,bx
	const dx,finfact
	jmp dx
:casdebase
	const ax,1
	const dx,finfact
	jmp dx
:finfact
	ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Fin de la fonction factorielle récursive
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Fonction de sortie sur erreur
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
:erreur
	const ax,chaine5
	callprintfs ax
	end
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Fin de la fonction de sortie sur erreur
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;
; Fonction principale
;;;;;;;;;;;;;;;;;;;;;
:debut
	const bp,pile
	const sp,pile
	const ax,2
	sub sp,ax
	const dx,saisie
	call dx
	const bx,valeurinit
	storew ax,bx
	const bx,chaine2
	callprintfs bx
	const bx,valeurinit
	callprintfu bx
	const bx,chaine3
	callprintfs bx
	push ax
	const bx,factorielle
	call bx
	pop bx
	const bx,resultat
	storew ax,bx
	callprintfu bx
	const bx,chaine4
	callprintfs bx
	end
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Fin de la fonction principale
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Début de stockage de la zone de pile
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
:pile
@int 0
