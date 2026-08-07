	AREA RESET, DATA, READONLY
	EXPORT __Vectors

__Vectors
	DCD 0x10001000
	DCD Reset_Handler	;

	AREA MYCODE, CODE, READONLY
	ENTRY
	EXPORT Reset_Handler

Reset_Handler

    LDR R0,=SRC	;
    ADD R0, R0, #36	;

    LDR R1,=DST	;
    ADD R1,R1, #36	;

    MOV R2,#10	;

BACK

    LDR	R3,[R0],#-4	;
    STR R3,[R1],#-4	;

    SUBS R2,R2,#1
    BNE BACK

STOP
	B	STOP

SRC	DCD 10,20,30,40,50,60,70,80,90,100


DST	SPACE	40

	END	;