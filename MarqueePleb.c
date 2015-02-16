void marquee(char *wordToMove){
     int move = end = start = 0;
     while(end == 0 || start == 0){
       if(move!= MAX_COL && end == 0){
        column = move;
        printstr(wordToMove);        
        timer(100);
        move++;
        if(move == MAX_COL)
          end = 1;       
       }
       else if(end == 1 && start == 0){
          column = move;
          printstr(wordToMove);        
          timer(100);
          move--;
          if(move == 0)
            start = 1;
       }
     }
}
