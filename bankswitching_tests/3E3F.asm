;;3E3F test

	processor 6502
	include "vcs.h"
	include "macro.h"
	
	seg.u vars
	org $80
	
	
	seg code
	org $0000
	rorg $F000
	
	nop
	nop
Start0

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
	
	lda #$1F
	sta COLUBK
	
	ldy #96
Picture
	sta WSYNC
	dey
	bne Picture
	
	jsr SWCH
	
	ldy #96
Picture1
	sta WSYNC
	dey
	bne Picture1
	
	lda #2
	sta VBLANK
	
	ldy #30
OverScan
	sta WSYNC
	dey
	bne OverScan
	
	jmp Start_Frame
	
	org $0100
	rorg $F100
SWCH
	sta $3F
	nop
	nop
	nop
	nop
	nop
	rts
	
	org $0FFC
	rorg $FFFC
	.word Start
	.word Start
	
	seg bank1
	org $1000
	rorg $F000
Start	
	sta $3E
	
Sub
	lda #$0F
	sta COLUBK
	rts
	
	org $1100
	rorg $F100
	nop
	nop
	jsr Sub
	sta $3E
	
	org $1FFC
	rorg $FFFC
	.word Start
	.word Start
