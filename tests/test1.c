struct myStruct
{
    uint8_t myVar;
};

uint8_t incrementNumber(uint8_t num)
{
    return num + 9;
}

void playSound()
{
    //__asm LDA #$FD
    //__asm STA $4015
    uint8_t lala = 253; // #$FD
    __asm LDA lala
    __asm STA $4015
    
    __asm lda #$3F
    __asm sta $4000

    __asm lda #$3F
    __asm sta $4001

    __asm lda #$FB
    __asm sta $4002

    __asm lda #$F9
    __asm sta $4003
}

void main()
{
    playSound();
    
    uint8_t var1 = incrementNumber(3);
    uint8_t var2 = 2 + var1;
    //var2 = 4;
}
