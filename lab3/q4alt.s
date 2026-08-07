        AREA RESET, DATA, READONLY
        EXPORT __Vectors

__Vectors
        DCD 0x10001000
        DCD Reset_Handler

        ALIGN

        AREA mycode, CODE, READONLY
        ENTRY
        EXPORT Reset_Handler

Reset_Handler

        LDR R0,=VALUE1
        LDR R1,[R0]

        LDR R0,=VALUE2
        LDR R3,[R0]

        MOV R4,#1          ; i

LOOP

        MUL R5,R1,R4       ; i*a

        MOV R6,R5          ; Temporary value

MOD

        CMP R6,R3
        BLT CHECK

        SUB R6,R6,R3
        B MOD

CHECK

        CMP R6,#0
        BEQ STORE

        ADD R4,R4,#1
        B LOOP

STORE

        LDR R7,=RESULT
        STR R5,[R7]

STOP
        B STOP

VALUE1
        DCD 12

VALUE2
        DCD 18

        AREA data,DATA,READWRITE

RESULT
        DCD 0

        END