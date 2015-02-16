//strcpy
void strcpy(char *dest, char *source)
{
   while(*source != '\0')
   {
      *dest = *source;
      source++;
      dest++;
   }
   *dest = '\0';
}

// strlen
int strlen(char *s)
{
   int c = 0;
 
   while(*(s+c) != '\0')
      c++;
 
   return c;
}

//strcmp
int strcmp(char *first, char *second)
{
   while(*first==*second)
   {
      if ( *first == '\0' || *second == '\0' )
         break;
 
      first++;
      second++;
   }
   if( *first == '\0' && *second == '\0' )
      return 0;
   else
      return -1;
}

//strcat
void strcat(char *original, char *add)
{
   while(*original != '\0')
      original++;
 
   while(*add != '\0')
   {
      *original = *add;
      add++;
      original++;
   }
   *original = '\0';
}

//strrev
int main()
{
   char s[100], r[100];
   int length, new_length, index;
 
   printf("Input a string\n");
   gets(s);
 
   length = strlen(s);
 
   for (new_length = length - 1, d = 0; new_length >= 0; new_length--, d++)
      r[d] = s[new_length];
 
   r[d] = '\0';
 
   printf("%s\n", r);
 
   return 0;
}
