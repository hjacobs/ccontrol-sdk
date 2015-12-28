/*	asXX.c
*
*	68HC05/11 assembler, from Motorola Freeware assembler
*	(can easily be expanded for other Motorola microcontrollers)
*
*	Compile with #define M68HC05 or #define M68HC11
*
*	Version for Linux
*
*	Didier Juges, 20 Dec 1994, Oct 1999
*
*	Version 2.11 includes following changes from Version 2.09:
*		- input files have default extension ".s" if def UNIX,
*				extension ".asm" otherwise
*		- command line option -erl added for error listing file which
*			includes the source file line which caused the error.
*			Error listing file has filename of the first input file
*			and extension .err
*	RELEASE TER_2.1 has bug fix (';' was returning 0, instead of the
*		ascii value of ; (line 628 )
*	Version 2.12 has option -lo for output listing file and gnu type 
*		command line format:
*		as11 [-options] [filenames]
*		and options are lower case in UNIX and all options start with '-'
*	Version 2.13 fixed a problem correctly naming .s19 file when there are 
*		dots in directory path, also changed char buf[80] to char buf[256]
*	Version 2.14 has bug fixes regarding the -erl flag
*----------------------------------------------------------------------*/
#define M68HC05
//#define DEBUG

#define RELEASE "TER_2.1"	/* May 1999 */
#define VERSION "2.14"	/* Oct 5, 1999 */
#define REL_DATE	"10/5/1999"
#define AUTHOR	"Didier Juges"
#ifdef M68HC05
#define DEVICE "68HC05"
#endif
#ifdef M68HC11
#define DEVICE "68HC11"
#endif

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "as.h"
#ifdef M68HC05
#include "table5.h"
#endif
#ifdef M68HC11
#include "table11.h"
#endif


void localinit( void );
int bitop( int op, int mode, int class );
void epage( int p );
void do_op( int opcode, int class );
#ifdef M68HC11
void do_gen( int op, int mode, int pnorm, int px, int py );
#endif
#ifdef M68HC05
void do_gen( int op, int mode );
#endif
void do_indexed( int op );
void initialize_1( void );
void initialize_2( char * );
void re_init( void );
void make_pass( void );
int parse_line( void );
void process( void );
char *FGETS( char *s, int n, register FILE *iop );
void do_pseudo( int op );
void PC_Exchange( int PC_ptr_new );
void IfMachine( int Token );
int GetToken( void );
int eval_ifd( void );
int eval_ifd( void );
int eval_ifnd( void );
int eval( void );
int is_op( char c );
int get_term( void );
int install( char *str, int val );
struct nlist *lookup( char *name);
struct oper *mne_look( char *str );
void fatal( char *str );
void error( char *str );
void warn( char *str );
int delim( char c );
char *skip_white( char *ptr );
void eword( int wd );
void emit( int byte );
void f_record( void );
void hexout( int byte );
void print_line( void );
int any( char c, char *str );
char mapdn( char c );
int lobyte( int i );
int hibyte( int i );
int head( char *str1, char *str2 );
int alpha( char c );
int alphan( char c );
int white( char c );
char *alloc( int nbytes );
int FNameGet( char *NameString );
char *strsave( char *s );
void pouterror( void );
void NewPage( void );
char *LastChar( char *strpt );
void fwdinit( void );
void fwdreinit( void );
void fwdmark( void );
void fwdnext( void );
void stable( struct nlist *ptr );
void cross( struct nlist *point );
void open_error_file( char * );
void open_listing_file( char * );

FILE *out;//stdout
char ListingFile[BUFSIZ];

/*---------------------------------------------------------------------*
* as --- cross assembler main program
*
*	Note: Compile with define DEBUG for function module debug 
*	statements.  Compile with define DEBUG2 for version 2 debug
*	statements only.  Compile with define IBM to use original, non-
*	Amiga, non-MAC, non-UNIX fgets() function.  Amiga version will
*	accept IBM generated source code but not the other way around.
*	Note added version (TER) 2.01 19 June 1989.
*----------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
	char **np, fbuf[BUFSIZ];
	char *i, *charp;
	char InputFile[BUFSIZ];
	int j = 0;
	void do_op();
	void f_record(); /* added for ANSI C compat ver TER_2.0 6/18/89 */

	out = fopen("temp", "rw");
	 
	printf( "\n%s Assembler release %s version %s, ", DEVICE, RELEASE, VERSION );
	printf( "%s %s\n", REL_DATE, AUTHOR );
	printf( "(c) Motorola (freeware)\n" );
	printf( "Major bugfixes by Diman Todorov.\n" );

	if( argc < 2 ){
		if( (charp = strrchr( argv[0], '/' )) == NULL )
			if( (charp = strrchr( argv[0], '\\' )) == NULL )
				charp = argv[0]-1;
		charp++;
		strcpy( fbuf, charp );
		if( (charp = strrchr( fbuf, '.' )) != NULL )
			*charp = '\0';
		printf( "\nUsage: %s [-options] [files]\n", fbuf );
		printf( "Options (options are FALSE by default):\n" );
		printf( "   -l      print listing (default to standard output)\n" );
		printf( "   -lo     redirect listing to output file (.lst)\n" );
		printf( "   -c      print cycle count\n" );
		printf( "   -s      generate symbol table flag\n" );
		printf( "   -erl    generate error message file\n" );
		printf( "   -cre    generate cross reference table flag\n" );
		printf( "   -crlf   add <CR><LF> to end of S record file\n" );
		printf( "   -nnf    number INCLUDE files separately (default TRUE)\n" );
		printf( "   -p50    form feed every 50 lines in listing output\n" );
		exit( 1 );
	}
	Argv = argv;
	initialize_1();
	N_files = 0;

	for( j=1; j < argc; j++ ){
		if( *argv[j] != '-' ){
			if( N_files == 0 )	/* v 2.14 */ 
				Argv = &argv[j-1]; /* *(Argv+1) points to the first file name */
			N_files++;
			if( N_files == 1 )
				strcpy( InputFile, argv[j] );
		}else{
			argv[j]++; /* skip '-' */
			for( i = argv[j]; *i != 0; i++ )
				if( ( *i <= 'Z') && ( *i >= 'A'))
					*i = *i + 32; /* convert to lower case */
			if( strcmp( argv[j], "l" ) == 0 )
				Lflag = 1;
			else if( strcmp( argv[j], "erl" ) == 0 )
				ERLflag = 1;
			else if( strcmp( argv[j], "c" ) == 0 )
				Cflag = 1;
			else if( strcmp( argv[j], "s" ) == 0 )
				Sflag = 1;
			else if( strcmp( argv[j], "lo" ) == 0 )
				Oflag = 1;
			else if( strcmp( argv[j], "cre" ) == 0 )
				CREflag = 1;
			else if( strcmp( argv[j], "crlf" ) == 0 ) /* test for crlf option */
				CRflag = 1;			/* add <CR><LF> to S record */
									/* ver TER_1.1 June 3, 1989 */
			else if( strcmp( argv[j], "nnf" ) == 0 ) /* test for nnf option */
				nfFlag=0;				/* nfFlag=1 means number INCLUDE
										separately. ver TER_2.0 6/17/89 */
			else if( strcmp( argv[j], "p50" ) == 0 )	/* page every 50 lines */
				Pflag50 = 1;			/* ver (TER) 2.01 19 Jun 89 */
		}
	}

	initialize_2( InputFile );
	
	root = NULL;
	Cfn = 0;
	np = &argv[argc - N_files - 1];
	Line_num = 0; /* reset line number */

	while( ++Cfn <= N_files ){
		strcpy( fbuf, *++np );
		if( ERLflag )	/* new with v 2.14 */
			open_error_file( fbuf );

		if(( Fd = fopen( fbuf, "r" )) == NULL )
		{
			printf("Cannot open input file %s\n", fbuf );
			exit(1);
		}
		else
		{
			make_pass();
			fclose( Fd );
		}
		
		if( Err_count > 0 && ERLflag ){		/* new with v 2.14 */
			/* print total bytes */
			fprintf( EFp, "Program + Init Data = %d bytes\n", F_total );
			fprintf( EFp, "Error count = %d\n", Err_count );	/* rel TER_2.0 */
			fclose( EFp );
		}
	}
	
	if( Err_count == 0 ){
		Pass++;
		printf("errors 0, Pass %d\n", Pass);
		
		re_init(); /* open output file Objfil */
		Cfn = 0;
		np = &argv[argc - N_files - 1];
		Line_num = 0;
		FdCount = 0;		/* Resets INCLUDE file nesting ver TER_2.0 6/17/89 */
		while( ++Cfn <= N_files ){
			
			strcpy( fbuf, *++np );
			
			if( ERLflag )
				open_error_file( fbuf );
			
			if( Oflag )
			{
				open_listing_file( fbuf );
				out = OFp;
			}
			
			else
				out = stdout;
//Diman Begin
		printf("fbuf: %s\n", fbuf);			
//Diman End
			
			if(( Fd = fopen( fbuf, "r" )) != NULL ){
				make_pass();
				fclose( Fd );
			}
			else
			{
			    printf("couldnt open %s at line 297\n", fbuf);			
			}

			/* print total bytes: */
			sprintf( fbuf, "Program + Init Data = %d bytes\n", F_total );
			fprintf( out, fbuf );
			if( ERLflag )
				fputs( fbuf, EFp );

			/* print error count: */
			sprintf( fbuf, "Error count = %d\n", Err_count );
			fprintf( out, fbuf );
			if( ERLflag )
				fputs( fbuf, EFp );

			if( Sflag == 1 ){
				fprintf( out, "\f" );
				stable( root );
			}
			if( CREflag == 1 ){
				fprintf( out, "\f" );
				cross( root );
			}
			if( CRflag == 1 )
				fprintf( Objfil, "S9030000FC%c\n", CR ); /* ver TER_1.1 print w <CR> */
			else
				fprintf( Objfil, "S9030000FC\n" ); /* at least give a descent ending */

			if( ERLflag )
				fclose( EFp );
			if( Oflag )
				fclose( OFp );
			else
				out = stdout;
		}
		fclose( Objfil );	/* close file correctly ver TER_1.1 */
	}
	
	else
	{
		/* just note errors, TER_2.0 */
		fprintf( out, "Program + Init Data = %d bytes\n", F_total );	/* print total bytes */
		fprintf( out, "Error count = %d\n", Err_count );	/* rel TER_2.0 */
	}
	exit( Err_count );	/* neat for UNIX cuz can test return value in script
						but on other systems like Amiga, mostly just makes
						further script statements fail if >10, else
						nothing.  Therefore, printed out byte count
						and error level. ver (TER) 2.02 19 Jun 89 */
    return 0;
} /* end main() */

/*---------------------------------------------------------------------*
*	open_error_file()
*----------------------------------------------------------------------*/
void open_error_file( char *pfn ){

	char *charp;

	strcpy( ErrorFile, pfn );
	if( (charp = strrchr( ErrorFile, '.' )) == NULL )
		strcat( ErrorFile, ".err" );
	else
		strcpy( charp, ".err" );
	if(( EFp = fopen( ErrorFile, "wt" )) == NULL ){
		fprintf( stderr, "\nCannot open error file %s\n", ErrorFile );
		ERLflag = 0;
	}
} /* end open_error_file() */

/*---------------------------------------------------------------------*
*	open_listing_file()
*----------------------------------------------------------------------*/
void open_listing_file( char *pfn ){

	char *charp;

	strcpy( ListingFile, pfn );
	if( (charp = strrchr( ListingFile, '.' )) == NULL )
		strcat( ListingFile, ".lst" );
	else
		strcpy( charp, ".lst" );
	if( (OFp = fopen( ListingFile, "wt" )) == NULL ){
		fprintf( stderr, "\nCannot open listing file %s\n", ListingFile );
		Oflag = 0;
	}
} /* end open_listing_file() */

#ifdef M68HC05
/*---------------------------------------------------------------------*
*	MC6805 specific processing
*----------------------------------------------------------------------*/

/* addressing modes */
#define IMMED   0       /* immediate */
#define IND     1       /* indexed */
#define OTHER   2       /* NOTA */

/*---------------------------------------------------------------------
	localinit --- machine specific initialization
----------------------------------------------------------------------*/
void localinit( void ){

}

/*---------------------------------------------------------------------
 *      do_op --- process mnemonic
 *
 * Called with the base opcode and it's class. Optr points to
 * the beginning of the operand field.

 	Arguments: 	opcode - base opcode
				class - mnemonic class
----------------------------------------------------------------------*/
void do_op( int opcode, int class ){

	int dist;   /* relative branch distance */
	int amode;  /* indicated addressing mode */
	char *peek;

	/* guess at addressing mode */
	peek = Optr;
	amode = OTHER;
	while( !delim( *peek ) && *peek != EOS )  /* check for comma in operand field */
		if( *peek++ == ',' ){
			amode = IND;
			break;
		}
	if( *Optr == '#' )
		amode = IMMED;

	switch( class ){
		case INH:                       /* inherent addressing */
			emit( opcode );
			return;
		case GEN:                       /* general addressing */
			do_gen( opcode, amode );
			return;
		case REL:                       /* short relative branches */
			eval();
			dist = Result - (  Pc+2 );
			emit( opcode );
			if( (  dist >127 || dist <-128) && Pass==2 ){
				error( "Branch out of Range" );
				emit( lobyte( -2 ) );
				return;
			}
			emit( lobyte( dist ));
			return;
		case NOIMM:
			if( amode == IMMED ){
				error( "Immediate Addressing Illegal" );
				return;
			}
			do_gen( opcode, amode );
			return;
		case GRP2:
			if( amode == IND ){
				do_indexed( opcode+0x20 );
				return;
			}
			eval();
			Cycles += 2;
			if( Force_byte ){
				emit( opcode );
				emit( lobyte( Result ));
				return;
			}
			if( Result>=0 && Result <=0xFF ){
				emit( opcode );
				emit( lobyte( Result ));
				return;
			}
			error( "Extended Addressing not allowed" );
			return;
		case SETCLR:
		case BTB:
			eval();
			if( Result <0 || Result >7 ){
				error( "Bit Number must be 0-7" );
				return;
			}
			emit( opcode | (Result << 1 ));
			if(*Optr++ != ',')
				error( "SYNTAX" );
			eval();
			emit( lobyte( Result ));
			if( class == SETCLR )
				return;
			/* else it's bit test and branch */
			if( *Optr++ != ',' )
				error( "SYNTAX" );
			eval();
			dist = Result - (  Old_pc+3 );
			if((  dist >127 || dist <-128) && Pass==2 ){
				error( "Branch out of Range" );
				emit(lobyte(-3));
				return;
			}
			emit( lobyte( dist ));
			return;
		default:
			fatal( "Error in Mnemonic table" );
	}
} /* end do_op() */

/*---------------------------------------------------------------------
	do_gen --- process general addressing
----------------------------------------------------------------------*/
void do_gen( int op, int mode ){

	if( mode == IMMED ){
		Optr++;
		emit( op );
		eval();
		emit( lobyte( Result ));
		return;
	}else
		if( mode == IND ){
			do_indexed( op+0x30 );
			return;
		}else
			if( mode == OTHER ){        /* direct or extended addressing */
				eval();
				if( Force_word ){
				emit( op+0x20 );
				eword( Result );
				Cycles += 3;
				return;
			}
		if( Force_byte ){
			emit( op+0x10 );
			emit( lobyte( Result ));
			Cycles += 2;
			return;
		}
		if(Result >= 0 && Result <= 0xFF){
			emit( op+0x10 );
			emit( lobyte( Result ));
			Cycles += 2;
			return;
		}else{
			emit( op+0x20 );
			eword( Result );
			Cycles += 3;
			return;
		}
	}else{
  		error( "Unknown Addressing Mode" );
  		return;
	}
} /* end do_gen() */

/*---------------------------------------------------------------------
	do_indexed --- handle all wierd stuff for indexed addressing
----------------------------------------------------------------------*/
void do_indexed( int op ){

	eval();
	if( !( *Optr++ == ',' && ( *Optr == 'x' || *Optr == 'X' )))
		warn( "Indexed Addressing Assumed" );
	if( Force_word ){
		if( op < 0x80 ){ /* group 2, no extended addressing */
			emit( op+0x10 ); /* default to one byte indexed */
			emit( lobyte( Result ));
			Cycles += 3;
			return;
		}
		emit( op );
		eword( Result );
		Cycles += 4;
		return;
	}
	Cycles += 3;    /* assume 1 byte indexing */
	if( Force_byte ){
		emit( op + 0x10 );
		emit( lobyte( Result ));
		return;
	}
	if(Result==0){
		emit(op+0x20);
		Cycles--;       /* ,x slightly faster */
		return;
	}
	if( Result > 0 && Result <= 0xFF ){
		emit( op + 0x10 );
		emit( lobyte( Result ));
		return;
	}
	if( op < 0x80 ){
		warn( "Value Truncated" );
		emit( op + 0x10 );
		emit( lobyte( Result ));
		return;
	}
	emit( op );
	eword( Result );
	Cycles++;       /* 2 byte slightly slower */
	return;
} /* end do_indexed() */
#endif


#ifdef M68HC11
/*---------------------------------------------------------------------*
*	MC6811 specific processing
*----------------------------------------------------------------------*/

#define PAGE1   0x00
#define PAGE2   0x18
#define PAGE3   0x1A
#define PAGE4   0xCD

/* addressing modes */
#define IMMED   0
#define INDX    1
#define INDY    2
#define LIMMED  3       /* long immediate */
#define OTHER   4

int     yflag = 0;      /* YNOIMM, YLIMM, and CPD flag */

void localinit( void ){

}

/*---------------------------------------------------------------------*
*      do_op --- process mnemonic
*
* Called with the base opcode and it's class. Optr points to
* the beginning of the operand field.
*
*	opcode: base opcode
*	class: mnemonic class
*----------------------------------------------------------------------*/
void do_op( int opcode, int class ){

	int     dist;   /* relative branch distance */
	int     amode;  /* indicated addressing mode */
	char *peek;

	/* guess at addressing mode */
	peek = Optr;
	amode = OTHER;
	while( !delim( *peek ) && *peek != EOS )  
		/* check for comma in operand field */
		if( *peek++ == ',' ){
			if( mapdn( *peek ) == 'y' )
				amode = INDY;
			else
				amode = INDX;
			break;
		}
	if( *Optr == '#' )
		amode = IMMED;
	yflag = 0;
	switch( class ){
		case P2INH:
			emit( PAGE2 );
		case INH:                       /* inherent addressing */
			emit( opcode );
			return;
		case REL:                       /* relative branches */
			eval();
			dist = Result - (Pc+2);
			emit( opcode );
			if( ( dist > 127 || dist < -128 ) && Pass == 2 ){
				error( "Branch out of Range" );
				emit( lobyte( -2 ) );
				return;
			}
			emit( lobyte( dist ));
			return;
		case LONGIMM:
			if( amode == IMMED )
				amode = LIMMED;
		case NOIMM:
			if( amode == IMMED ){
				error( "Immediate Addressing Illegal" );
				return;
			}
		case GEN:                       /* general addressing */
			do_gen( opcode, amode, PAGE1, PAGE1, PAGE2 );
			return;
		case GRP2:
			if( amode == INDY ){
				Cycles++;
				emit( PAGE2 );
				amode = INDX;
			}
			if( amode == INDX )
				do_indexed( opcode );
			else{   /* extended addressing */
				eval();
				emit( opcode + 0x10 );
				eword( Result );
			}
			return;
		case CPD:               /* cmpd */
			if( amode == IMMED )
				amode = LIMMED;
			if( amode == INDY )
				yflag=1;
			do_gen( opcode, amode, PAGE3, PAGE3, PAGE4 );
			return;
		case XNOIMM:            /* stx */
			if( amode == IMMED ){
				error( "Immediate Addressing Illegal" );
				return;
			}
		case XLIMM:             /* cpx, ldx */
			if( amode == IMMED )
				amode = LIMMED;
			do_gen( opcode, amode, PAGE1, PAGE1, PAGE4 );
			return;
		case YNOIMM:            /* sty */
			if( amode == IMMED ){
				error( "Immediate Addressing Illegal" );
				return;
			}
		case YLIMM:             /* cpy, ldy */
			if( amode == INDY )
				yflag = 1;
			if( amode == IMMED )
				amode = LIMMED;
			do_gen( opcode, amode, PAGE2, PAGE3, PAGE2 );
			return;
		case BTB:               /* bset, bclr */
		case SETCLR:            /* brset, brclr */
			opcode = bitop( opcode, amode, class );
			if( amode == INDX )
				Cycles++;
			if( amode == INDY ){
				Cycles+=2;
				emit( PAGE2 );
				amode = INDX;
			}
			emit( opcode );
			eval();
			emit( lobyte( Result ));   /* address */
			if( amode == INDX )
				Optr += 2;      /* skip ,x or ,y */
			Optr = skip_white( Optr );
			eval();
			emit( lobyte( Result ));   /* mask */
			if( class == SETCLR )
				return;
			Optr = skip_white( Optr );
			eval();
			dist = Result - (Pc+1);
			if( ( dist > 127 || dist < -128 ) && Pass == 2 ){
				error( "Branch out of Range" );
				dist = Old_pc - (Pc+1);
			}
			emit( lobyte( dist ));
			return;
		default:
			fatal( "Error in Mnemonic table" );
	}
} /* end do_op() */

/*---------------------------------------------------------------------*
*      bitop --- adjust opcode on bit manipulation instructions
*----------------------------------------------------------------------*/
int bitop( int op, int mode, int class ){

	if( mode == INDX || mode == INDY )
		return( op );
	if( class == SETCLR )
		return( op-8 );
	else if( class == BTB )
		return( op-12 );
	else
		fatal( "bitop" );
	return( 0 );	/* to keep gcc happy */

} /* end bitop() */

/*---------------------------------------------------------------------*
*      do_gen --- process general addressing modes
*	op - base opcode
*	mode - addressing mode
*	pnorm - page for normal addressing modes: IMM,DIR,EXT
*	px - page for INDX addressing
*	py - page for INDY addressing
*----------------------------------------------------------------------*/
void do_gen( int op, int mode, int pnorm, int px, int py ){

	switch( mode ){
		case LIMMED:
			Optr++;
			epage( pnorm );
			emit( op );
			eval();
			eword( Result );
			break;
		case IMMED:
			Optr++;
			epage( pnorm );
			emit( op );
			eval();
			emit( lobyte( Result ));
			break;
		case INDY:
			if( yflag )
				Cycles += 2;
			else
				Cycles += 3;
			epage( py );
			do_indexed( op + 0x20 );
			break;
		case INDX:
			Cycles += 2;
			epage( px );
			do_indexed( op + 0x20 );
			break;
		case OTHER:
			eval();
			epage( pnorm );
			if( Force_word ){
				emit( op + 0x30 );
				eword( Result );
				Cycles += 2;
				break;
			}
			if( Force_byte ){
				emit( op + 0x10 );
				emit( lobyte( Result ));
				Cycles++;
				break;
			}
			if( Result >= 0 && Result <= 0xFF ){
				emit( op + 0x10 );
				emit( lobyte( Result ));
				Cycles++;
				break;
			}else{
				emit( op + 0x30 );
				eword( Result );
				Cycles += 2;
				break;
			}
			break;
		default:
			error( "Unknown Addressing Mode" );
	}
} /* end do_gen() */

/*---------------------------------------------------------------------*
*      do_indexed --- handle all wierd stuff for indexed addressing
*----------------------------------------------------------------------*/
void do_indexed( int op ){

	char c;

	emit( op );
	eval();
	if( *Optr++ != ',' )
		error( "Syntax" );
	c = mapdn( *Optr++ );
	if( c != 'x' && c != 'y')
		warn( "Indexed Addressing Assumed" );
	if( Result < 0 || Result > 255)
		warn( "Value Truncated" );
	emit( lobyte( Result ));

} /* end do_indexed() */

/*---------------------------------------------------------------------*
*      epage --- emit page prebyte
*----------------------------------------------------------------------*/
void epage( int p ){

	if( p != PAGE1 )        /* PAGE1 means no prebyte */
		emit( p );

} /* end epage() */
#endif


/*--------------------------------------------------------------------*
*	initialize_1()
*---------------------------------------------------------------------*/
void initialize_1( void ){

#ifdef DEBUG
	printf( "Initializing\n" );
#endif
	Err_count = 0;
	Pc	= 0;
	Pass = 1;
	Lflag	= 1;
	ERLflag = 0;
	Cflag = 0;
	Ctotal = 0;
	Sflag = 0;
	CREflag = 0;
	N_page = 0;
	Line[MAXBUF-1] = NEWLINE;

} /* end initialize_1() */

/*--------------------------------------------------------------------*
*	initialize_2()
*---------------------------------------------------------------------*/

void initialize_2( char *pFile )
{

	char *charp;
	
		
	strcpy( Obj_name, pFile ); /* copy first file name into array */
	
	if( (charp = strrchr(Obj_name, '.')) != NULL)
	{
	    charp[0] = '\0';
	}

	strcat( Obj_name, ".s19" );  /* append .s19 to file name. */

	fwdinit(); /* forward ref init */
	localinit(); /* target machine specific init. */
}

/*---------------------------------------------------------------------
*	re_init()
*---------------------------------------------------------------------*/
void re_init( void ){

	char buf[BUFSIZ];

//Diman Begin
	printf( "Reinitializing\n" );
	printf( "Obj_Name at line 923: %s\n", Obj_name );
//Diman End

	Pc = 0;
	E_total = 0;
	P_total = 0;
	Ctotal = 0;
	N_page = 0;
	fwdreinit();
	
	if(( Objfil = fopen( Obj_name, "w" )) == NULL){	/* v 2.14 */
		sprintf( buf, "Can't create object file %s", Obj_name );
		fatal( buf );
	}
		
}	/* end re_init() */

/*---------------------------------------------------------------------
	make_pass()
----------------------------------------------------------------------*/
void make_pass( void )
{

#ifdef DEBUG
	printf( "Pass %d\n", Pass );
#endif

	Line[0] = '\0';

	while( fgets( Line, MAXBUF-1, Fd ) != NULL )
	{  
		Line_num++;
		P_force = 0; /* No force unless bytes emitted */
		N_page = 0;
		if( parse_line() )
			process();
		if( Pass == 2 && Lflag && !N_page )
			print_line();
		P_total = 0; /* reset byte count */
		Cycles = 0; /* and per instruction cycle count */
		Line[0] = '\0';
	}
	f_record();

} /* end make_pass() */

/*---------------------------------------------------------------------
	parse_line --- split input line into label, op and operand
----------------------------------------------------------------------*/
int parse_line( void ){

	register char *ptrfrm = Line;
	register char *ptrto = Label;
	
	if( *ptrfrm == '\r' )
	{
	    printf("Error: This is a dos source file, convert it to unix first!\n");
	    exit(1);
	}
	
	if( *ptrfrm == '*' || *ptrfrm == '\n' || *ptrfrm == ';' )
		/* added check for ';' ver TER_1.1 */
		/* June 3, 1989 */
		return( 0 ); /* a comment line */

	while( delim( *ptrfrm ) == NO )	/* parsing Label */
		*ptrto++ = *ptrfrm++;
	if( *--ptrto != ':' )
		ptrto++;     /* allow trailing : */
	*ptrto = EOS;

	ptrfrm = skip_white( ptrfrm );
	if( *ptrfrm == ';' ){		/* intercept comment after label, ver TER_2.0 */
		if( *(ptrfrm-1) != '\'' && *(ptrfrm+1) != '\'' ){	/* ver TER_2.1 */
			*Op = *Operand = EOS;	/* comment, no Opcode or Operand */
			return( 1 );
		}
	}
	ptrto = Op;

	while( delim( *ptrfrm ) == NO )	/* parsing Opcode */
		*ptrto++ = mapdn( *ptrfrm++ );
	*ptrto = EOS;

	ptrfrm = skip_white( ptrfrm );
	if( *ptrfrm == ';' ){		/* intercept comment, ver TER_2.0 */
		if( *(ptrfrm-1) != '\'' && *(ptrfrm+1) != '\'' ){	/* ver TER_2.1 */
			*Operand = EOS;		/* comment, no Operand */
			return( 1 );
		}
	}

	ptrto = Operand;
	while( ( *ptrfrm != NEWLINE) && ( *ptrfrm != ';' || (*ptrfrm == ';' && *(ptrfrm-1) == '\'' && *(ptrfrm+1) == '\'') )) 
		/* ver TER_2.1 */
		/* ver TER_2.0 */
		*ptrto++ = *ptrfrm++;	/* kill comments */
	*ptrto = EOS;
/* Diman Begin
	printf( "Label\t\t%s-\n", Label );
	printf( "Op\t\t%s-\n", Op );
	printf( "Operand\t\t%s-\n", Operand ); Diman End*/
	return( 1 );

} /* end parse_line() */

/*--------------------------------------------------------------------*
*	process --- determine mnemonic class and act on it
*---------------------------------------------------------------------*/
void process( void ){

	struct oper *i;
	struct oper *mne_look();

	Old_pc = Pc;  /* setup `old' program counter */
	Optr = Operand;  /* point to beginning of operand field */

	if( *Op == EOS ){  /* no mnemonic */
		if( *Label != EOS )
			install( Label, Pc );
	}else if( (i = mne_look( Op )) == NULL ){
		sprintf( errbuf, "Unrecognized Mnemonic: %s", Op );
		error( errbuf );
	}else if( i->class == PSEUDO )
		do_pseudo( i->opcode );
	else{
		if( *Label )
			install( Label, Pc );
		if( Cflag )
			Cycles = i->cycles;
		do_op( i->opcode, i->class );
		if( Cflag )
			Ctotal += Cycles;
	}
} /* end process() */

/*------------------------------------------------------------------*
*	FGETS()
*
*  get at most n chars from iop
*  Added rev TER_1.1 June 3, 1989 
*  Adapted from Kernighan & Ritchie
*  An fgets() that is IBM proof. Not needed if
*  this IS an IBM
*-------------------------------------------------------------------*/
#ifndef IBM
char *FGETS( char *s, int n, register FILE *iop )
{
    register int c;
    register char *cs;

	cs = s;
	while( --n > 0 && ( c = getc( iop )) != EOF ){ /* read chars if not file end */
		if( ( *cs = c ) != CR )
			cs++; /* incr buffer pointer if not CR */
		/* If CR, leave to be written over */
		if( c == '\n')
			break;
	}
/*	if(c == EOF)
	    printf("c == EOF\n");
	if(strcmp(cs, s) == 0)
	    printf("cs == s\n");
*/
	*cs = '\0';	/* replace NEWLINE with NULL as in standard fgets() */
	return(( c == EOF && (strcmp(cs, s) != 0) ) ? NULL : s );  /* return NULL if this is EOF */

} /* end FGETS() */
#endif

/*--------------------------------------------------------------------*
*	FILE: pseudo --- pseudo op processing
*---------------------------------------------------------------------*/

#define RMB     0       /* Reserve Memory Bytes         */
#define FCB     1       /* Form Constant Bytes          */
#define FDB     2       /* Form Double Bytes (words)    */
#define FCC     3       /* Form Constant Characters     */
#define ORG     4       /* Origin                       */
#define EQU     5       /* Equate                       */
#define ZMB     6       /* Zero memory bytes            */
#define FILL    7       /* block fill constant bytes    */
#define OPT     8       /* assembler option             */
#define NULL_OP 9       /* null pseudo op               */
#define PAGE    10      /* new page                     */
#define INCLUDE 11		/* include <file> or "file" ver TER_2.0		*/
#define END		12		/* include <file> terminator ver TER_2.0	*/
#define IFD		13		/* if define <symbol> ver TER_2.0 */
#define IFND	14		/* if not define <symbol> ver TER_2.0 */
#define ELSE	15		/* else (for IF statements) ver TER_2.0 */
#define ENDIF	16		/* endif( for IF statements) ver TER_2.0 */
#define BSS		17		/* block storage segment (RAM) ver TER_2.09 */
#define CODE	18		/* code segment ver TER_2.09 25 Jul 89 */
#define DATA	19		/* data segment ver TER_2.09 25 Jul 89 */
#define AUTO	20		/* data segment ver TER_2.09 25 Jul 89 */

struct oper pseudo[] = {
"=",	PSEUDO,	EQU,	0,	/* ver TER_2.09 25 Jul 89 */
"auto",	PSEUDO,	AUTO,	0,	/* ver TER_2.09 25 Jul 89 */
"bss",	PSEUDO,	BSS,	0,	/* ver TER_2.09 25 Jul 89 */
"bsz",  PSEUDO, ZMB,    0,
"code",	PSEUDO, CODE,	0,	/* ver TER_2.09 25 Jul 89 */
"data", PSEUDO, DATA,	0,	/* ver TER_2.09 25 Jul 89 */
"else",	PSEUDO,	ELSE,	0,	/* ver TER_2.0 6/17/89 */
"end",	PSEUDO, END,	0,	/* ver TER_2.0 6/17/89 */
"endif",PSEUDO,	ENDIF,	0,	/* ver TER_2.0 6/17/89 */
"equ",  PSEUDO, EQU,    0,
"fcb",  PSEUDO, FCB,    0,
"fcc",  PSEUDO, FCC,    0,
"fdb",  PSEUDO, FDB,    0,
"fill", PSEUDO, FILL,   0,
"ifd",  PSEUDO, IFD,	0,	/* ver TER_2.0 6/17/89 */
"ifnd", PSEUDO, IFND,	0,	/* ver TER_2.0 6/17/89 */
"include", PSEUDO, INCLUDE, 0,	/* ver TER_2.0 6/17/89 */
"nam",  PSEUDO, NULL_OP,0,
"name", PSEUDO, NULL_OP,0,
"opt",  PSEUDO, OPT,    0,
"org",  PSEUDO, ORG,    0,
"pag",  PSEUDO, PAGE,   0,
"page", PSEUDO, PAGE,   0,
"ram",	PSEUDO, BSS,	0,	/* ver TER_2.09 25 Jul 89 */
"rmb",  PSEUDO, RMB,    0,
"spc",  PSEUDO, NULL_OP,0,
"ttl",  PSEUDO, NULL_OP,0,
"zmb",  PSEUDO, ZMB,    0 
};

/*--------------------------------------------------------------------*
*	do_pseudo --- do pseudo op processing
*---------------------------------------------------------------------*/
void do_pseudo( int op )
{

	char fccdelim, *strsave();
	int fill;
	int c;	/*test variable  ver TER_2.0 6/18/89 */
	char *savept;	/* savept is pointer to string save */
	FILE *FdTemp;	/* ver TER_2.0 6/17/89 */

	void NewPage();
	void IfMachine(); /* rel TER_2.0 6/18/89 */
	void PC_Exchange();	/* ver TER_2.09 25 Jul 89 */

	if( op != EQU && *Label )
		install( Label, Pc );

	P_force++;


	switch( op ){
		case RMB:                       /* reserve memory bytes */
			if( eval() )
			{
				Pc +=  Result;
				f_record();     /* flush out bytes */
			}
			else
				error( "Undefined Operand during Pass One" );
			break;
		case ZMB:                       /* zero memory bytes */
			if( eval() )
				while( Result-- )
					emit( 0 );
			else
				error( "Undefined Operand during Pass One" );
				break;
		case FILL:                      /* fill memory with constant */
			eval();
			fill = Result;
			if( *Optr++ != ',' )
				error( "Bad fill" );
			else{
				Optr = skip_white( Optr );
				eval();
				while( Result-- )
					emit( fill );
			}
			break;
		case FCB:                       /* form constant byte(s) */
			do{
				Optr = skip_white( Optr );
				eval();
				if( Result > 0xFF ){
					if( !Force_byte )
						warn( "Value truncated" );
						Result = lobyte( Result );
				}
				emit( Result );
			}while( *Optr++ == ',' );
			break;
		case FDB:                       /* form double byte(s) */
			do{
				Optr = skip_white( Optr );
				eval();
				eword( Result );
			}while( *Optr++ == ',' );
			break;
		case FCC:                       /* form constant characters */
			if( *Operand == EOS )
				break;
			fccdelim = *Optr++;
			while( *Optr != EOS && *Optr != fccdelim )
				emit( *Optr++ );
			if( *Optr == fccdelim )
				Optr++;
			else
				error( "Missing Delimiter" );
			break;
		case ORG:                       /* origin */
			if( eval() ){
				Old_pc = Pc = Result;
				f_record();     /* flush out any bytes */
			}else
				error( "Undefined Operand during Pass One" );
			break;
		case EQU:                       /* equate */
			if( *Label == EOS ){
				error( "EQU requires label" );
				break;
			}
			if( eval() ){
				install( Label, Result );
				Old_pc = Result;        /* override normal */
			}else
				error( "Undefined Operand during Pass One" );
			break;
		case OPT:                       /* assembler option */
			P_force = 0;
			if( head( Operand, "l" ) )
		    	Lflag = 1;
			else if( head( Operand, "nol" ))
				Lflag = 0;
			else if( head( Operand, "o" ))
				Oflag = 1;
			else if( head( Operand, "c" )){
				Cflag = 1;
				Ctotal = 0;
			}else if( head( Operand, "noc" ))
				Cflag = 0;
			else if( head( Operand, "contc" )){
				Cflag = 1;
			}else if(  head( Operand, "s" ))
				Sflag = 1;
			else if(  head( Operand, "cre" ))
				CREflag = 1;
			else if( head( Operand, "p50" ))	/* turn on page flag */
				Pflag50 = 1;	/* ver (TER) 2.02 19 Jun 89 */
			else if(  head( Operand, "crlf" )) /* add <CR> <LF> to */
				CRflag = 1;  /* S record ver TER_2.08 */
			else if( head( Operand, "nnf" ))	/* no extra line no. */
				nfFlag = 0;  /* w include files ver TER_2.08 */
			else
				error( "Unrecognized OPT " );
			break;
		case PAGE:                      /* go to a new page */
			P_force = 0;
			N_page = 1;
			if( Pass == 2 )
				if( Lflag )
					NewPage();
				break;
		case NULL_OP:                   /* ignored psuedo ops */
			P_force = 0;
			break;
		case INCLUDE:	/* case INCLUDE added ver TER_2.0 6/17/89 */
			P_force = 0;	/* no PC in printed output */
			if(( c = FNameGet( InclFName )) == 0 )
				error( "Improper INCLUDE statement" );
			else
			{
				if( FdCount > MAXINCFILES )
					error( "too many INCLUDE files" );
				else
				{
//Diman Begin
					printf("Include filename line 1301: %s\n", InclFName);
//Diman End
					if(( FdTemp = fopen( InclFName, "r" )) == 0 )
					{
						strcpy( cfbuf, Argv[Cfn] );
						fprintf( out, "%s, line no. ", cfbuf );	/* current file name */
						fprintf( out, "%d: ", Line_num ); /* current line number */
						Page_lines++;	/* increment lines per page */
						fprintf( out, "warning: can't open INCLUDE file %s\n", InclFName );
					}
					
					else
					{
						if(( savept = strsave( InclFName )) == 0 )
							error( "out of memory for INCLUDE file name" );
						else{
							InclFiles[FdCount].fp = Fd;	/* save current fp */
							if( nfFlag ){
								InclFiles[FdCount].line_num = Line_num;
								/* save current line count */
								Line_num = 0;	/* reset for new file */
							}

							/* VER 2.1 modification: */
							/* was: InclFiles[FdCount].name = Argv[Cfn]; */
							/* save pointer to current name */
							strcpy( cfbuf, Argv[Cfn] );
							InclFiles[FdCount].name = cfbuf;

							/* now replace pointer to current name with
							pointer to name of Include file */
							Argv[Cfn] = savept;
							
							Fd = FdTemp;	/* now replace current file with
											INCLUDE fp */
							FdCount++;	/* and increment "stack" */
						}
					}
				}
			}
			break;
		case END:
			P_force = 0;
			if( FdCount > 0 ){
				/* skip END statements in files received from CLI arguments */
				fclose( Fd );	/* close file from this level nest */
				FdCount--;	/* "pop stack" */
				Fd = InclFiles[FdCount].fp;	/* restore fp from nested stack */
				if( nfFlag )
					Line_num = InclFiles[FdCount].line_num;
				Argv[Cfn] = InclFiles[FdCount].name;
				/* restore file name pointer */
			}
			break;
		case IFD:
			P_force = 0;
			c = eval_ifd();
			IfMachine( c );
			break;
		case IFND:
			P_force = 0;
			c = eval_ifnd();
			IfMachine( c );
			break;
		case ELSE:
			P_force = 0;
			IfMachine( IF_ELSE );
			break;
		case ENDIF:
			P_force = 0;
			IfMachine( IF_ENDIF );
			break;
		case CODE:	/* CODE,DATA,BSS,AUTO ver TER_2.09 */
			PC_Exchange( 0 );
			break;
		case DATA:
			PC_Exchange( 1 );
			break;
		case BSS:
			PC_Exchange( 2 );
			break;
		case AUTO:
			PC_Exchange( 3 );
			break;
		default:
			fatal( "Pseudo error" );
			break;
	}
}

/*---------------------------------------------------------------------*
*	PC_Exchange --- Save current PC and recover requested one
*	added ver TER_2.09
*	request 0=CODE,1=DATA,2=BSS
*----------------------------------------------------------------------*/
void PC_Exchange( int PC_ptr_new ){

	P_force = 0;	/* no PC in output cuz wrong first time ( ?) */
	PC_Save[PC_ptr] = Pc;	/* save current PC */
	PC_ptr = PC_ptr_new;	/* remember which one we're using */
	Old_pc = Pc = PC_Save[PC_ptr];  /* recover the one requested */
	f_record();	/* flush out any bytes, this is really an ORG */

}

/*--------------------------------------------------------------------*
*	IfMachine() --- This function implements the IFD & IFND conditional
*	assembly machine.
*	version 1.0 made for release TER_2.0 cross assembler 27 Jun 89
*---------------------------------------------------------------------*/

#define FALSE_BLOCK	0	/* values for state variable */
#define TRUE_BLOCK	1
#define POP_TEST	2
#define ELSE_TEST	3
#define FALSE		0
#define TRUE		1

void IfMachine( int Token ){
		/* input token from calling machine */
		/* See file as.h for definition (globals) */

	static int State = TRUE_BLOCK, StackPt = 0, IfStack[MAXIFD];
	/* State variable, pointer to "IF stack pointer" and "Stack" */
	int Hiatus;	/* flag to break out of FSM & resume normal processing */

	Hiatus = FALSE; /* infinite loop to operate machine
					until time to break out */
	do{
		/* test at end */

#ifdef DEBUG3
		printf( "If Machine state=%u , token=%u\n", State, Token );
#endif

		if( State == TRUE_BLOCK )
			/* a block of statements processed normally */
			switch( Token ){
				case IF_TRUE:
					IfStack[StackPt] = TRUE;
					if( ++StackPt > MAXIFD ){
						/* check for over flow */
						StackPt = MAXIFD;
						error( "Error:IFD/IFND nested too deep" );
					}
					/* print_line() will be done in normal processing */
					Hiatus = TRUE;	/* resume normal line processing */
					break;
				case IF_FALSE:
					IfStack[StackPt] = TRUE;
					if( ++StackPt > MAXIFD ){
						/* check for over flow */
						StackPt = MAXIFD;
						error( "Error:IFD/IFND nested too deep" );
					}
					if( Pass == 2 && Lflag && !N_page )
						/* need to print here */
						print_line(); /* cuz will not return to normal */
						Token = GetToken(); /* get next line & examine for IF */
						State = FALSE_BLOCK; /* change state */
						break;
					case IF_ELSE:
						if( StackPt == 0 )
							/* bad IF nesting */
							error( "Error: ELSE without IF" );
						if( Pass == 2 && Lflag && !N_page )
							print_line();
						Token = GetToken(); /* get next line & examine for IF */
						State = FALSE_BLOCK;
						break;
					case IF_ENDIF:
						if( StackPt == 0 ) 	/* bad IF nesting */
							error( "Error: ENDIF without IF" );
						else
							StackPt--; /* popped state must be TRUE */
						Hiatus = TRUE;
						break;
						/* case NORMAL is not implemented as it represents normal line
									processing. */
					case IF_END:	/* file ended with improperly nested IFD */
						/* this code can't actually be reached at present
									in a TRUE_BLOCK */
						fatal( "Error: file ended before IFD/IFND/ELSE/ENDIF" );
						break;
					default:
						/* This code can't be reached at the present.
									Logically would happen if EOF but handled
									else where */
						fatal( "Can't get here from there." );
						break;
				}
				else if( State == FALSE_BLOCK )	/* statements not processed */
					switch( Token ){
						case IF_TRUE:	/* doesn't make any diff. Just nest IFs */
						case IF_FALSE:
							IfStack[StackPt] = FALSE;
							if( ++StackPt > MAXIFD ){
								StackPt = MAXIFD;
								error( "Error:IFD/IFND nested too deep" );
							}
							if( Pass == 2 && Lflag && !N_page )
								print_line();
							Token = GetToken();
							break;
						case IF_ELSE:
							if( Pass == 2 && Lflag && !N_page )
								print_line();
							State = ELSE_TEST;
							break;
						case IF_ENDIF:
							State = POP_TEST;
							break;
						case IF_END:
							/* file ended with improperly nested IFD */
							fatal( "Fatal Error: file ended before last ENDIF" );
							/* Fatal will exit assembler.  Things are too
										messed up.  Include file handling is else where
										and don't want to duplicate here. */
							break;
						case IF_NORMAL:	/* normal line in a FALSE BLOCK */
							if( Pass == 2 && Lflag && !N_page )
								print_line();
							Token = GetToken();
							break;
						default:
							fatal( "Fatal Error: file ended before last ENDIF" );
							/* must be EOF or something else terrible */
							break;
					}
				else if( State == POP_TEST ){	/* pop up outside nest state */
					if( StackPt == 0 ){	/* bad IF nesting */
						error( "Error: ENDIF without IF" );
					if( Pass == 2 && Lflag && !N_page )
						print_line();
					State = TRUE;
				}else{
					StackPt--;	/* pop stack */
					if( IfStack[StackPt] == TRUE ){	/* back to normal */
						/* had to come from FALSE block cuz TRUE block cannot
									be inside FALSE block */
						if( Pass == 2 && Lflag && !N_page )
							print_line();
						State = TRUE;
						Hiatus = TRUE;	/* sleep for normal line processing */
					}else{
						/* gotta be that stack == FALSE, still inside FALSE nest */
						if( Pass == 2 && Lflag && !N_page )
							print_line();
						State = FALSE;
						Token = GetToken();
					}
				}
			}else if( State == ELSE_TEST ){	/* change state */
				if( IfStack[StackPt-1] == TRUE ){
					State = TRUE_BLOCK;
					Hiatus = TRUE;
				}else
					State = FALSE_BLOCK;
			}
	}while( Hiatus == FALSE );	/* loop if not exit */
}


/*---------------------------------------------------------------------*
*	GetToken() --- get another line from within False Block and extract
*				token as would be done in pseudo.c.  Returns token id:
*				(similar to make_pass() except one line at a time
*				most variables & constants are global)
*			IF_TRUE		IFD/IFND evaluated to TRUE
*			IF_FALSE	IFD/IFND evaluated to FALSE
*			IF_ELSE		ELSE pseudo op
*			IF_ENDIF	ENDIF pseudo op
*			IF_END		END pseudo op
*			IF_NORMAL	none of the above, perhaps assy code
*			IF_EOF		encountered end of file
*		This function exists because conditional assembly was added
*		as pseudo op rather than with key character ( "%" ) and did
*		not want to disturb original software topology
*----------------------------------------------------------------------*/
int GetToken( void ){

	struct nlist *lookup();

	if( FGETS( Line, MAXBUF-1, Fd ) == (char *)NULL )

//	if( ( Line, MAXBUF-1, Fd ) == (char *)NULL )

		return( IF_EOF );	/* banged into eof */
	Line_num++;
	P_force = 0;
	N_page = 0;
	if( !parse_line() )
		return( IF_NORMAL );	/* comment line */
	if( *Op == EOS )			/* skipping over label, probably. */
		return( IF_NORMAL );	/* strcmp() returns 0 if arg1 is NULL string */

	if( strcmp( Op, "ifd" ) == 0 )
		return( eval_ifd() );
	else if( strcmp( Op, "ifnd" ) == 0 )
		return( eval_ifnd() );
	else if( strcmp( Op, "else" ) == 0 )
		return( IF_ELSE );
	else if( strcmp( Op, "endif" ) == 0 )
		return( IF_ENDIF );
	else if( strcmp( Op, "end" ) == 0 )
		return( IF_END );
	else
		return( IF_NORMAL );	/* or might be garbage...but who cares?
								This is FALSE BLOCK */
} /* end GetToken() */

/*---------------------------------------------------------------------*
*	eval_ifd() --- evaluate an if define statement for TRUE or FALSE
*----------------------------------------------------------------------*/
int eval_ifd( void ){

	struct nlist *np;	/* symbol structure */

	if( *Operand == EOS ) {
		warn( "No symbol for IFD" );
		return( IF_FALSE );	/* nothing to check */
	}
	if( (np = lookup( Operand )) == NULL )
		return( IF_FALSE );	/* not defined at all...yet */
	else
		if( np->def2 == Pass )
			return( IF_TRUE ); /* defined for this pass */

	return( IF_FALSE );	/* not defined this pass */

} /* eval_ifd() */

/*------------------------------------------------------------------------*
*	eval_ifnd() --- evaluate an if not define statement for TRUE or FALSE
*-------------------------------------------------------------------------*/
int eval_ifnd( void ){

	struct nlist *np;	/* symbol structure */

	if( *Operand == EOS ){
		warn( "No symbol for IFND" );
		return( IF_TRUE );	/* nothing to check */
	}
	if( (np = lookup( Operand )) == NULL )
		return( IF_TRUE );	/* not defined at all...yet */
	else
		if( np->def2 == Pass )
			return( IF_FALSE ); /* defined for this pass */

	return( IF_TRUE );	/* not defined this pass */

} /* end eval_ifnd() */

/*---------------------------------------------------------------------*
*	eval --- evaluate expression
*
*      an expression is constructed like this:
*
*      expr ::=  expr + term |
*                expr - term ;
*                expr * term ;
*                expr / term ;
*                expr | term ;
*                expr & term ;
*                expr % term ;
*                expr ^ term ;
*
*      term ::=  symbol |
*                * |
*                constant ;
*
*      symbol ::=  string of alphanumerics with non-initial digit
*
*      constant ::= hex constant |
*                   binary constant |
*                   octal constant |
*                   decimal constant |
*                   ascii constant;
*
*      hex constant ::= '$' {hex digits};
*
*      octal constant ::= '@' {octal digits};
*
*      binary constant ::= '%' { 1 | 0 };
*
*      decimal constant ::= {decimal digits};
*
*      ascii constant ::= ''' any printing char;
*
*----------------------------------------------------------------------*/
int eval( void ){

	int left,right;     /* left and right terms for expression */
	char o;             /* operator character */

#ifdef DEBUG
	printf( "Evaluating %s\n", Optr );
#endif
	Force_byte = NO;
	Force_word = NO;
	if( *Optr == '<' ){
		Force_byte++;
		Optr++;
	}else if( *Optr == '>' ){
		Force_word++;
		Optr++;
	}
	left = get_term();         /* pickup first part of expression */
	while( is_op( *Optr )){
		o = *Optr++; /* pickup connector and skip */
		right = get_term();     /* pickup current rightmost side */
		switch( o ){
			case '+':
				left += right;
				break;
			case '-':
				left -= right;
				break;
			case '*':
				left *= right;
				break;
			case '/':
				left /= right;
				break;
			case '|':
				left |= right;
				break;
			case '&':
				left &= right;
				break;
			case '%':
				left %= right;
				break;
			case '^':
				left = left^right;
				break;
		}
	}
	Result = left;
#ifdef DEBUG
	printf( "Result=%x\n", Result );
	printf( "Force_byte=%d  Force_word=%d\n", Force_byte, Force_word );
#endif
	return( YES );

} /* end eval() */

/*---------------------------------------------------------------------*
*	is_op --- is character an expression operator?
*----------------------------------------------------------------------*/
int is_op( char c ){

	if( any( c, "+-*/&%|^" ))
		return( YES );
	return( NO );
}

/*---------------------------------------------------------------------*
*	get_term --- evaluate a single item in an expression
*----------------------------------------------------------------------*/
int get_term( void ){

	char hold[MAXBUF];
	char *tmp;
	int val = 0;        /* local value being built */
	int minus;          /* unary minus flag */
	struct nlist *lookup(),*pointer;
	struct link *pnt,*bpnt;

	if( *Optr == '-' ){
		Optr++;
		minus = YES;
	}else
		minus = NO;

		while( *Optr == '#' )
			Optr++;

	/* look at rest of expression */
	if( *Optr == '%' ){
		/* binary constant */
		Optr++;
		while( any( *Optr, "01" ))
			val = (val * 2) + ( ( *Optr++ ) - '0' );
	}else if( *Optr == '@' ){
		/* octal constant */
		Optr++;
		while( any( *Optr, "01234567" ))
			val = (val * 8) + ( ( *Optr++ ) - '0' );
	}else if( *Optr == '$' ){
		/* hex constant */
		Optr++;
		while( any( *Optr, "0123456789abcdefABCDEF" ))
			if( *Optr > '9' )
				val = (val * 16) + 10 + ( mapdn( *Optr++ ) - 'a' );
			else
				val = (val * 16) + ( ( *Optr++ ) - '0' );
	}else if( any( *Optr, "0123456789" )){ /* decimal constant */
		while( *Optr >= '0' && *Optr <= '9' )
			val = (val * 10) + ( ( *Optr++ ) - '0' );
	}else if( *Optr == '*' ){    /* current location counter */
		Optr++;
		val = Old_pc;
	}else if( *Optr == '\'' ){   /* character literal */
		Optr++;
		if( *Optr == EOS )
			val = 0;
		else
			val = *Optr++;
	}else if( alpha( *Optr ) ){ /* a symbol */
		tmp = hold;     /* collect symbol name */
		while( alphan( *Optr ))
			*tmp++ = *Optr++;
		*tmp = EOS;
		pointer = lookup( hold );
		if( pointer != NULL ){
			if( Pass == 2 ){
				pnt = pointer->L_list;
				bpnt = NULL;
				while( pnt != NULL ){
					bpnt = pnt;
					pnt = pnt->next;
				}
				pnt = (struct link *) alloc( sizeof( struct link ));
				if( bpnt == NULL )
					pointer->L_list = pnt;
				else
					bpnt->next = pnt;
				pnt->L_num = Line_num;
				pnt->next = NULL;
			}
			val = Last_sym;
		}else{
			if( Pass == 1 ){
				/* forward ref here */
				fwdmark();
				if( !Force_byte )
					Force_word++;
				val = 0;
			}else
				/* added ver TER_2.0  2 Jul 89 */
				error( "Symbol undefined Pass 2" );
		}
		if( Pass == 2 && Line_num == F_ref && Cfn == Ffn ){
			if( !Force_byte  )
				Force_word++;
			fwdnext();
		}
	}else
		/* none of the above */
		val = 0;

	if( minus )
		return( -val );
	else
		return( val );

} /* end get_term() */

/*---------------------------------------------------------------------*
*	install --- add a symbol to the table
*----------------------------------------------------------------------*/
int install( char *str, int val ){

	struct link *lp;
	struct nlist *np,*p,*backp;
	struct nlist *lookup();
	int     i;
	char *LastChar();	/* ver TER_2.0 */


	if( !alpha( *str ) ){
		error( "Error: Illegal Symbol Name" );
		return(NO);
	}
	if( ( np = lookup( str )) != NULL )
	{
		if( *LastChar( str ) == '@' ){	/* redefinable symbol  ver TER_2.0 */
			np->def = val;		/* redefined */
			return( YES );
		}
		
		if( Pass == 2 ){
			np->def2 = 2;	/* defined for this pass=Pass  ver TER_2.0 */
			if( np->def == val )
				return( YES );
			else{
				error( "Error: Phasing Error" );
				return( NO );
			}
		}
		printf( "Error: Symbol Redefined" );
		exit(1);
		return( NO );
	}
	/* enter new symbol */

//Diman Begin
//	printf( "Installing %s as %d\n", str, val );
//Diman End

	np = (struct nlist *) alloc( sizeof( struct nlist ));
	if( np == (struct nlist *)ERR ){
		error( "Symbol table full" );
		return( NO );
	}
	np->name = alloc( strlen( str ) + 1 );
	if( np->name == (char *)ERR ){
		error( "Symbol table full" );
		return( NO );
	}
	strcpy( np->name, str );
	np->def = val;
	np->def2 = 1;	/* defined for this pass=Pass  ver TER_2.0 */
	np->Lnext = NULL;
	np->Rnext = NULL;
	lp = (struct link *) alloc( sizeof( struct link ));
	np->L_list = lp;
	lp->L_num = Line_num;
	lp->next = NULL;
	p = root;
	backp = NULL;
	while( p != NULL ){
		backp = p;
		i = strcmp( str, p->name );
		if( i < 0 )
			p = p->Lnext;
		else
			p = p->Rnext;
	}
	if( backp == NULL )
		root = np;
	else if( strcmp( str, backp->name ) < 0 )
		backp->Lnext = np;
	else backp->Rnext = np;
		return( YES );
}

/*---------------------------------------------------------------------*
*	lookup --- find string in symbol table
*----------------------------------------------------------------------*/
struct nlist *lookup( char *name ){

	struct nlist *np;
	int i;
	char *c;	/* ver TER_2.0 */

	c = name;	/* copy symbol pointer */
	while( alphan( *c ) )	/* search for end of symbol */
		c++;	/* next char */
	*c = EOS;	/* disconnect anything after for good compare */
	/* end of mods for ver TER_2.0 */

	np = root;	/* and now go on and look for symbol */
	while( np != NULL ){
		i = strcmp( name, np->name );
		if( i == 0 ){
			Last_sym = np->def;
			return( np );
		}else if( i < 0 )
			np = np->Lnext;
		else
			np = np->Rnext;
	}
	Last_sym = 0;
	/* if( Pass == 2 )
		error( "symbol Undefined on pass 2" ); */
		/* commented out ver TER_2.0 2 Jul 89 */
		/* function used in IFD */
	return( NULL );
}

#define NMNE (sizeof(table)/ sizeof(struct oper))
#define NPSE (sizeof(pseudo)/ sizeof(struct oper))
/*---------------------------------------------------------------------*
*      mne_look --- mnemonic lookup
*
*      Return pointer to an oper structure if found.
*      Searches both the machine mnemonic table and the pseudo table.
*----------------------------------------------------------------------*/
struct oper *mne_look( char *str ){

	struct oper *low,*high,*mid;
	int cond;

	/* Search machine mnemonics first */
	low =  &table[0];
	high = &table[ NMNE-1 ];
	while( low <= high){
		mid = low + ( high - low )/2;
		if( (  cond = strcmp( str, mid->mnemonic )) < 0 )
			high = mid - 1;
		else if( cond > 0 )
			low = mid + 1;
		else
			return( mid );
	}

	/* Check for pseudo ops */
	low =  &pseudo[0];
	high = &pseudo[NPSE-1];
	while( low <= high ){
		mid = low + ( high - low )/2;
		if( (  cond = strcasecmp( str, mid->mnemonic )) < 0 )
			high = mid - 1;
		else if( cond > 0 )
			low = mid + 1;
		else
			return( mid );
	}

	return( NULL );
}

/*---------------------------------------------------------------------*
*	fatal --- fatal error handler
*----------------------------------------------------------------------*/
void fatal( char *str ){

	char buf[256];

	pouterror();	/* added ver TER_2.0 4 Jul 89 */
	
	sprintf( buf, "%s\n", str );
	fprintf( out, buf );
	
	if( ERLflag )
		fputs( buf, EFp );

	exit( 1 );	/* Amiga & UNIX prefer positive numbers for error
			minimal error is 10 (no system prob) */
			//Bullshit, Diman Todorov
}

/*---------------------------------------------------------------------*
*      error --- error in a line
*                      print line number and error
*----------------------------------------------------------------------*/
void error( char *str ){

	char buf[256];

/* if(N_files > 1)	commented out test for N_files in rel TER_2.0
			because a single command line source file
			(which is what N_files counts) can have multiple
			include source files.
*/
	pouterror();
	sprintf( buf, "%s\n", str );
	fprintf( out, buf );
	if( ERLflag ){
		fputs( buf, EFp );
		fputs( Line, EFp );
	}
}

/*---------------------------------------------------------------------*
*      warn --- trivial error in a line
*                      print line number and error
*----------------------------------------------------------------------*/
void warn( char *str ){

	char buf[256];

/* if(N_files > 1) 	commented out in rel TER_2.0 same reason as above
*/
	strcpy( cfbuf, Argv[Cfn] );
	fprintf( out, "%s, line no.", cfbuf );	/* current file name */
	fprintf( out, "%d: ", Line_num );	/* current line number */
	fprintf( out, "Warning --- %s\n", str );
	Page_lines++;
	if( ERLflag ){
		fputs( Line, EFp );
		sprintf( buf, "\nWarning --- %s, line no. %d\n", str, Line_num );
		fputs( buf, EFp );
		fputs( Line, EFp );
	}
}

/*---------------------------------------------------------------------*
*	delim --- check if character is a delimiter
*----------------------------------------------------------------------*/
int delim( char c ){

	if( any( c, " \t\n" ) )
		return( YES );
	return( NO );
}

/*---------------------------------------------------------------------*
*	skip_white --- move pointer to next non-whitespace char
*----------------------------------------------------------------------*/
char *skip_white( char *ptr ){

	while( *ptr == BLANK || *ptr == TAB )
		ptr++;
	return( ptr );
}

/*---------------------------------------------------------------------
	eword --- emit a word to code file
----------------------------------------------------------------------*/
void eword( int wd ){

	emit( hibyte( wd ));
	emit( lobyte( wd ));
}

/*---------------------------------------------------------------------*
*	emit --- emit a byte to code file
*----------------------------------------------------------------------*/
void emit( int byte ){

#ifdef DEBUG
	printf( "%2x @ %4x\n", byte, Pc );
#endif
	if( Pass == 1 ){
		Pc++;
		return;
	}
	if( P_total < P_LIMIT )
		P_bytes[P_total++] = byte;
	E_bytes[E_total++] = byte;
	Pc++;
	if( E_total == E_LIMIT )
		f_record();
}

/*---------------------------------------------------------------------*
*	f_record --- flush record out in `S1' format
*    			made void for ANSI C compat ver TER_2.0 6/18/89
*----------------------------------------------------------------------*/
void f_record( void ){

	int i;
	int chksum;
//	printf("entering frecord pass = %d E_total %d\n", Pass, E_total);

	if( Pass == 1 )
		return;

	if( E_total == 0 )
	{
		E_pc = Pc;
		return;
	}

	F_total += E_total;			/* total bytes in file ver (TER)2.01 19 Jun 89 */
	chksum =  E_total + 3;		/* total bytes in this record */
	chksum += lobyte( E_pc );
	chksum += E_pc>>8;
	fprintf( Objfil, "S1" );	/* record header preamble */
	hexout( E_total + 3 );		/* byte count +3 */
	hexout( E_pc>>8 );			/* high byte of PC */
	hexout( lobyte( E_pc )); 	/* low byte of PC */
	for( i=0; i<E_total; i++ ){
		chksum += lobyte( E_bytes[i] );
		hexout( lobyte( E_bytes[i] )); /* data byte */
	}
	chksum =~ chksum;       /* one's complement */
	hexout( lobyte( chksum )); /* checksum */
	if( CRflag == 1 )	/* test for CRflag added ver TER_1.1 */
		fprintf( Objfil, "%c\n", CR );  /* print in IBM format for some PROM boxes */
	else
		fprintf( Objfil, "\n" ); /* this is original statement */
	E_pc = Pc;
	E_total = 0;
}

char *hexstr = { "0123456789ABCDEF" } ;

/*--------------------------------------------------------------------*
*	hexout()
*---------------------------------------------------------------------*/
void hexout( int byte ){

	byte = lobyte( byte );
	fprintf( Objfil, "%c%c", hexstr[byte>>4], hexstr[byte&017] );
}

/*---------------------------------------------------------------------*
*	print_line --- pretty print input line
*----------------------------------------------------------------------*/
void print_line( void ){

	int i;
	char buf[256];
	register char *ptr;

	fprintf( out, "%04d ", Line_num );
	if( P_total || P_force )
		fprintf( out, "%04x", Old_pc );
	else
		fprintf( out, "    " );

	for( i=0; i<P_total && i<6; i++ )
		fprintf( out, " %02x", lobyte( P_bytes[i] ));
	for( ; i<6; i++ )
		fprintf( out, "   " );
	fprintf( out, "  " );

	if( Cflag ){
		if( Cycles )
			fprintf( out, "[%2d ] ", Cycles );
		else
			fprintf( out, "      " );
	}
	ptr = Line;
	/* just echo the line back out */
/*	while( *ptr != '\n' )   
		putchar( *ptr++ ); */
	strcpy( buf, Line );
	*strchr( buf, '\n' ) = '\0';
	fprintf( out, buf );

	for( ; i<P_total; i++ ){
		if( i%6 == 0 )
			fprintf( out, "\n    " );
		fprintf( out, " %02x", lobyte( P_bytes[i] ));
	}
	if( Pflag50 && (++Page_lines > 50) )	/* form feed if flag set */
		NewPage();			/* ver (TER) 2.02 19 Jun 89 */
	fprintf( out, "\n" );
}

/*---------------------------------------------------------------------*
*	any --- does str contain c?
*----------------------------------------------------------------------*/
int any( char c, char *str ){

	while( *str != EOS )
		if( *str++ == c )
			return( YES );
	return( NO );
}

/*---------------------------------------------------------------------*
*	mapdn --- convert A-Z to a-z
*----------------------------------------------------------------------*/
char mapdn( char c ){

	if( c >= 'A' && c <= 'Z' )
		return( (char)( c + 040 )); /* cast value to char for ANSI C, ver TER_2.0 */
	return( c );
}

/*---------------------------------------------------------------------*
*	lobyte --- return low byte of an int
*----------------------------------------------------------------------*/
int lobyte( int i ){

	return( i & 0xFF );
}

/*---------------------------------------------------------------------*
*	hibyte --- return high byte of an int
*----------------------------------------------------------------------*/
int hibyte( int i ){

	return( ( i >> 8 ) & 0xFF );
}

/*---------------------------------------------------------------------*
*	head --- is str2 the head of str1?
*----------------------------------------------------------------------*/
int head( char *str1, char *str2 ){

	while( *str1 != EOS && *str2 != EOS ){
		if( *str1 != *str2 )
			break;
		str1++;
		str2++;
	}
	if( *str1 == *str2 )
		return( YES );
	if( *str2 == EOS )
		if( any( *str1, " \t\n, +-]; *" ))
			return( YES );
	return( NO );
}

/*---------------------------------------------------------------------*
*	alpha --- is character a legal letter
*----------------------------------------------------------------------*/
int alpha( char c ){

	if( c<= 'z' && c>= 'a' )
		return( YES );
	if( c<= 'Z' && c>= 'A' )
		return( YES );
	if( c== '_' )
		return( YES );
	if( c== '.' )
		return( YES );
	return( NO );
}

/*---------------------------------------------------------------------*
*	alphan --- is character a legal letter or digit
*----------------------------------------------------------------------*/
int alphan( char c ){

	if( alpha( c ) )
		return( YES );
	if( c<= '9' && c>= '0' )
		return( YES );
	if( c == '$' )
		return( YES );      /* allow imbedded $ */
	if( c == '@' )
		return( YES );	/* allow imbedded @, added ver TER_2.0
						added to permit redefinable variables */
	return( NO );
}

/*---------------------------------------------------------------------*
*	white --- is character whitespace?
*----------------------------------------------------------------------*/
int white( char c ){

	if( c == TAB || c == BLANK || c == '\n' )
		return( YES );
	return( NO );
}

/*---------------------------------------------------------------------*
*	alloc --- allocate memory
*----------------------------------------------------------------------*/
char *alloc( int nbytes ){
	char * malloc();
	return( malloc( nbytes ));
}

/*---------------------------------------------------------------------*
*	FNameGet --- Find a file name <file> or "file" in the Operand string
*	added ver TER_2.0 6/17/89
*
*	Argument: NameString - pointer to output string
*
*	note: this routine will return a file name with white
*	space if between delimeters.  This is permitted in
*	AmigaDOS.  Other DOS may hiccup or just use name up to white space 
*----------------------------------------------------------------------*/
int FNameGet( char *NameString ){

	char *frompoint;	/* pointers to input string, don't bash Operand */
	char *topoint;		/* pointer to output string, don't bash NameString */

	frompoint = Operand;	/* include file name is in parsed string Operand */
	topoint = NameString;	/* copy of pointer to increment in copying */
	if( *frompoint != '<' && *frompoint != '"')
		return( 0 ); /* bad syntax */
	frompoint++;	/* skip < or " */

	while( *frompoint != '>' && *frompoint != '"')
	{
		/* look for delimeter */
		if( *frompoint == EOS )
			return( 0 );	/* missing delimeter */
		*topoint++ = *frompoint++;	/* copy path & file name for DOS */
	}

	*topoint = EOS;	/* terminate file name */
	return( 1 );	/* proper syntax anyway */
}

/*---------------------------------------------------------------------*
* --- strsave()  find a place to save a string & return pointer to it
* 		added ver TER_2.0 6/18/89  function taken from
*		Kernighan & Ritchie 78 
*----------------------------------------------------------------------*/
char *strsave( char *s ){

	char *p;

	if( ( p = alloc( strlen( s ) + 1 )) != NULL )
		strcpy( p, s );
	return( p );
}

/*---------------------------------------------------------------------*
* pouterror() ---- print out standard error header
* 			added rel TER_2.0 6/18/89
*----------------------------------------------------------------------*/
void pouterror( void ){

	char buf[256], *charp;

	strcpy( cfbuf, Argv[Cfn] );
	fprintf( out, "%s, line no. ", cfbuf );	/* current file name */
	fprintf( out, "%d: ", Line_num );	/* current line number */
	/* NOTE: THERE IS NO \n ! Calling procedure supplies suffixing
		error message and \n. viz. file pseudo.c procedure do_pseudo
		in case INCLUDE. Note also that error count is incremented.  */
	Err_count++;
	Page_lines++;	/* increment lines per page */
	if( ERLflag ){
		sprintf( buf, "%s, line no. %d: ", cfbuf, Line_num );
		fputs( buf, EFp );
	}
}

/*---------------------------------------------------------------------*
* New Page() --- form feed a new page, print heading & inc page number
*		Moved here from do_pseudo (pseudo.c) in ver (TER) 2.02
*		19 Jun 89 so that can call from print_line() as well
*		for p50 option. 
*----------------------------------------------------------------------*/
void NewPage( void ){

	char *charp;

	Page_lines = 0;	/* ver TER_2.08 so that OPT PAGE works */
	fprintf( out, "\f" );
	strcpy( cfbuf, Argv[Cfn] );
	fprintf( out, "%-10s", charp );
	fprintf( out, "                                   " );
	fprintf( out, "page %3d\n", Page_num++ );
}

/*---------------------------------------------------------------------*
* LastChar() ----- return a pointer to the last character in a string
*		Exception: will return garbage if NULL string
*
*	Argument:	strpt -  pointer to string to be examined
*----------------------------------------------------------------------*/
char *LastChar( char *strpt ){

	char *c;

	c = strpt;	/* don't zap original */
	while( *c != EOS )	/* search for end */
		c++;
	return( --c );	/* back up one to last character */
}

/*---------------------------------------------------------------------*
*	file I/O version of forward ref handler
*----------------------------------------------------------------------*/

#define FILEMODE 0644 /* file creat mode */
#define UPDATE  2 /* file open mode */
#define ABS  0 /* absolute seek */

int Forward =0;  /* temp file's file descriptor */
char Fwd_name[] = { "Fwd_refs" } ;

/*---------------------------------------------------------------------*
*	fwdinit --- initialize forward ref file
*----------------------------------------------------------------------*/
void fwdinit( void ){

	Forward = creat( Fwd_name, FILEMODE );
	if( Forward < 0 )
		fatal( "Can't create temp file" );
	close( Forward ); /* close and reopen for reads and writes */
	Forward = open( Fwd_name, UPDATE );
	if( Forward < 0 )
		fatal( "Forward ref file has gone." );
	unlink( Fwd_name );
}

/*---------------------------------------------------------------------*
*	fwdreinit --- reinitialize forward ref file
*----------------------------------------------------------------------*/
void fwdreinit( void ){

	F_ref = 0;
	Ffn = 0;
	lseek( Forward, 0L, ABS );   /* rewind forward refs */
	read( Forward, &Ffn, sizeof( Ffn ));
	read( Forward, &F_ref, sizeof( F_ref )); /* read first forward ref into mem */
#ifdef DEBUG
	printf( "First fwd ref: %d,%d\n", Ffn, F_ref );
#endif
}

/*---------------------------------------------------------------------*
*	fwdmark --- mark current file/line as containing a forward ref
*----------------------------------------------------------------------*/
void fwdmark( void ){

	write( Forward, &Cfn, sizeof( Cfn ));
	write( Forward, &Line_num, sizeof( Line_num ));
}

/*---------------------------------------------------------------------*
*	fwdnext --- get next forward ref
*----------------------------------------------------------------------*/
void fwdnext( void ){

	int stat;

	stat = read( Forward, &Ffn, sizeof( Ffn ));
#ifdef DEBUG
	printf( "Ffn stat=%d ", stat );
#endif
	stat = read( Forward, &F_ref, sizeof( F_ref ));
#ifdef DEBUG
	printf( "F_ref stat=%d  ", stat );
#endif
	if( stat < 2 ){
		F_ref = 0; 
		Ffn = 0;
	}
#ifdef DEBUG
	printf( "Next Fwd ref: %d,%d\n", Ffn, F_ref );
#endif
}

/*---------------------------------------------------------------------*
*	stable --- prints the symbol table in alphabetical order
*----------------------------------------------------------------------*/
void stable( struct nlist *ptr ){

	if( ptr != NULL){
		stable( ptr->Lnext );
		fprintf( out, "%-10s %04x\n", ptr->name, ptr->def );
		stable( ptr->Rnext );
	}
}

/*---------------------------------------------------------------------*
*	cross  --  prints the cross reference table
*----------------------------------------------------------------------*/
void cross( struct nlist *point ){

	struct link *tp;
	int i = 1;

	if( point != NULL ){
		cross( point->Lnext );
		fprintf( out, "%-10s %04x *", point->name, point->def );
		tp = point->L_list;
		while( tp != NULL ){
			if( i++ > 10 ){
				i = 1;
				fprintf( out, "\n                      " );
			}
			fprintf( out, "%04d ", tp->L_num );
			tp = tp->next;
		}
		fprintf( out, "\n" );
		cross( point->Rnext );
	}
}

