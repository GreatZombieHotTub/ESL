	AREA	RESET,DATA,READONLY
	EXPORT	__Vectors
	
__Vectors
	DCD	0x10001000
	DCD	Reset_Handler
	ALIGN
	AREA	mycode,CODE,READONLY
	ENTRY
	EXPORT	Reset_Handler
	
Reset_Handler
	LDR	R0,=VAL1	;
	LDR	R1,=VAL2	;
	LDR	R2,=RESULT	;
	
	MOV	R3,#0	;
	
	;first 32 bit word
	LDR	R4,[R0,R3]
	LDR	R5,[R1,R3]
	SUBS	R6,R4,R5
	STR	R6,[R2,R3]
	ADD	R3,R3,#4
	
LOOP
	CMP	R3,#16	;
	BEQ	STOP
	LDR	R4,[R0,R3]
	LDR	R5,[R1,R3]
	SBCS	R6,R4,R5
	STR	R6,[R2,R3]
	ADD	R3,R3,#4
	B	LOOP
	
STOP
	B	STOP
	
VAL1
	DCD	20
	DCD	10
	DCD	5
	DCD	2
	
VAL2
	DCD	5
	DCD	2
	DCD	1
	DCD	1
	
	AREA	data,DATA,READWRITE
	
RESULT
	DCD	0,0,0,0
	
	END
	