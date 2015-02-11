//strcpy
void strcpy(char *dest, char *source)
{
   while(*source)
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
 
   while(*(s+c))
      c++;
 
   return c;
}

//strcmp
int strcmp(char s1[], char s2[])
{
	int index = 0;
	
	if(s1 == '\0')
	{
		if(s2 ==  '\0')
			return 0;
		else
			return -1;
	}

	else
	{
		if(s2 ==  '\0')
			return 1;

		else
		{	
			while(s1[index] == s2[index])
				index++;
				
			if(s1[index] > s2[index])
				return 1;

			else if(s1[index] < s2[index])
				return -1;			
		}	
		
	}
}

//strcat
void strcat(char *original, char *add)
{
   while(*original)
      original++;
 
   while(*add)
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
