	AREA RESET, DATA, READONLY
	EXPORT __Vectors

__Vectors
	DCD 0x40001000
	DCD Reset_Handler

	ALIGN

	AREA mycode, CODE, READONLY
	ENTRY
	EXPORT Reset_Handler

Reset_Handler
	LDR R0,=NUM
	LDRB R1,[R0]

	; Get first digit using F0 mask
	AND R2,R1,#0xF0
	MOV R2,R2,LSR #04

	CMP R2,#09
	BLO FIRST
	ADD R2,#07

FIRST
	ADD R2,#0x30

	; Shift first ASCII digit
	MOV R2,R2,LSL #08

	; Get second digit using 0F mask
	AND R3,R1,#0x0F

	CMP R3,#09
	BLO SECOND
	ADD R3,#07

SECOND
	ADD R3,#0x30

	; Combine ASCII digits
	ORR R2,R2,R3

STOP
	B STOP
	ALIGN

NUM
	DCD 0x00000021

	END