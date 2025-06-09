;;A Bankswitching deom for John Payson's 4A50 scheme.  64K ROM, 32K RAM
;;By: Rick Skrbina 6/13/09
;;
;;$71-$7F and $F4-$FF - ZP hotspots, BS modes selected here
;;$1000-$17FF access any 2K region of RAM, or the first 32K of ROM
;;$1800-$1DFF access first 1.5K of any 2K reigon of RAM or last 32K of ROM
;;$1E00-$1EFF may point to any 256 byte page of RAM or ROM
;;$1F00-$1FFF will always access the last 256 bytes of ROM
;;
;;Hotspots (taken from http://casperkitty.com/stella/CARTFMT.htm)
;;
;;$6C00-$6CFF  Enable page 0-255 of ROM at $1E00-$1EFF
;;$6D00-$6D7F  Enable page 0-127 of RAM at $1E00-$1EFF
;;
;;.htm)
;;
;;$6C00-$6CFF  Enable page 0-255 of ROM at $1E00-$1EFF
;;$6D00-$6D7F  Enable page 0-127 of RAM at $1E00-$1EFF
;;
;;$6E00-$6E0F  Enable block 0-15 of ROM at $1000-$17FF
;;$6E40-$6E4F  Enable block 0-15 of RAM at $1000-$17FF
;;
;;$6F10-$6F1F  Enable block 16-31 of ROM at $1800-$1DFF (first 1.5K only)
;;$6F40-$6F4F  Enable block 0-15 of RAM at $1800-$1DFF (first 1.5K only)


;;Constants

StartLow0	equ $0000
StartLow1	equ $0800
StartLow2	equ $1000
StartLow3	equ $1800
StartLow4	equ $2000
StartLow5	equ $2800
StartLow6	equ $3000
StartLow7	equ $3800
StartLow8	equ $4000
StartLow9	equ $4800
StartLowA	equ $5000
StartLowB	equ $5800
StartLowC	equ $6000
StartLowD	equ $6800
StartLowE	equ $7000
StartLowF	equ $7800

StartHi0	equ $8000
StartHi1	equ $8800
StartHi2	equ $9000
StartHi3	equ $9800
StartHi4	equ $A000
StartHi5	equ $A800
StartHi6	equ $B000
StartHi7	equ $B800
StartHi8	equ $C000
StartHi9	equ $C800
StartHiA	equ $D000
StartHiB	equ $D800
StartHiC	equ $E000
StartHiD	equ $E800
StartHiE	equ $F000
StartHiF	equ $F800

	processor 6502
	include "vcs.h"
	include "macro.h"
	
	seg.u RIOT_RAM
	org $80
	
;ROM slices

	seg low0
	org StartLow0
	rorg $1000
	
Low0
	.byte $00

	seg low1
	org StartLow1
	rorg $1000
	
Low1
	.byte $02
	
	seg low2
	org StartLow2
	rorg $1000
	
Low2
	.byte $04
	
	seg low3
	org StartLow3
	rorg $1000
	
Low3
	.byte $06
	
	seg low4
	org StartLow4
	rorg $1000
	
Low4
	.byte $08
	
	seg low5
	org StartLow5
	rorg $1000
	
Low5
	.byte $0A
	
	seg low6
	org StartLow6
	rorg $1000
	
Low6
	.byte $0C
	
	seg low7
	org StartLow7
	rorg $1000
	
Low7
	.byte $0E
	
	seg low8
	org StartLow8
	rorg $1000
	
Low8
	.byte $10
	

	seg low9
	org StartLow9
	rorg $1000
	
Low9
	.byte $12
	
	seg lowA
	org StartLowA
	rorg $1000
	
LowA
	.byte $14
	
	seg lowB
	org StartLowB
	rorg $1000
	
LowB
	.byte $16
	
	seg lowC
	org StartLowC
	rorg $1000
	
LowC
	.byte $18
	
	seg lowD
	org StartLowD
	rorg $1000
	
LowD
	.byte $1A
	
	seg lowE
	org StartLowE
	rorg $1000
	
LowE
	.byte $1C
	
	seg lowF
	org StartLowF
	rorg $1000
	
LowF

	.byte $1E
	
	seg hi0
	org StartHi0
	rorg $1800
Hi0
	.byte $20
	
	seg hi1
	org StartHi1
	rorg $1800
Hi1
	.byte $22
	
	seg hi2
	org StartHi2
	rorg $1800
Hi2
	.byte $24
	
	seg hi3
	org StartHi3
	rorg $1800
Hi3
	.byte $26
	
	seg hi4
	org StartHi4
	rorg $1800
Hi4
	.byte $28
	
	seg hi5
	org StartHi5
	rorg $1800
Hi5
	.byte $2A
	
	seg hi6
	org StartHi6
	rorg $1800
Hi6
	.byte $2C
	
	seg hi7
	org StartHi7
	rorg $1800
Hi7
	.byte $2E
	
	seg hi8
	org StartHi8
	rorg $1800
Hi8
	.byte $30
	
	seg hi9
	org StartHi9
	rorg $1800
Hi9
	.byte $32
	
	seg hiA
	org StartHiA
	rorg $1800
HiA
	.byte $34
	
	seg hiB
	org StartHiB
	rorg $1800
HiB
	.byte $36
	
	seg hiC
	org StartHiC
	rorg $1800
HiC
	.byte $38
	
	seg hiD
	org StartHiD
	rorg $1800
HiD
	.byte $3A
	
	seg hiE
	org StartHiE
	rorg $1800
HiE
	.byte $3C
	


	seg hiF
	org StartHiF
	rorg $1800
HiF
	.byte $3E
	
	org $FF00
	rorg $1F00		;last page of ROM, always mapped at $1F00-$1FFF
	
Start
	CLEAN_START
	
	nop $6D00	;Page 0 of RAM at $1E00
	dex
	stx PF1
	stx CTRLPF
	
Start_Frame

	lda $2
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
	
	ldy #15
	ldx #0
LoBanks
	nop $6E00,X
	lda $1000
	sta COLUBK
	inx
	sta WSYNC
	sta WSYNC
	sta WSYNC
	sta WSYNC
	sta WSYNC
	sta WSYNC
	dey
	bne LoBanks
	
	ldy #15
	ldx #0
Rest
	nop $6F10,X
	lda $1800
	sta COLUBK
	inx
	sta WSYNC
	sta WSYNC
	sta WSYNC
	sta WSYNC
	sta WSYNC
	sta WSYNC
	dey
	bne Rest
	
	lda #2
	sta VBLANK
	
	ldy #30
OverScan
	sta WSYNC
	dey
	bne OverScan
	
	inc $1E00
	lda $1E00
	sta COLUPF
	
	jmp Start_Frame
	

	
	org $FFF8
	rorg $1FF8
	.word $0001
	.word $4A50
	.word Start
	.word Start
