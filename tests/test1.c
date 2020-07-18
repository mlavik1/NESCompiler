#include "controller.h"

struct myStruct
{
    uint8_t myVar;
};

void play_sound();
void change_note();
uint8_t readController();

uint8_t increment_number(uint8_t num)
{
    return num + 1;
}

uint8_t note;
uint8_t counter;

void main()
{
    note = 0;
    counter = 0;
    
    play_sound();
    
    while(1 == 1)
    {
        uint8_t ctrlval = read_ctrl_0();
        if(ctrlval != CTRL_BUTTON_A)
        {
            counter = counter + 1;
        }
        if(counter == 250)
        {
            note = increment_number(note);
            counter = 0;
            change_note();
        }
    }
}

void play_sound()
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

void change_note()
{
    __asm lda note
    __asm sta $4002
}

