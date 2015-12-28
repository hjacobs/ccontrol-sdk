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
int getprog(int);

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
    
    tcgetattr(fd, &options);
    printf("termios: %li %ld %ld %ld %ld\n", options.c_iflag, options.c_oflag, options.c_cflag, options.c_lflag, options.c_line);
    
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
    printf("turning on\n");

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

int getprog(int fd)
{
    char chout = 'x';
    char *sendc;
    char *recvc;
    
    sendc = (char *) malloc (255);
    recvc = (char *) malloc (255);
    
    recvc[0] = '\0';

    sendc[0] = 3;  //dump program
    write(fd, sendc, 1); 
    printf("dumping...\n");

    while (1) 
    {
       read(fd, &chout, sizeof(chout));

       recvc[0] = chout;
       printf("%d ", recvc[0]);
       chout=0;
       
       if(recvc[0] == -1)
           break;
       
       usleep(20000);
     }
    printf("\n");
    return 0;    
}


int main(int argc, char **argv)
{
    int mainfd = 0;
    int opt;
    char *filename;
    char *devname;
    
    if(argc < 2)
    {
    	printf("Usage: %s -f <file.dat> -d <tty device>\n", argv[0]);
	return 1;
    }
	
        
    while(1)
    {        
        opt = getopt(argc, argv, "d:");

	if(opt == -1)
	    break;
	
        switch(opt)
	{
	    case 'd':
	    	devname = optarg;
	    break;  	  

	    default:
		printf("Usage: %s -f <file.dat> -d <tty device>\n", argv[0]);
	        exit(1);
	    break;
        }
    }
    
    mainfd = open_port(devname);
    
    if(mainfd < 0)
    {
    	printf("failed opening %s\n", devname);
	return 1;
    }
    printf("opened device successfully\n");
    
    if(initmc(mainfd) < 0)
    {
	printf("failed mc init\n");
	return 1;
    }
    printf("intialzied micro controller successfully\n");
    
    if(getprog(mainfd) == 1)
    {
	printf("faied getting program\n");
	return 1;
    }     
    printf("got program successfully\n");


    close(mainfd);
}
