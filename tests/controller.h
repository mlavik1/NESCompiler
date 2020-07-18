uint8_t ctrl0_buttons;

#define CTRL_BUTTON_A 128
#define CTRL_BUTTON_B 64
#define CTRL_BUTTON_SELECT 32
#define CTRL_BUTTON_START 16
#define CTRL_BUTTON_UP 8
#define CTRL_BUTTON_DOWN 4
#define CTRL_BUTTON_LEFT 2
#define CTRL_BUTTON_RIGHT 1

uint8_t read_ctrl_0()
{
	__asm clc
	__asm lda #$01
	__asm sta $4016
	__asm sta ctrl0_buttons
	__asm lsr A
	__asm sta $4016
	
	__asm lda $4016
	__asm lsr A
	__asm rol ctrl0_buttons
	__asm lda $4016
	__asm lsr A
	__asm rol ctrl0_buttons
	__asm lda $4016
	__asm lsr A
	__asm rol ctrl0_buttons
	__asm lda $4016
	__asm lsr A
	__asm rol ctrl0_buttons
	__asm lda $4016
	__asm lsr A
	__asm rol ctrl0_buttons
	__asm lda $4016
	__asm lsr A
	__asm rol ctrl0_buttons
	__asm lda $4016
	__asm lsr A
	__asm rol ctrl0_buttons
	__asm lda $4016
	__asm lsr A
	__asm rol ctrl0_buttons
	return ctrl0_buttons;	
}


