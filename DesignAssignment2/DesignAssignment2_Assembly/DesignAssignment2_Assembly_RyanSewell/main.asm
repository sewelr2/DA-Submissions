;
; DesignAssignment2_Assembly_RyanSewell.asm
;
; Created: 03/08/25 13:44:00
; Author : sewel
;

.org 0x00 ;set program origin in memory
	ldi r16, 0b11111101 ;r16 = 0xFD 
	out DDRC, r16 ;Set PC1 as input

	ldi r16, 0b00100000 ;r16 = 0x20
	out DDRB, r16 ;Set PB5 as output
	clr r16 ;clear r16

MAIN:
	in r17, PINC ;Read PINC into r17
	sbrs r17, 1 ;Skip next instruction if switch is pressed
	rjmp MAIN ;Continue polling if switch is not pressed
	
	ldi r18, 10
	sbi PORTB, 5 ;Turn on LED
;1.5 sec delay routine
DELAY_LOOP: dec r18 ;Loop 10 times
	call DELAY_150ms ;Call 150ms delay subroutine
	brne DELAY_LOOP ;Loop until r18 = 0
	cbi PORTB, 5 ;Turn off LED

	rjmp MAIN ;Begin polling again


;150ms delay subroutine
DELAY_150ms:
	ldi r19, 125 ;Set outer loop counter to 125
L1:
	ldi r20, 250 ;Set inner loop counter to 250
L2:
	nop ;1 cc
	dec r20
	brne L2 ;Loop 250 times
	dec r19
	brne L1 ;Loop 125 times
	ret


