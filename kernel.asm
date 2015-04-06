;;kernel.asm

;nasm directive - 32 bit
bits 32
section .text
        ;multiboot spec
        align 4
        dd 0x1BADB002            ;magic
        dd 0x00                  ;flags
        dd - (0x1BADB002 + 0x00) ;checksum. m+f+c should be zero

global start
global keyboard_handler
global timer
global read_port
global write_port
global load_idt
global timer_handler
global asmtest

extern kmain	        
extern keyboard_handler_main
extern timer_handler_main
extern test
extern test2

load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti
	ret

;READING AND WRITING TO PORTS

read_port:
	mov edx, [esp + 4]
	in al, dx	
	ret

write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret
	
;KEYBOARD Interrupt Handling
keyboard_handler:     
	mov	eax, esp
	push	eax
            
	call    keyboard_handler_main
	
	pop	eax
	iretd

;TIMER Interrupt Handling
timer_handler:
	call	timer_handler_main
	iretd

asmtest:
	mov 	eax, [esp + 4]
	push 	eax

	mov	eax, esp
	add	eax, 4h
	push	eax

	;call 	test

	pop 	eax
	pop 	eax

	ret

start:
  cli 			;block interrupts
  call kmain
  hlt		 	;halt the CPU
