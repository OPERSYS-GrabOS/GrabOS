/*
*  kernel.c
*/

unsigned int col = 0;		//col & row corresponds to the monitor
unsigned int row = 0;
char *vidptr = (char*)0xb8000; 	//video mem begins here.
unsigned int cursor = 0;	//var where cursor is remembered

/*Moves the cursor using data from col and row*/
void moveCursor() {
	if(col > 80)
		col = 0;

	cursor = ((row * 80) + col) * 2;
}

int absoluteVal(int num) {
	if(num >= 0)
		return num;
	else return -num;
}

void clrscr() {
	unsigned int j = 0;
	while(j < 80 * 25 * 2) {
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

void kmain(void)
{	
	unsigned int i = 0;
	unsigned int j = 0;
	//clear all
	clrscr();
	
	printInt(0);
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
	
	return;
}
