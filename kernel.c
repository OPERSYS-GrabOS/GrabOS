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
#define PARAM_LIMIT 9

#define ENTER_KEY_CODE 0x1C
#define NUM_OF_PROCESSES 6

#define BLACK 0x00
#define BLUE 0x01
#define GREEN 0x02
#define CYAN 0x03
#define RED 0x04
#define MAGENTA 0x05
#define ORANGE 0x06
#define WHITE 0x07

extern unsigned char keyboard_map[128];
extern unsigned char hexNumbers[16];
extern void switch_handler(void);
extern void keyboard_handler(void);
extern void timer_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);
extern int asmtest(int);

unsigned int col = 0;		//col & row corresponds to the monitor
unsigned int row = 0;
unsigned int buffer_counter = 0;
char *vidptr = (char*)0xb8000; 	//video mem begins here.
unsigned int cursor = 0;	//var where cursor is remembered
char* headline = "GrabOS>";
unsigned char console_buffer[1000];
unsigned char command[25];
unsigned char param[100];
unsigned char intParam[10];	//for arithmetic parameters
unsigned char cap_flag = 0;
unsigned char* console_option;
char numstr[8];	

int currProg = 0;

struct IDT_entry{
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct marquee{
	unsigned char str[100];
	unsigned int line;
	unsigned int begin_pos;		//position of the first char
	unsigned int end_pos;		//position of the last char
	unsigned int direction;		//direction marquee is moving || 1-right ;; 0-left
	unsigned int color;
} mar;

typedef struct process{
	int eip;	
	int ebp;
	int esp;
	int stack[1024];
	
	int progNum;
	char screen[SCREENSIZE];
	unsigned int col;
	unsigned int row;
}processNode;

processNode PCS[NUM_OF_PROCESSES];
struct IDT_entry IDT[IDT_SIZE];
struct marquee MRQ[MAX_ROW];

unsigned int PCS_CTR = 0;
unsigned int MRQ_CTR = 0;

static inline void outb(unsigned short, unsigned char);
static inline char inb(unsigned short);

void idt_init(void);
void kb_init(void);

void move_cursor();
int absoluteVal(int);
void sleep(int);
void clrscr();
void clrLine(int);

int strcmp(char*, char*);
void strcopy(char*, char*);
int len(char*);

void printStr(char*);
void printInt(int);
void printHex(int);

void scroll();
void shiftRight(int);
void shiftLeft(int);

void keyboard_handler_main(int*);
void timer_handler_main(void);

void doCommand(int*);
void processKey(char, int*);
void commandSay();
void commandRun(int*);

void moveMarquee(struct marquee*);
void removeMarquee(int index);
char* findOption();

void switchConsole(int*);
void createProcess(int*, int, int);
void restore(int*, int);
void backup(int*);

void prog1();
void prog2();
void prog3();
void prog4();
void prog5();
void switchProg();
void kmain(void);

void init_stack()
{	
	
	PCS[0].progNum = 0;
	PCS[0].ebp = (int)&(PCS[0].stack[1024]); 
	PCS[0].esp = (int)&(PCS[0].stack[1024]);
	PCS[0].eip = (int)&switchProg;
	PCS[1].progNum = 0;
	PCS[1].ebp = (int)&(PCS[0].stack[1024]); 
	PCS[1].esp = (int)&(PCS[0].stack[1024]);
	PCS[1].eip = (int)&kmain;
	PCS_CTR+=2;
	currProg = 1;
	/*PCS[1].ebp = (int)&(PCS[1].stack[1024]); 
	PCS[1].esp = (int)&(PCS[1].stack[1024]);
	PCS[1].eip = (int)&prog1;
	PCS[2].ebp = (int)&(PCS[2].stack[1024]); 
	PCS[2].esp = (int)&(PCS[2].stack[1024]);
	PCS[2].eip = (int)&prog2;*/
}

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
	unsigned long func_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	func_address = (unsigned long)keyboard_handler;
	IDT[0x21].offset_lowerbits = func_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (func_address & 0xffff0000) >> 16;

	/* populate IDT entry of timer's interrupt */
	func_address = (unsigned long) timer_handler;
	IDT[0x20].offset_lowerbits = func_address & 0xffff;
	IDT[0x20].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x20].zero = 0;
	IDT[0x20].type_attr = INTERRUPT_GATE;
	IDT[0x20].offset_higherbits = (func_address & 0xffff0000) >> 16;

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
	write_port(0x21 , 0xFC);

}

void kb_init(void) {
	write_port(0x21, 0xFC);
}

//http://www.programmingsimplified.com/c/source-code/c-program-convert-string-to-integer-without-using-atoi-function
int ParseInt(char a[]) 
{
	  int c, sign, offset, n;
	 
	  if (a[0] == '-') {  // Handle negative integers
	    sign = -1;
	  }
	 
	  if (sign == -1) {  // Set starting position to convert
	    offset = 1;
	  }
	  else {
	    offset = 0;
	  }
	 
	  n = 0;
	 
	  for (c = offset; a[c] != '\0'; c++) {
	    n = n * 10 + a[c] - '0';
	  }
	 
	  if (sign == -1) {
	    n = -n;
	  }
	 
	  return n;
}

void scroll() {
	int ctr;
	for(ctr = 0; ctr < SCREENSIZE - 160; ctr += 2) {
		vidptr[ctr] = vidptr[ctr + 160];
		vidptr[ctr + 1] = vidptr[ctr + 161];	
	}
	for(;ctr < SCREENSIZE; ctr += 2) {
		vidptr[ctr] = ' ';
		vidptr[ctr + 1] = WHITE;
	}
	for(ctr = 0; ctr < MRQ_CTR; ctr++) {
		if(MRQ[ctr].line < 0)
			removeMarquee(ctr);
		MRQ[ctr].line--;
	}
}

/*Moves the cursor using data from col and row*/
void moveCursor() {
	int position;

	if(col > MAX_COL) {
		col = 0;
		row++;
	}
	
	if(row >= MAX_ROW) 
	{
		scroll();
		row = 24;		
	}

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

int strcmp(char* str1, char* str2) {
	int i = 0;
	while(str1[i] != '\0' && str2[i] != '\0') {
		if(str1[i] != str2[i]) {
			return 1;
		}
		i++;
	}
	return 0;
}

void strcopy(char* dest, char* src) {
	int i = 0;

	while(src[i] != '\0') {
		dest[i] = src[i];
		i++;	
	}
}

int len(char* str) {
	int length = 0;
	while(str[length] != '\0')
		length++;

	return length;
}

void getCommand() {
	int i, j;
	for(i = 0; console_buffer[i] != ' ' && i < 25; i++) {
		command[i] = console_buffer[i];
	}i++;
	for(j = 0; console_buffer[i] != '\0'; i++, j++) {
		param[j] = console_buffer[i];
	}
	param[j] = '\0';
}

void clrscr() {
	unsigned int j = 0;
	while(j < MAX_COL * MAX_ROW * 2) {
		//blank character
		vidptr[j] = ' ';
		//attribute-byte: light grey on black screen	
		vidptr[j+1] = WHITE; 		
		j = j + 2;
	}
	col = 0;
	row = 0;
	MRQ_CTR = 0;
	moveCursor();
}

/*Main use for marquees*/
void clrLine(int line) {
	unsigned int j;
	unsigned int pos;
	
	for(j = 0; j < MAX_COL; j++) {
		pos = ((line * 80) + j) * 2;
		vidptr[pos] = ' ';
		vidptr[pos + 1] = WHITE;	
	}
}

void clrstr(char* str) {
	unsigned int i;
	for(i = 0; str[i] != '\0'; i++)
		str[i] = '\0';
}

void printStr(char *str) {
	unsigned int j = 0;		//var used in traversing char* str;	

	while(str[j] != '\0') {
		if(str[j] != '\n') {
			vidptr[cursor] = str[j];
			vidptr[cursor+1] = WHITE;
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
	int ASCII_VALUE = 48;
	int numTemp;
	int i, j;
	//char charTemp;
	int ctr = 0;
	 

	if(num < 0)
		printStr("-");

	num = absoluteVal(num);

	if(num == 0) {
		printStr("0");
	} else {
		for(numTemp = num; numTemp > 0; ctr++, numTemp /=10);
		
		numTemp = num;
		//if(ctr >= 8) {
			for(i = 0; i < 8; i++)
				numstr[i] = '\0';
			numstr[ctr] = '\0';
			ctr--;
			for(i = ctr; i>=0; i--)
			{
				numstr[i] = ((numTemp % 10) + ASCII_VALUE);
				numTemp /= 10;
			}
			printStr(numstr);
		//} else printStr("Integer too long!");	
		/*
		for(ctr = 0, numTemp = num; numTemp > 0; ctr++, numTemp /= 10) {
			numstr[ctr] = ((numTemp % 10) + ASCII_VALUE);
		} numstr[ctr] = '\0';

		for(i = 0, j = ctr - 1; i <= j; i++, j--) {
			charTemp = numstr[i];
			numstr[i] = numstr[j];
			numstr[j] = charTemp;
		}
		printStr(numstr);*/
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

char* firstParam() {
	int i;
	
	for(i = 0; param[i] != ' ' && i < PARAM_LIMIT; i++) {
		intParam[i] = param[i];
	}intParam[i] = '\0';/*terminates ret*/
	return intParam;
}

char* secondParam() {
	int i, j;
	
	for(i = 0; param[i] != ' ' && i < 25; i++);i++;
	for(j = 0; (param[i] != ' ' || param[i] != '\0') && i < 25 && j < 4; i++, j++)
		intParam[j] = param[i];
	intParam[j] = '\0';/*terminates ret*/
	return intParam;
}

void clearBuffer() {
	clrstr(console_buffer);
	clrstr(command);
	clrstr(param);
	clrstr(console_option);
	buffer_counter = 0;	
}

void commandSay() {
	printStr(param);
}

void commandAdd() {
	int sum = ParseInt(firstParam());	
	
	sum += ParseInt(secondParam());
	printInt(sum);	
/*
	int i = 0, j = 0;
	char* num;	

	while(param[i] != '\0')
	{
		if(param[i] != ' ')
		{
			num[j] = param[i];
			num[j+1] = '\0';
			j++;
		}
		else if(param[i] == ' ')
		{
			sum += ParseInt(num);
		}
		i++;
		
	}*/
	
}

void commandMarquee() {
	console_option = findOption();

	if(strcmp(console_option, "-black") == 0)
		mar.color = BLACK;
	else if(strcmp(console_option, "-blue") == 0)
		mar.color = BLUE;
	else if(strcmp(console_option, "-green") == 0)
		mar.color = GREEN;
	else if(strcmp(console_option, "-cyan") == 0)
		mar.color = CYAN;
	else if(strcmp(console_option, "-red") == 0)
		mar.color = RED;
	else if(strcmp(console_option, "-magenta") == 0)
		mar.color = MAGENTA;
	else if(strcmp(console_option, "-orange") == 0)
		mar.color = ORANGE;
	else if(strcmp(console_option, "0") == 0 || strcmp(console_option, "-white") == 0)
		mar.color = WHITE;
	else {
		printStr(console_option);
		printStr(" is not a valid option");
		return;
	}

	strcopy(mar.str, param);
	mar.line = row;	
	mar.begin_pos = 0;
	mar.end_pos = len(mar.str);
	mar.direction = 1;
	
	MRQ[MRQ_CTR] = mar;
	MRQ_CTR++;
	
	clrstr(mar.str);
}

void commandRun(int* ptr) {
	if(strcmp(param, "prog1") == 0) {
		createProcess(ptr, (int)&prog1, 1);
	} else if(strcmp(param, "prog2") == 0) {
		createProcess(ptr, (int)&prog2, 2);
	} else if(strcmp(param, "prog3") == 0) {
		createProcess(ptr, (int)&prog3, 3);
	} else if(strcmp(param, "prog4") == 0) {
		createProcess(ptr, (int)&prog4, 4);
	} else if(strcmp(param, "prog5") == 0) {
		createProcess(ptr, (int)&prog5, 5);
	} else {
		printStr("Program does not exist\n");
	}
}

void doCommand(int* ptr) {
	int prog;
	int i;

	getCommand();
	if(currProg == 0) {
		i = ParseInt(command);
		restore(ptr, i);
	}else if(strcmp(command, "say") == 0) {
		printStr("\n");		
		commandSay();	
		printStr("\n");
		printStr(headline);
	}else if(strcmp(command, "add") == 0) {
		printStr("\n");		
		commandAdd();
		printStr("\n");
		printStr(headline);
	}else if(strcmp(command, "cls") == 0) {
		clrscr();
		printStr(headline);	
	}else if(strcmp(command, "marquee") == 0){
		printStr("\n");
		commandMarquee();
		printStr("\n");
		printStr(headline);
	}else if(strcmp(command, "clrline") == 0){
		clrLine(row);
		printStr("\n");
		printStr(headline);
	}else if(strcmp(command, "run") == 0) {
		printStr("\n");
		commandRun(ptr);
	} else {
		printStr("\nInvalid Command\n");
		printStr(headline);
	}

	clearBuffer();
}

int isChar(char key) {
	if(key >= 32 && key <= 126)
		return 1;
	else return 0;
}

void processKey(char key, int* ptr) {
	char in = keyboard_map[key];
	
	if(cap_flag == 1 && in != '\b' && in == '\n')
		in -= 32;
	if(key < 0)
		return;
	else if(in == '\n'){
		if(buffer_counter != 0) 
			doCommand(ptr);
		else {
			printStr("\n");
			printStr(headline);	
		}
	} else if(keyboard_map[key] == '`') {
		backup(ptr);
		restore(ptr, 0);
	} else if(keyboard_map[key] == '\b' && buffer_counter > 0) {
		vidptr[cursor-2] = ' ';

		console_buffer[buffer_counter] = '\0';
		buffer_counter--;
		col--;
	} else { 
	
		vidptr[cursor] = keyboard_map[key];
		vidptr[cursor+1] = WHITE;
	
		console_buffer[buffer_counter] = keyboard_map[key];
		buffer_counter++;
		console_buffer[buffer_counter] = '\0';
		col++;	
	} 
	
	moveCursor();
}

void createProcess(int* ptr, int prog, int progNum) {
	backup(ptr);
	PCS[PCS_CTR].progNum = progNum;
	PCS[PCS_CTR].ebp = (int)&(PCS[PCS_CTR].stack[1024]); 
	PCS[PCS_CTR].esp = (int)&(PCS[PCS_CTR].stack[1024]);
	PCS[PCS_CTR].eip = prog;
	
	restore(ptr, PCS_CTR);
	PCS_CTR++;
}

void backup(int* ptr) {
	int i;
	
	PCS[currProg].ebp = ptr[0];
	PCS[currProg].eip = ptr[1];
	PCS[currProg].esp = ptr[-1];

	for(i = 0; i < SCREENSIZE; i++) {
		PCS[currProg].screen[i] = vidptr[i];
	}
	PCS[currProg].col = col;
	PCS[currProg].row = row;
}

void restore(int* ptr, int PCS_index) {
	int i;
	clrscr();
	
	currProg = PCS_index;
	ptr[0] = PCS[PCS_index].ebp;	
	ptr[1] = PCS[PCS_index].eip;
	ptr[-1] = PCS[PCS_index].esp;
	
	for(i = 0; i < SCREENSIZE; i++) {
		vidptr[i] = PCS[PCS_index].screen[i];	
	}
	col = PCS[PCS_index].col;
	row = PCS[PCS_index].row;
	
	moveCursor();
}

void keyboard_handler_main(int* ptr) {
	unsigned char status;
	char keycode;

	status = read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	
		if (status & 0x01) {
			keycode = read_port(KEYBOARD_DATA_PORT);
			processKey(keycode, ptr);
		}
	
	/* write EOI */
	write_port(0x20, 0x20);
}

void timer_handler_main(void) {
	int ctr;

	for(ctr = 0; ctr < MRQ_CTR; ctr++) {
		moveMarquee(&MRQ[ctr]);
	}
	write_port(0x20, 0x20);
}

void moveMarquee(struct marquee* mar) {
	unsigned int start;
	unsigned int pos;
	unsigned int index;
	
	if((*mar).direction == 1) {
		(*mar).begin_pos++;
		(*mar).end_pos++;	
		if((*mar).end_pos == MAX_COL)
			(*mar).direction = 0;
	} else {
		(*mar).begin_pos--;
		(*mar).end_pos--;
		if((*mar).begin_pos == 0)
			(*mar).direction = 1;	
	}
	clrLine((*mar).line);
 	for(start = (*mar).begin_pos, index = 0; index < len((*mar).str); start++, index++){
		if((*mar).line >= 0) {	
			pos = (((*mar).line * 80) + start) * 2;
			vidptr[pos] = (*mar).str[index];
			vidptr[pos+1] = (*mar).color;
		}
  	}			
}

char* findOption() {
	int i, j;

	if(param[0] == '-') {
		for(i = 0; param[i] != ' ' && i < 100; i++)
			console_option[i] = param[i];
		console_option[i] = '\0';
		for(j = 0; param[i] != '\0'; i++, j++)
			param[j] = param[i];

		param[j] = '\0';
		return console_option;
	} else return "0"; 
}

void switchProg() {
	int i;
	printStr("Program List:\n");
	for(i = 1; i < PCS_CTR; i++) {
		printStr("[");
		printInt(i);
		printStr("] ");
		switch(PCS[i].progNum) {
			case 0: printStr("console"); break;
			case 1: printStr("prog1"); break;
			case 2: printStr("prog2"); break;
			case 3: printStr("prog3"); break;
			case 4: printStr("prog4"); break;
			case 5: printStr("prog5"); break;
		}printStr("\n");
	}
	while(1);
}

void prog1()
{
	int x = 0, n = 0,i=0;
	while(1)
	{	
		printInt(x);
		printStr(" ");		
		for(n=0; n<9999; n++)
			for(i=0; i<9999; i++);
		x++;
	}
}

void prog2()
{
	int x = 0, n = 0,i=0;
	while(1)
	{
		printInt(x);
		printStr(" ");
		
		for(n=0; n<9999; n++)
			for(i=0; i<9999; i++);
		x += 2;
	}
}

void prog3(){
}

void prog4(){
}

void prog5(){
}

void kmain(void)
{
	idt_init();

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
	init_stack();
	kb_init();
	while(1);
	return;
}
