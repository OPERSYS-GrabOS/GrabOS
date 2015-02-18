void display_marquee(char* strMarquee){
     
     do{
        clearLine();
        moveColumn();
        printstr(strMarquee);    
     }while(1);
}

void timer(int delay) {
	int i;	
	for(i = 0; i < delay * 100000; i++);
}

void moveColumn(){
    unsigned int move = column;
     
     if(move < MAX_COL){
        column++;     
     }
     else{
       column--;
     }
}

void clearLine(){
     unsigned int currIn = 0;
     /*Traverse the Current Row and of the current column then delete all contents 
     of all columns before and including the current column*/
     while(currIn <= column){
        vidptr[currIn] = ' ';        
        vidptr[currIn+1] = 0x07; 		
		currIn++;
     }
  /*Not Sure how to call Sleep Function Handler which is in Assembly*/
}

initialize the IDT
    unsigned long address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

   /*Not Sure how to map Software Interrupts in the IDT Table 
   but interrupt numbers 48-254 are reserved for Software Interrupts*/


   /*Not Sure how to incorporate these(Adding the Marquee Interrupt in the IDT).*/
    address = (unsigned long)marquee_handler; 
    IDT[0x48].offset_lowerbits = address & 0xffff;
	IDT[0x48].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[0x48].zero = 0;
	IDT[0x48].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[0x48].offset_higherbits = (address & 0xffff0000) >> 16;
	
	/*Sleep/Timer Interrupt definition in the IDT Table*/
	 address = (unsigned long)sleep_handler; 
    IDT[0x52].offset_lowerbits = address & 0xffff;
	IDT[0x52].selector = 0x08; /* KERNEL_CODE_SEGMENT_OFFSET */
	IDT[0x52].zero = 0;
	IDT[0x52].type_attr = 0x8e; /* INTERRUPT_GATE */
	IDT[0x52].offset_higherbits = (address & 0xffff0000) >> 16;
	
	
	
Assembly Code
marquee_handler:                 
	call    display_marquee
	iretd
	
sleep_handler :
    call    sleep[1000]
	iretd
