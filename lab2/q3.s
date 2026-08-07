AREA RESET, DATA, READONLY
EXPORT __Vectors

__Vectors
DCD 0x10001000
DCD Reset_Handler

AREA MYCODE, CODE, READONLY
ENTRY
EXPORT Reset_Handler

Reset_Handler

    LDR R0, =ARRAY
    ADD R1, R0, #36     ; Address of last element

    MOV R2, #5          ; Only 5 swaps needed

LOOP

    LDR R3, [R0]        ; First element
    LDR R4, [R1]        ; Last element

    STR R4, [R0]        ; Place last at front
    STR R3, [R1]        ; Place first at end

    ADD R0, R0, #4      ; Move start forward
    SUB R1, R1, #4      ; Move end backward

    SUBS R2, R2, #1
    BNE LOOP

STOP
B STOP

AREA DATASEG, DATA, READWRITE

ARRAY
DCD 10,20,30,40,50,60,70,80,90,100

END