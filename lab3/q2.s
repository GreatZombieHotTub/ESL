	AREA	RESET,DATA,READONLY
	EXPORT	__Vectors

__Vectors
	DCD	0x10001000
	DCD	Reset_Handler

	AREA	mycode,CODE,READONLY
	ENTRY
	EXPORT	Reset_Handler

Reset_Handler

	LDR R0,=VALUE1
	LDR R1,=VALUE2
	LDR R2,=RESULT

	MOV R3,#4	; 

        ; First word
	LDR	R4,[R0],#4
	LDR	R5,[R1],#4
	ADDS	R6,R4,R5
	STR	R6,[R2],#4
	SUBS	R3,R3,#1

LOOP
	BEQ	STOP	; Done after 4 words

	LDR	R4,[R0],#4
	LDR	R5,[R1],#4

	ADCS	R6,R4,R5	; Add with carry
	STR	R6,[R2],#4

	SUBS	R3,R3,#1
	BNE LOOP

STOP
	B	STOP

VALUE1
	DCD	0xFFFFFFFF
	DCD	0
	DCD	0
	DCD	0

VALUE2
	DCD	1
	DCD	0
	DCD	0
	DCD	0

	AREA data,DATA,READWRITE

RESULT
	DCD	0,0,0,0

	END