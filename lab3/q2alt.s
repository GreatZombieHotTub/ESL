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

        LDR R0, =VALUE1
        LDR R1, =VALUE2
        LDR R2, =RESULT

        MOV R3, #0            ; Index

        LDR R4,[R0,R3]
        LDR R5,[R1,R3]
        ADDS R6,R4,R5
        STR R6,[R2,R3]

        ADD R3,R3,#4

        LDR R4,[R0,R3]
        LDR R5,[R1,R3]
        ADCS R6,R4,R5
        STR R6,[R2,R3]

        ADD R3,R3,#4

        LDR R4,[R0,R3]
        LDR R5,[R1,R3]
        ADCS R6,R4,R5
        STR R6,[R2,R3]

        ADD R3,R3,#4

        LDR R4,[R0,R3]
        LDR R5,[R1,R3]
        ADCS R6,R4,R5
        STR R6,[R2,R3]

STOP
        B STOP

VALUE1
        DCD 0xFFFFFFFF
        DCD 0x11111111
        DCD 0x22222222
        DCD 0x33333333

VALUE2
        DCD 0x00000001
        DCD 0x22222222
        DCD 0x33333333
        DCD 0x44444444

        AREA data, DATA, READWRITE

RESULT
        DCD 0,0,0,0

        END