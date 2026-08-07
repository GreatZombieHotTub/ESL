	AREA	RESET,DATA,READONLY
	EXPORT	__Vectors

__Vectors
	DCD	0x10001000	;
	DCD	Reset_Handler	;
	
	ALIGN
	
	AREA	mycode,CODE,READONLY
	ENTRY
	EXPORT	Reset_Handler
	
Reset_Handler
	MOV	R0,#2	;
	MOV	R1,#4	;
	ADD R0,R0,R1	;
	LDR	R3,=SRC;
STOP	
	B	STOP
SRC DCD 8,0x123456; 
	AREA DATASEG, DATA, READWRITE
	END	;
	