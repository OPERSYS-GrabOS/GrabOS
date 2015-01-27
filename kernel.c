/*
*  kernel.c
*/

unsigned int col = 0;		//col & row corresponds to the monitor
unsigned int row = 0;
char *vidptr = (char*)0xb8000; 	//video mem begins here.
unsigned int cursor = 0;	//var where cursor is remembered
unsigned int MAX_COL = 80;
unsigned int MAX_ROW = 25;

static inline void outb(unsigned short port, unsigned char val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* TODO: Is it wrong to use 'N' for the port? It's not a 8-bit constant. */
    /* TODO: Should %1 be %w1? */
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

void printstr(char *str) {
	unsigned int j = 0;			//var used in traversing char* str;	

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
	int numDigit = 0;
	char* numstr;
	unsigned int ctr = 0;
	 
	if(num < 0)
		printstr("-");

	numTemp = absoluteVal(num);
	
	if(numTemp == 0) {
		numDigit = 1;
		numstr[0] = '0';
	} else {
		while(numTemp > 0) {
			numTemp /= 10;
			numDigit++;
		} numTemp = absoluteVal(num);
	}

	numstr[numDigit] = '\0';
	ctr = numDigit - 1;

	while(numTemp > 0 && ctr >= 0) {
		numstr[ctr] = ((numTemp % 10) + ASCII_VALUE);
		ctr--;
	
		numTemp /= 10;
	}	
	
	printstr(numstr);	
}

void printHex(char* hex) {
	unsigned int j = 0;			//var used in traversing char* str;	

	while(hex[j] != '\0') {
		if(hex[j] != '\n') {
			vidptr[cursor] = hex[j];
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

void scroll() {
	int i;
	
	i = MAX_COL * MAX_ROW * 2;
	while(i >= 0) {
		vidptr[i] = vidptr[i - 160];	
		i--;
	}

	for(i = 1; i < 160; i += 2) 
		vidptr[i] = 0x07;
	row = 24;
	moveCursor();
	
}

void kmain(void)
{	
	int i;
	clrscr();
	printstr("\n");	
	printstr("             ======================================================\n");
	printstr("                     /^--^\\     /^--^\\     /^--^\\     /^--^\\\n");
	printstr("                    |      |   |      |   |      |   |      |\n");
	printstr("                     \\____/     \\____/     \\____/     \\____/\n");
	printstr("                    /      \\   /      \\   /      \\   /      \\\n");
	printstr("                   |        | |        | |        | |        |\n");
	printstr("                    \\__  __/   \\__  __/   \\__  __/   \\__  __/\n");
	printstr("             |^|^|^|^|/ /^|^|^|^|^\\ \\^|^|^|^/ /^|^|^|^|^\\ \\^|^|^|^|\n");
	printstr("             | | | | / /  | | | | |\\ \\| | |/ /| | | | | |\\ \\ | | ||\n");
	printstr("             | | | | / /  | | | | |\\ \\| | |/ /| | | | | |\\ \\ | | ||\n");
	printstr("             ########\\ \\###########/ /#####\\ \\###########/ /#######\n");
	printstr("             | | | | |\\/| | |  | | \\/| | | |\\/| | | | | |\\/ | | | |\n");
	printstr("             |_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_||\n");
	printstr("             ======================================================\n");
	printstr("                       _____           _      ____   _____ \n");
	printstr("                      / ____|         | |    / __ \\ / ____|\n");
	printstr("                     | |  __ _ __ __ _| |__ | |  | | (___  \n");
	printstr("                     | | |_ | '__/ _` | '_ \\| |  | |\\___ \\ \n");
	printstr("                     | |__| | | | (_| | |_) | |__| |____) |\n");
	printstr("                      \\_____|_|  \\__,_|_.__/ \\____/|_____/ \n");
	sleep(10000);
	clrscr();

	printstr("C:\\");
	return;
}