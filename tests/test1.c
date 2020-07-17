struct myStruct
{
    uint8_t myVar;
};

void playSound();
void changeNote();

uint8_t incrementNumber(uint8_t num)
{
    return num + 1;
}

uint8_t note;
uint8_t counter;

void main()
{
	note = 0;
	counter = 0;
	
    playSound();
	
    while(1 == 1)
    {
		counter = counter + 1;
		if(counter == 250)
		{
			note = incrementNumber(note);
			counter = 0;
			changeNote();
		}
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

void changeNote()
{
    __asm lda note
    __asm sta $4002
}
