/*
*  kernel.c
*/
#include "keyboard_map.h"

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define MAX_ROW 25
#define MAX_COL 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * MAX_COL * MAX_ROW

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C

extern unsigned char keyboard_map[128];
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

unsigned int col = 0;		//col & row corresponds to the monitor
unsigned int row = 0;
char *vidptr = (char*)0xb8000; 	//video mem begins here.
unsigned int cursor = 0;	//var where cursor is remembered
char* headline = "GrabOS>";
char hexNumbers[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

struct IDT_entry{
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

static inline void outb(unsigned short port, unsigned char val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline char inb(unsigned short port)
{
    unsigned char ret;
    asm volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/*     Ports
	*	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	/* ICW1 - begin initialization */
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	/* ICW4 - environment info */
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(0x21 , 0xFD);
}

void keyboard_handler_main(void) {
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;
		vidptr[cursor++] = keyboard_map[keycode];
		vidptr[cursor++] = 0x07;	
	}
}

/*Moves the cursor using data from col and row*/
void moveCursor() {
	int position;

	if(col > MAX_COL)
		col = 0;

	cursor = ((row * 80) + col) * 2;
	position = cursor/2;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (unsigned char)(position&0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (unsigned char)((position>>8)&0xFF));
}

int absoluteVal(int num) {
	if(num >= 0)
		return num;
	else return -num;
}

void sleep(int delay) {
	int i;	
	for(i = 0; i < delay * 100000; i++);
}

void clrscr() {
	unsigned int j = 0;
	while(j < MAX_COL * MAX_ROW * 2) {
		//blank character
		vidptr[j] = ' ';
		//attribute-byte: light grey on black screen	
		vidptr[j+1] = 0x07; 		
		j = j + 2;
	}
	col = 0;
	row = 0;
	moveCursor();
}

void printStr(char *str) {
	unsigned int j = 0;		//var used in traversing char* str;	

	while(str[j] != '\0') {
		if(str[j] != '\n') {
			vidptr[cursor] = str[j];
			vidptr[cursor+1] = 0x07;
			col++;
		} else {
			col = 0;
			++row;
		}
		moveCursor();
		++j;	
	}
}

void printInt(int num) {
	unsigned int ASCII_VALUE = 48;
	int numTemp;
	int i, j;
	char* numstr;
	char charTemp;
	unsigned int ctr;
	 
	if(num < 0)
		printStr("-");

	num = absoluteVal(num);
	
	if(num == 0) {
		printStr("0");
	} else {
		for(ctr = 0, numTemp = num; numTemp > 0; ctr++, numTemp /= 10) {
			numstr[ctr] = ((numTemp % 10) + ASCII_VALUE);
		} numstr[ctr] = '\0';

		for(i = 0, j = ctr - 1; i <= j; i++, j--) {
			charTemp = numstr[i];
			numstr[i] = numstr[j];
			numstr[j] = charTemp;
		}
		printStr(numstr);
	} 
	
	
}

void printHex(int hex) {
	char* bin;
	int ctr, i, j, temp;
	char charTemp;
	
	if(hex == 0) {
		bin[0] = hexNumbers[hex];
		bin[1] = '\0';
	} else {
		for(ctr = 0, temp = hex; temp > 0; ctr++, temp /= 16) {
			bin[ctr] = hexNumbers[temp % 16];
		} 
	
		for(i = 0, j = ctr - 1; i <= j; i++, j--) {
			charTemp = bin[i];
			bin[i] = bin[j];
			bin[j] = charTemp;
		}
		bin[ctr] = '\0';
	}
	
	printStr(bin);
}

unsigned char get_scancode()
{
    unsigned char inputdata, temp;
   
	do{
   		inputdata = inb(0x60);
		if(inputdata != temp) {
			clrscr();
			printStr(headline);
		 	printHex(inputdata);
			temp = inputdata;
		}
		//printStr(&inputdata);
	}while(inputdata != 0);

    return inputdata;
}

void scroll() {
	int i;
	
	//i = MAX_COL * MAX_ROW * 2;
	i = 0;
	while(i >= MAX_COL * MAX_ROW * 2) {
		vidptr[i] = vidptr[i + 160];	
		i++;
		row--;
	}

	for(i = 1; i < 160; i += 2) 
		vidptr[i] = 0x07;
	moveCursor();
	
}

void kmain(void)
{	
	int i;
	clrscr();
	printStr("\n");	
	printStr("             ======================================================\n");
	printStr("                     /^--^\\     /^--^\\     /^--^\\     /^--^\\\n");
	printStr("                    |      |   |      |   |      |   |      |\n");
	printStr("                     \\____/     \\____/     \\____/     \\____/\n");
	printStr("                    /      \\   /      \\   /      \\   /      \\\n");
	printStr("                   |        | |        | |        | |        |\n");
	printStr("                    \\__  __/   \\__  __/   \\__  __/   \\__  __/\n");
	printStr("             |^|^|^|^|/ /^|^|^|^|^\\ \\^|^|^|^/ /^|^|^|^|^\\ \\^|^|^|^|\n");
	printStr("             | | | | / /  | | | | |\\ \\| | |/ /| | | | | |\\ \\ | | ||\n");
	printStr("             | | | | / /  | | | | |\\ \\| | |/ /| | | | | |\\ \\ | | ||\n");
	printStr("             ########\\ \\###########/ /#####\\ \\###########/ /#######\n");
	printStr("             | | | | |\\/| | |  | | \\/| | | |\\/| | | | | |\\/ | | | |\n");
	printStr("             |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_||\n");
	printStr("             ======================================================\n");
	printStr("                       _____           _      ____   _____ \n");
	printStr("                      / ____|         | |    / __ \\ / ____|\n");
	printStr("                     | |  __ _ __ __ _| |__ | |  | | (___  \n");
	printStr("                     | | |_ | '__/ _` | '_ \\| |  | |\\___ \\ \n");
	printStr("                     | |__| | | | (_| | |_) | |__| |____) |\n");
	printStr("                      \\_____|_|  \\__,_|_.__/ \\____/|_____/ \n");
	sleep(1000);
	clrscr();

	printStr(headline);
	idt_init();
	kb_init();

	while(1);
	return;
}
