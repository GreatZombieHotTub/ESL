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
	LDR R0,=Numbers	;
	MOV	R1,#10	;
	MOV	R2,#0	;
	
LOOP
	LDR	R3,[R0]	;
	ADD	R0,R0,#4	;
	ADD	R2,R2,R3	;
	
	SUBS	R1,R1,#1	
	BNE	LOOP
	
	LDR	R4,=Result
	STR	R2,[R4]
	
STOP
	B	STOP
	
Numbers	
	DCD	1,2,3,4,5,6,7,8,9,10
	AREA	data,DATA,READWRITE	
	
Result	DCD	0

	END

	