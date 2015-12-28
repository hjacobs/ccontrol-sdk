/**********************************************************
	EASYCONTROL - 68HC05B6 Source

	CODEDEFS.H
**********************************************************/


#ifndef __CODEDEFS_H
#define __CODEDEFS_H


/*******************************************
*** Kommandos des Host-PC an EasyControl ***
*******************************************/


#define CMD_NOCOMMAND	0
// kein Befehl
// Parameter: keine

#define CMD_GETINFO 1
// gibt Versionsinfo-String an serieller Schnittstelle aus
// Parameter: keine

#define CMD_PROGRAM     2
// Programm-Download ins serielle EEPROM

#define CMD_DUMPPROG 3
#define CMD_EETEST 4
#define CMD_START 5
#define CMD_STOP 6
#define CMD_DEBUG 7
#define CMD_USERCODE 8
#define CMD_DUMPUSERCODE 9

#define CMD_DUMPFILE	13
#define CMD_SETTIME	14
#define CMD_RESET 255

/*******************************************
*** Makrocodes fuer das Anwenderprogramm ***
*******************************************/

// Systembefehle
#define CODE_NOP		0	// n.p.
#define CODE_BEGIN		1	// n.p.
#define CODE_WAIT		2	// word delay
#define CODE_GOTO		3	// word adr
#define CODE_IFNOTGOTO		4	// word adr
#define CODE_GOSUB		5	// word adr
#define CODE_RETURN		6	// n.p.
#define CODE_RETURNINTERRUPT    7	// n.p.
#define CODE_SYS		8	// word adr
#define CODE_SLOWMODE		9	// byte on/off

#define CODE_END		0xFF	// n.p.

// nur fuer Debugcode
#define CODE_LINENR CODE_END-1  // word BASIC-Zeilennummer
#define CODE_WAITLOOP CODE_END-2  // n.p.


// Portzugriffe
#define CODE_GETPORT		10	// byte port nr
#define CODE_GETBYTEPORT	11	// byte port nr
#define CODE_GETWORDPORT	12	// n.p.
#define CODE_SETPORT		13	// byte port nr
#define CODE_SETBYTEPORT	14	// byte port nr
#define CODE_SETWORDPORT	15	// n.p.
#define CODE_ADC      		16	// byte ad nr
#define CODE_DAC     		17	// byte da nr

// Speicheroperationen
#define CODE_LOADIM      	20	// word value
#define CODE_LOADBIT	 	21	// byte bit nr
#define CODE_LOADBYTE    	22	// byte byte nr
#define CODE_LOADWORD	 	23	// byte word nr
#define CODE_STOREBIT		24	// byte bit nr
#define CODE_STOREBYTE		25	// byte byte nr
#define CODE_STOREWORD		26	// byte word nr
#define CODE_LOOKUP		27	// word table adr
#define CODE_LOADSPECIALDATA	28	// byte id
#define CODE_STORESPECIALDATA   29	// byte id

// special data id
#define SPECIALDATA_YEAR	0	
#define SPECIALDATA_MONTH	1
#define SPECIALDATA_DAY		2
#define SPECIALDATA_DAYOWEEK	3
#define SPECIALDATA_HOUR	4
#define SPECIALDATA_MIN		5
#define SPECIALDATA_SEC		6
#define SPECIALDATA_TIMER	7
#define SPECIALDATA_USERTICKS	8
#define SPECIALDATA_FREQHZ	9
#define SPECIALDATA_FREQHZ2	10
#define SPECIALDATA_FILEFREE	11
#define SPECIALDATA_FILELEN	12

// Ausgabefunktionen
#define CODE_SENDRESULT  	30	// n.p.
#define CODE_SENDTEXT		31	// null term. string
#define CODE_SENDBYTE		32	// byte byte
#define CODE_ISBYTEAVAIL	33	// n.p.
#define CODE_GETBYTE		34	// n.p.
#define CODE_INPUT		35	// n.p.
#define CODE_HANDSHAKE		36	// byte on/off
#define CODE_ISCTS		37	// n.p.

// logische Operationen
#define CODE_NOT		40	// n.p.
#define CODE_AND		41	// n.p.
#define CODE_NAND		42	// n.p.
#define CODE_OR			43	// n.p.
#define CODE_NOR		44	// n.p.
#define CODE_XOR		45	// n.p.
#define CODE_SHL		46	// n.p.
#define CODE_SHR		47	// n.p.

#define CODE_RANDOMIZE		48	// n.p.
#define CODE_RAND		49	// n.p.

// numerische Operationen
#define CODE_NEG		50	// n.p.
#define CODE_ADD		51	// n.p.
#define CODE_SUB		52	// n.p.
#define CODE_MUL		53	// n.p.
#define CODE_DIV		54	// n.p.
#define CODE_MOD		55	// n.p.
#define CODE_ABS		56	// n.p.
#define CODE_ROOT		57	// n.p.
#define CODE_MAX	 	58	// n.p.
#define CODE_MIN	 	59	// n.p.

// Vergleichsoperationen
#define CODE_HIGHER		60	// n.p.
#define CODE_HIGHERSAME		61	// n.p.
#define CODE_LOWER		62	// n.p.
#define CODE_LOWERSAME		63	// n.p.
#define CODE_EQUAL		64	// n.p.
#define CODE_NOTEQUAL		65	// n.p.
#define CODE_SGN		66	// n.p.

#define CODE_FILEIO		70	// byte file command

#define FILE_OPENFORREAD	1
#define FILE_OPENFORWRITE	2
#define FILE_OPENFORAPPEND	3
#define FILE_CLOSE		4
#define FILE_PRINT		5
#define FILE_INPUT		6
#define FILE_DUMP		7

#define CODE_CHECKEOF		71
#define CODE_SETBAUD		72
#define CODE_SETINTERRUPT	73

#define CODE_ONBYTEEQUAL	74
#define CODE_ONWORDEQUAL	75
#define CODE_ADDBYTEANDGOTO	76
#define CODE_ADDWORDANDGOTO	77

// weitere Portzugriffe
#define CODE_TOGGLEPORT		80
#define CODE_PULSEPORT		81
#define CODE_DEACTPORT		82
#define CODE_DEACTBYTEPORT	83
#define CODE_DEACTWORDPORT	84

#define LASTCODE		CODE_DEACTWORDPORT


#endif
