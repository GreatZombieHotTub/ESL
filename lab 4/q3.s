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

	MOV R2,#0
	MOV R3,R1

	; Find tens digit
TEN
	CMP R3,#10
	BLO UNIT

	SUB R3,R3,#10
	ADD R2,R2,#1
	B TEN

	; R2 = tens digit
	; R3 = units digit

UNIT
	MOV R2,R2,LSL #04
	ORR R2,R2,R3

	; R2 = BCD result

STOP
	B STOP

	ALIGN

NUM
	DCD 0x00000019

	END