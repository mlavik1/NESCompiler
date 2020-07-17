struct myStruct
{
    uint8_t myVar;
};

void playSound();

uint8_t incrementNumber(uint8_t num)
{
    return num + 9;
}

void main()
{
    uint8_t a = 2;
    uint8_t b = 1+2;
    if(a == b)
    {
        playSound();
    }
    else
    {
        playSound();
    }
    
    uint8_t var1 = incrementNumber(3);
    uint8_t var2 = 2 + var1;
    //var2 = 4;
    while(1 == 1)
    {
    }
}

void playSound()
{
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
