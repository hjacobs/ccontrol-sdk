#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <fcntl.h>   /* File control definitions */

int dos2unix (char *, char *);


int dos2unix(char *src_filename, char *dest_filename)
{
  FILE *in;
  FILE *out;
  int c;

  if ((in = fopen (src_filename, "r")) == NULL)
    return -1;

  if ((out = fopen (dest_filename, "w")) == NULL)
    {
      fclose (in);
      return -1;
    }

  while ((c = fgetc (in)) != EOF)
    {
        if (c != '\r')
    	{
            fputc (c, out);
	}
    }

  fclose (in);
  fclose (out);

  return 0;
}


int main(int argc, char **argv)
{
    int mainfd = 0;
    
    if(argc != 3)
    {
	printf("Usage: %s <in file> <out file>\n", argv[0]);
	return 1;
    }
    
    if(dos2unix(argv[1], argv[2]) < 0)
    {
	printf("dos2unix conversion failed, probably failed opening %s or creating temp.dat\n", argv[1]);
	return 1;
    
    }
    return 0;
}