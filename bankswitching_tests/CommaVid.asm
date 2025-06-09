;;A bank switching demo for Commavid's 2K + 1K RAM scheme
;;By: Rick Skrbina 11/22/09

	processor 6502
	include "vcs.h"
	include "macro.h"
	
BG_ColorR	equ BG_ColorW - 1024
	
	seg.u RIOT_RAM
	org $80
	
	seg.u CART_RAM
	org $1400		;read $1000-13FF write $1400-17FF
	
BG_ColorW	ds 1
	
	seg CODE
	org $1800
Start
	CLEAN_START
	
Start_Frame
	lda #2
	sta VBLANK
	sta VSYNC
	sta WSYNC
	sta WSYNC
	sta WSYNC
	lsr
	sta VSYNC
	
	ldy #37
VerticalBlank
	sta WSYNC
	dey
	bne VerticalBlank
	
	sty VBLANK
	
	ldy #192
Picture
	sta WSYNC
	dey
	bne Picture
	
	lda #2
	sta VBLANK
	
	ldy #30
OverScan
	sta WSYNC
	dey
	bne OverScan
	
	clc
	lda BG_ColorR
	adc #1
	sta BG_ColorW
	sta COLUBK
	
	jmp Start_Frame


	org $1FFC
	.word Start
	.byte "RS"
