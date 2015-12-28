#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

/*
* 'open_port()' - Open serial port 1.
*
* Returns the file descriptor on success or -1 on error.
*/

int open_port(char *);
int initmc(int);
int sendprog(char *, int);
int dos2unix (char *, char *);

int open_port(char *device)
{
    int fd; 
    struct termios *current;
    struct termios options;
    
//    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd == -1)
	return -1;
    
    bzero(&options, sizeof(options));
    
//    fcntl(fd, F_SETFL, FNDELAY);  

    tcgetattr(fd, &options);
//    printf("termios: %li %ld %ld %ld %ld\n", options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag, options.c_line);
    
    options.c_cflag = B9600 | CRTSCTS | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &options);

    return (fd);
}

int initmc(int fd)
{
    char chout = 'x';
    char *sendc;
    char *recvc;
    int i = 0;
    
    sendc = (char *) malloc (255);
    recvc = (char *) malloc (255);
    
    recvc[0] = '\0';

    sendc[0] = 1;  //Turn On
    write(fd, sendc, 1); 
    printf("sent ascii 1 to mc (init)\n");

    while (1) //get version string
    {
       read(fd, &chout, sizeof(chout));

       recvc[i] = chout;
//       printf("%d %c\n", recvc[i], chout);
       i++;
       recvc[i] = '\0';
       
       if(chout == 13)
           break;
       chout=0;
       usleep(20000);
     }
    
    printf("%s\n", recvc);
    
    if(strcmp(recvc, "CCTRL-BASIC Version 1.1 (20.12.96)\r") == 0)
	return 0;
    else 
        return -1;
}

int sendprog(char *filen, int sfd)
{
    int fd, i = 0;
    short int length;
    char ch;
    char *buf;
    
    buf = (char *) malloc(255);
    
    fd = open(filen, O_RDONLY);  //open file

    if (fd == -1)
    {
	fprintf(stderr, "Unable to open %s\n",filen);
	return -1;
    }
    
    while(ch != '\n')  //check if it is a CCTRL-BASIC file
    {
	read(fd, &ch, sizeof(char));
        buf[i] = ch;
        i++;
    }

    buf[i] = '\0';
    
    if(strcmp(buf, "CCTRL-BASIC\n") != 0)
    {
    	printf("Wrong file format.\n");    
	return -1;
    }
    else
	printf("dat file header is correct\n");
    
    
    i = 0;
    ch = 0;
    buf[i] = '\0';    
    
    //if it is, then send program length
    while(ch != '\n')  //read the program length
    {
	read(fd, &ch, sizeof(char));
        buf[i] = ch;
	i++;
        buf[i] = '\0';    

    }
    
    length = atoi(buf);
    
    //prepare mc for eprom programming
    buf[0] = 2;
    write(sfd, buf, 1);
    printf("sent ascii 2 (prepare for writing program)\n");
    
    //send prog length    
    printf("program length: %i\n", length);
    length = htons(length);
    write(sfd, &length, sizeof(short int));
        
    buf[0] = '\0';
    i = 0;
    ch = 'x';

    //send program
    while(read(fd, &ch, sizeof(char)) > 0)
    {
	
        if(ch == 32)
        {
	    ch = atoi(buf);
//            printf("%i\n", ch);
	    
	    write(sfd, &ch, sizeof(char));
	    
	    ch = 0;
	    usleep(20000);
	    
	    i = 0;
	    buf[i] = '\0';
        }

        else
        {	
		buf[i] = ch;
		ch = 0;
		i++;
		buf[i] = '\0';
        }
	
//        printf("loop\n", ch);

    }
    
    return 0;

}

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
    int opt;
    char *filename;
    char *devname;
    
    if(argc < 3)
    {
    	printf("Usage: %s -f <file.dat> -d <tty device>\n", argv[0]);
	return 1;
    }

    while(1)
    {        
        opt = getopt(argc, argv, "d:f:");

	if(opt == -1)
	    break;
	
        switch(opt)
	{
	    case 'd':
	    	devname = optarg;
	    break;  	  

	    case 'f':
	    	filename = optarg;
	    break;    

	    default:
		printf("Usage: %s -f <file.dat> -d <tty device>\n", argv[0]);
	        exit(1);
	    break;
        }
    }
    
    if(strlen(filename) < 1 || strlen(devname) < 1)
    {
        printf("Usage: %s -f <file.dat> -d <tty device>\n", argv[0]);
        exit(1);
    }
    
    if(dos2unix(filename, "123123123temp.dat") < 0)
    {
	printf("dos2unix conversion failed, probably failed opening %s or creating 123123123temp.dat\n", filename);
	return 1;
    }
    
    mainfd = open_port(devname);
    
    if(mainfd < 0)
    {
    	printf("failed opening %s\n", devname);
	return 1;
    }
    else
        printf("opened device successfully\n");
    
    if(initmc(mainfd) < 0)
    {
	printf("failed mc init\n");
	return 1;
    }
    else
        printf("intialzied micro controller successfully\n");
    
    if(sendprog("123123123temp.dat", mainfd) < 0)
    {
	printf("failed programming\n");
	return 1;
    }     
    else
        printf("programmed eeprom successfully\n");

    unlink("123123123temp.dat");
    close(mainfd);
    return 0;
}
