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
	LDR R4,=RESULT

	LDRB R1,[R0]

	; Get tens digit
	AND R2,R1,#0xF0
	MOV R2,R2,LSR #04

	; Multiply tens digit by 10
	MOV R3,#10
	MUL R2,R2,R3

	; Get units digit
	AND R3,R1,#0x0F

	; Add units digit
	ADD R2,R2,R3

	; Store hexadecimal result
	STRB R2,[R4]

STOP
	B STOP

	ALIGN

NUM
	DCD 0x00000025

	AREA mydata, DATA, READWRITE

RESULT
	DCD 0

	END