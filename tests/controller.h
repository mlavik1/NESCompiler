uint8_t ctrl0_buttons;

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


