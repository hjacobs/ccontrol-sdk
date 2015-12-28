/********************************************************************

  compscan.cpp

  Complex Scans

********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fstream.h>

#include "compiler.hpp"
#include "68HC05B6/codedefs.h"

#include "comperr.h"
#include "textres.h"
#include "keywords.h"

const char S19FILENAME[] = "SYSCODE.S19";
const char ASMFILENAME[] = "SYSCODE.ASM";
const char ASMCALL[] = "ASM.BAT";

static const char KEYWORD_IF[] = "IF";
static const char KEYWORD_THEN[] = "THEN";
static const char KEYWORD_ELSE[] = "ELSE";

static const char KEYWORD_FOR[] = "FOR";
static const char KEYWORD_TO[] = "TO";
static const char KEYWORD_STEP[] = "STEP";
static const char KEYWORD_NEXT[] = "NEXT";

//static const char KEYWORD_WHILE[] = "WHILE";
//static const char KEYWORD_WEND[] = "WEND";

static const char KEYWORD_GOTO[] = "GOTO";
static const char KEYWORD_GOSUB[] = "GOSUB";
static const char KEYWORD_RETURN[] = "RETURN";
static const char KEYWORD_SYS [] = "SYS";

static const char KEYWORD_LOOKTAB[] = "LOOKTAB";
static const char KEYWORD_RANDOMIZE[] = "RANDOMIZE";

static const char KEYWORD_BAUD[] = "BAUD";
static const char KEYWORD_GET[] = "GET";
static const char KEYWORD_PUT[] = "PUT";
static const char KEYWORD_PRINT[] = "PRINT";
static const char KEYWORD_INPUT[] = "INPUT";

static const char KEYWORD_OPENFILE[] = "OPEN#";
static const char KEYWORD_CLOSEFILE[] = "CLOSE#";
static const char KEYWORD_READ[] = "READ";
static const char KEYWORD_WRITE[] = "WRITE";
static const char KEYWORD_APPEND[] = "APPEND";
static const char KEYWORD_PRINTFILE[] = "PRINT#";
static const char KEYWORD_INPUTFILE[] = "INPUT#";

static const char KEYWORD_SLOWMODE[] = "SLOWMODE";
static const char KEYWORD_INTERRUPT[] = "INTERRUPT";
static const char KEYWORD_PAUSE[] = "PAUSE";
static const char KEYWORD_WAIT[] = "WAIT";
static const char KEYWORD_END[] = "END";
static const char KEYWORD_BEEP[] = "BEEP";

static const char KEYWORD_TABLE[] = "TABLE";
static const char KEYWORD_TABEND[] = "TABEND";

static const char KEYWORD_SYSCODE[] = "SYSCODE";
static const char KEYWORD_CODEEND[] = "SYSEND";
//static const char KEYWORD_ASM[] = "ASM";

static const char KEYWORD_HANDSHAKE[] = "HANDSHAKE";

static const char KEYWORD_DEACT[] = "DEACT";
static const char KEYWORD_TOG[] = "TOG";
static const char KEYWORD_PULSE[] = "PULSE";

const char KEYWORD_AT[] = "AT";
const char KEYWORD_INITMODEM[] = "INITMODEM";
const char KEYWORD_SENDMAIL[] = "SENDMAIL";
const char KEYWORD_GETMAIL[] = "GETMAIL";
const char KEYWORD_RETRY[] = "RETRY";

// Zugriff auf Ports und Daten

//const int MAX_USERBYTES = 24;
//const int MAX_USERWORDS = 12;
//const int MAX_USERBITS = 192;
const int MAX_USERBYTES = 1024;
const int MAX_USERWORDS = 512;
const int MAX_USERBITS = 8192;
//changed by Diman Todorov 19.4.2001

const int MAX_USERBYTEPORTS = 2;
const int MAX_USERPORTS = 16;
const int MAX_USERWORDPORTS = 1;
const int MAX_ADPORTS = 8;
const int MAX_DAPORTS = 2;
const int MAX_AD12PORTS = 2;

const char* USERDATA_KEYWORDS[] = { "BIT", "BYTE", "WORD", "PORT", "BYTEPORT",
												"WORDPORT", "AD", "DA",
												"",  /*wg. SPECIALDATA*/
												"AD12" };
const int USERDATA_LIMITS[] = { MAX_USERBITS, MAX_USERBYTES, MAX_USERWORDS,
										  MAX_USERPORTS, MAX_USERBYTEPORTS,
										  MAX_USERWORDPORTS, MAX_ADPORTS, MAX_DAPORTS,
										  0,0 };




//-----------------------------------------------------------------
  void CCompiler::ScanStatement ( char* word ) throw(COMPILERERROR)
//-----------------------------------------------------------------
{
  char message[50];

  char label1[20];
  char label2[20];


  int flag = bHasNetAccessInThisStatement;
  bHasNetAccessInThisStatement = 0;

  char errlabbuf[20];
  strcpy(errlabbuf, onerrorlabel);
  MakeLabel(onerrorlabel);

  if ( ! word )
	 word = ScanWord();

  int bDoRetry = STREQU(word, KEYWORD_RETRY);

  if ( bDoRetry )
  {
	 DefineLabel(onerrorlabel);
	 word = ScanWord();
  }

  PutLineInfoCode();

  // IF term THEN statement [ELSE statement]
  if ( STREQU(word, KEYWORD_IF) )
  {
	 bInsideIfThenLine = 1;
	 ScanTerm();

	 // Sprung wenn term-Ergebnis false
	 PutCode(CODE_IFNOTGOTO); MakeLabel(label1); UseForwardLabel(label1);

	 // nach THEN steht das Statement fuer term = true
	 Validate(ScanWord(), KEYWORD_THEN);
	 ScanStatement();

	 if ( EOS() )
		DefineLabel(label1);
	 else
	 {
		Validate(ScanWord(), KEYWORD_ELSE);
		PutCode(CODE_GOTO); MakeLabel(label2); UseForwardLabel(label2);
		DefineLabel(label1);

		// Statement im ELSE-Zweig
		ScanStatement();
		DefineLabel(label2);
	 }
	 bInsideIfThenLine = 0;
  }

  // FOR assignment TO term [STEP term]
  else if ( STREQU(word, KEYWORD_FOR) )
  {
	 ValidateNotEOS();

	 // Startwertzuweisung an Laufvariable
	 long variable = ScanAssignment();

// Aenderung am 20.11.97 wg. Syntaxerweiterung
	 if ( variable == -1 )
	 {
		sprintf(message, "%s", TXT_SYNTAXERROR);
		ThrowError(COMPERR_SYNTAXERROR, message);
	 }

	 // Schluesselwort TO muss folgen
	 Validate(ScanWord(), KEYWORD_TO);

	 // Schleifenkopf merken
	 new FORTOLINE(line+scanpos, variable, GetCodeAdr());

	 // Zeile nicht weiter auswerten
	 line[scanpos] = 0;
  }

  // NEXT
  else if ( STREQU(word, KEYWORD_NEXT) )
  {
	// NEXT ohne FOR -> Fehler
	if ( FORTOLINE::last == NULL )
	{
	  sprintf(message, "%s", TXT_NEXTWITHOUTFOR);
	  ThrowError(COMPERR_NEXTWITHOUTFOR, message);
	}

	 // Schleifenkopf laden
	 strcpy(line, FORTOLINE::last->buf);
	scanpos = 0;

	// TO-Ausdruck berechnen; Laufvariable laden; wenn gleich, dann Abbruch
	ScanTerm();

	 switch ( HIBYTE(FORTOLINE::last->variable) )
	 {
	  case USERBYTE: PutCode(CODE_ONBYTEEQUAL); break;
	  case USERWORD: PutCode(CODE_ONWORDEQUAL); break;
	  default:break;
	  // Fehler-nur Userbytes und words zulaessig
	 }
	PutCode(LOBYTE(FORTOLINE::last->variable));
	MakeLabel(label1); UseForwardLabel(label1);

	// STEP-Ausdruck berechnen oder 1,
	if ( STREQU(ScanWord(), KEYWORD_STEP ) )
	  ScanTerm();
	else
	{
	  PutCode(CODE_LOADIM);
	  PutCode(0);
	  PutCode(1);
	}

	 switch ( HIBYTE(FORTOLINE::last->variable) )
	 {
	  case USERBYTE: PutCode(CODE_ADDBYTEANDGOTO); break;
	  case USERWORD: PutCode(CODE_ADDWORDANDGOTO); break;
	  default:		  break;
	  // Fehler-nur Userbytes und words zulaessig
	 }
	 PutCode(LOBYTE(FORTOLINE::last->variable));
	 PutCode(HIBYTE(FORTOLINE::last->headadr));
	 PutCode(LOBYTE(FORTOLINE::last->headadr));

	 if ( !bInsideIfThenLine )
		delete FORTOLINE::last;

	 // Ende der Scheife
	 DefineLabel(label1);
  }

/*  else if ( STREQU(word, KEYWORD_WHILE) )
  {
  } */

  else if ( STREQU(word, KEYWORD_ON) )
  {
	 long variable;

	 word = ScanWord();
	 if ( !IsVariable(word, variable) )
	 {
		sprintf(message, "%s %s", TXT_VARIABLE, TXT_EXPECTED);
		ThrowError(COMPERR_VAREXPECTED, message);
	 }

	 // GOSUB oder GOTO
	 unsigned char code;

	 word = ScanWord();
	 if ( STREQU(word, KEYWORD_GOTO) )
		code = CODE_GOTO;
	 else if ( STREQU(word, KEYWORD_GOSUB) )
		code = CODE_GOSUB;
	 else
	 {
		sprintf(message, "%s/%s %s", KEYWORD_GOSUB, KEYWORD_GOTO, TXT_EXPECTED);
		ThrowError(COMPERR_GOTOGOSUBEXPECTED, message);
	 }

	 // Sprungselektion
	 unsigned char selection = 0;

	 while ( !EOS() )
	 {
		if ( selection  )
		  Validate(ScanChar(), ',');

		PutLoadVarCode(variable);
		PutCode(CODE_LOADIM); PutCode(0); PutCode(selection);
		PutCode(CODE_EQUAL);
		PutCode(CODE_IFNOTGOTO); MakeLabel(label1); UseForwardLabel(label1);

		PutCode(code);
		word = ScanWord();
		CAlias* pLabelAlias = labels.Find(word);
		if ( pLabelAlias == NULL )
		  UseForwardLabel(word);
		else
		{
		  PutCode(HIBYTE(pLabelAlias->GetValue()));
		  PutCode(LOBYTE(pLabelAlias->GetValue()));
		}
		DefineLabel(label1);
		selection++;
	 }
  }
  else if ( STREQU(word, KEYWORD_INITMODEM) )
	 PutInitModemCode ();
  else
	 ScanInstruction(word);

  if ( bHasNetAccessInThisStatement )
	 PutGosubCheckMailCode();
  if ( !bDoRetry )
	 DefineLabel(onerrorlabel);

  bHasNetAccessInThisStatement = flag ;
  strcpy(onerrorlabel, errlabbuf);
}



//----------------------------------------------
  void CCompiler::ScanInstruction ( char* word )
//----------------------------------------------
{
  // Instructions sind Zuweisungen, Anweisungen und Sprungbefehle
  char message[50];
  long variable;
  long value;

  if ( !word ) word = ScanWord();

  //==============
  // Ein-/Ausgabe
  //==============

  //------------------------------
  if ( STREQU(word, KEYWORD_GET) )
  {
	PutCode(CODE_GETBYTE);
	word = ScanWord();
	if ( IsVariable(word, variable) )
	  PutStoreVarCode(variable);
	else
	{
	  sprintf(message, "%s %s", TXT_VARIABLE, TXT_EXPECTED);
	  ThrowError(COMPERR_VAREXPECTED, message);
	}
  }

  //-----------------------------------
  else if ( STREQU(word, KEYWORD_PUT) )
  {
	 ScanTerm();
	 PutCode(CODE_SENDBYTE);
  }

  //-------------------------------------
  else if ( STREQU(word, KEYWORD_PRINT) )
  {
	 do
	{
	  if ( !TestWord(KEYWORD_ELSE) )
	  {
		 // Textausgabe
		 if ( TestChar('"') )
		 {
			char ch;
			ScanChar();
			PutCode(CODE_SENDTEXT);
			while ( '"' != (ch = ScanChar(0)) )
			{
			  ValidateNotEOL();
			  PutCode(ch);
			}
			PutCode(0);
		 }
		 else if ( !EOS() )
		{
		  ScanTerm();
		  PutCode(CODE_SENDRESULT);
		 }
	  }
	  switch ( NextChar() )
	  {
		case ';': ScanChar(); break;
		case ',': ScanChar(); PutCode(CODE_SENDTEXT); PutCode(0x9); PutCode(0); break;
		default:  PutCode(CODE_SENDTEXT); PutCode(0xD);  PutCode(0xA);
				  PutCode(0); break;
	  }
	}
	while ( !( EOS() || TestWord(KEYWORD_ELSE)) );
  }

  //-------------------------------------
  else if ( STREQU(word, KEYWORD_INPUT) )
  {
	PutCode(CODE_INPUT);
	ValidateNotEOS();
	word = ScanWord();
	if ( IsVariable(word, variable) )
	  PutStoreVarCode(variable);
	else
	{
	  sprintf(message, "%s %s", TXT_VARIABLE, TXT_EXPECTED);
	  ThrowError(COMPERR_VAREXPECTED, message);
	}
  }

  //-------------------------------------
  else if ( STREQU(word, KEYWORD_HANDSHAKE) )
  {
	 PutCode(CODE_HANDSHAKE);
	 ValidateNotEOS();
	 word = ScanWord();
	 if ( IsConstant(word, value) )
		PutCode(LOBYTE(value)); // ACHTUNG nur ein Byte Parameter!!!
	 else
	 {
		sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
		ThrowError(COMPERR_CONSTEXPECTED, message);
	 }
  }

  //============
  // Dateiarbeit
  //============

  //----------------------------------------
  else if ( STREQU(word, KEYWORD_OPENFILE) )
  {
	PutCode(CODE_FILEIO);

	if ( EOS() )
	{
	  PutCode(FILE_OPENFORWRITE);
	  return;
	}

	Validate(ScanWord(), KEYWORD_FOR);
	ValidateNotEOS();

	word = ScanWord();

	 if ( STREQU(word, KEYWORD_WRITE) ) PutCode(FILE_OPENFORWRITE);
	 else if ( STREQU(word, KEYWORD_APPEND) ) PutCode(FILE_OPENFORAPPEND);
	 else if ( STREQU(word, KEYWORD_READ) ) PutCode(FILE_OPENFORREAD);
	 else
	 {
		sprintf(message, "%s/%s/%s %s", KEYWORD_READ, KEYWORD_WRITE,
				  KEYWORD_APPEND, TXT_EXPECTED);
		ThrowError(COMPERR_SYNTAX, message);
	 }
  }

  //-----------------------------------------
  else if ( STREQU(word, KEYWORD_CLOSEFILE) )
  {
	PutCode(CODE_FILEIO);
	PutCode(FILE_CLOSE);
  }

  //-----------------------------------------
  else if ( STREQU(word, KEYWORD_PRINTFILE) )
  {
	ScanTerm();
	PutCode(CODE_FILEIO);
	PutCode(FILE_PRINT);
  }

  //-----------------------------------------
  else if ( STREQU(word, KEYWORD_INPUTFILE) )
  {
	PutCode(CODE_FILEIO);
	PutCode(FILE_INPUT);
	word = ScanWord();
	long variable;
	if ( IsVariable(word, variable) )
	  PutStoreVarCode(variable);
	else
	{
	  sprintf(message, "%s %s", TXT_VARIABLE, TXT_EXPECTED);
	  ThrowError(COMPERR_VAREXPECTED, message);
	}
  }

  //-------------------
  // Spezialanweisungen
  //-------------------

  //---------------------------------
  else if ( STREQU(word, KEYWORD_TABLE) )
  {
	 ValidateNotEOS();

	 // Tabellenbezeichner
	 word = ScanWord();
	 if ( labels.Find(word) )
	 {
		sprintf(message, "%s %s", TXT_REDEFINEDLABEL, word);
		ThrowError(COMPERR_LABELREDEFINED, message);
	 }

	 DefineLabel(word);

	 // Tabellendateiname
	 if ( TestChar('"') )
	 {
		ScanChar();
		SkipWhites();

		char filename[256];
		int i = 0;

		while ( !(EOS() || TestChar('"')) )
		  filename[i++] = line[scanpos++];
		filename[i] = 0;

		Validate(ScanChar(), '"');
		ValidateEOS();
		ScanTableFromFile(filename);
	 }
	 // oder Tabellendaten inline
	 else
	 {
		bInsideTable = 1;
		ScanTableData();
	 }
  }

  //---------------------------------
  else if ( STREQU(word, KEYWORD_LOOKTAB) )
  {
	 ValidateNotEOS();

	 char* table = strdup(ScanWord());
	 CAlias* pLabelAlias = labels.Find(table);

	 // Indexberechnung
	 Validate(ScanChar(), ',');
	 ScanTerm();

	 // Lookup-Token mit Tabellenadresse
	 PutCode(CODE_LOOKUP);
	 if ( pLabelAlias == NULL )
	 {
		UseForwardLabel(table);
	 }
	 else
	 {
		PutCode(HIBYTE(pLabelAlias->GetValue()));
		PutCode(LOBYTE(pLabelAlias->GetValue()));
	 }
	 free(table);

	 // Ergebnis in einer Variablen speichern
	 long variable;
	 Validate(ScanChar(), ',');
	 word = ScanWord();
	 if ( IsVariable(word, variable) )
		PutStoreVarCode(variable);
	 else
	 {
		sprintf(message, "%s %s", TXT_VARIABLE, TXT_EXPECTED);
		ThrowError(COMPERR_VAREXPECTED, message);
	 }
  }

  //---------------------------------
  else if ( STREQU(word, KEYWORD_RANDOMIZE) )
  {
	 ScanTerm();
	 PutCode(CODE_RANDOMIZE);
  }

  //------------------------------------
  else if ( STREQU(word, KEYWORD_BAUD) )
  {
	 PutCode(CODE_SETBAUD);
	 word = ScanWord();
	 if ( IsConstant(word, variable) )
		PutCode((unsigned char) variable);
	 else
	 {
		sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
		ThrowError(COMPERR_CONSTEXPECTED, message);
	 }
  }

  //----------------------------------------
  else if ( STREQU(word, KEYWORD_SLOWMODE) )
  {
	 PutCode(CODE_SLOWMODE);
	 word = ScanWord();
	 if ( IsConstant(word, variable) )
		PutCode((unsigned char) variable);
	 else
	 {
		sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
		ThrowError(COMPERR_CONSTEXPECTED, message);
	 }
  }

  //-----------------------------------------
  else if ( STREQU(word, KEYWORD_INTERRUPT) )
  {
	PutCode(CODE_SETINTERRUPT);
	word = ScanWord();

	if ( STREQU(word, KEYWORD_OFF) )
	{
	  PutCode(0);
	  PutCode(0);
	}
	else
	{
	  CAlias* pLabelAlias = labels.Find(word);
	  if ( pLabelAlias == NULL )
	  {
	    UseForwardLabel(word);
	  }
	  else
	  {
	    PutCode(HIBYTE(pLabelAlias->GetValue()));
	    PutCode(LOBYTE(pLabelAlias->GetValue()));
	  }
	}
  }
  //-------------------------------------
  else if ( STREQU(word, KEYWORD_PAUSE) )
  {
	ScanTerm();
	PutCode(CODE_WAIT);
  }

  //------------------------------------
  else if ( STREQU(word, KEYWORD_WAIT) )
  {
	if ( bDoIncludeDebugInfo )
	  PutCode(CODE_NOP);

	unsigned int adr = GetCodeAdr();

	if ( bDoIncludeDebugInfo )
	  PutCode(CODE_WAITLOOP);

	ScanTerm();
	PutCode(CODE_IFNOTGOTO);
	PutCode(HIBYTE(adr));
	PutCode(LOBYTE(adr));
  }

  //-----------------------------------
  else if ( STREQU(word, KEYWORD_END) )
  {
	 PutCode(CODE_END);
  }

  //------------------------------------
  else if ( STREQU(word, KEYWORD_BEEP) )
  {
	 unsigned pos = scanpos;
	 word = ScanWord();

	 if ( STREQU(word, KEYWORD_OFF) )
	 {
		PutCode(CODE_LOADIM);
		PutCode(0); PutCode(0);
		PutCode(CODE_STORESPECIALDATA);
		PutCode(SPECIALDATA_USERTICKS);
	 }
	 else
	 {
		char label[20];
		scanpos = pos;
		// Ton an
		ScanTerm();
		PutCode(CODE_STORESPECIALDATA);
		PutCode(SPECIALDATA_USERTICKS);

		// Tondauer
		Validate(ScanChar(), ',');
		ScanTerm();
		PutCode(CODE_WAIT);

		// Dauer = 0 entspricht endlos
		PutCode(CODE_IFNOTGOTO);
		MakeLabel(label); UseForwardLabel(label);

		// sonst Ton aus
		PutCode(CODE_LOADIM);
		PutCode(0); PutCode(0);
		PutCode(CODE_STORESPECIALDATA);
		PutCode(SPECIALDATA_USERTICKS);

		DefineLabel(label);

		// Pausendauer
		Validate(ScanChar(), ',');
		ScanTerm();
		PutCode(CODE_WAIT);
	 }
  }

  //------------------
  // Sprunganweisungen
  //------------------

  //------------------------------------
  else if ( STREQU(word, KEYWORD_GOTO) )
  {
	 PutCode(CODE_GOTO);

	 word = ScanWord();
	 CAlias* pLabelAlias = labels.Find(word);
	 if ( pLabelAlias == NULL )
	 {
		UseForwardLabel(word);
	 }
	 else
	 {
		PutCode(HIBYTE(pLabelAlias->GetValue()));
		PutCode(LOBYTE(pLabelAlias->GetValue()));
	 }
  }

  //-------------------------------------
  else if ( STREQU(word, KEYWORD_GOSUB) )
  {
	 PutCode(CODE_GOSUB);

	 word = ScanWord();
	 CAlias* pLabelAlias = labels.Find(word);
	 if ( pLabelAlias == NULL )
	 {
		UseForwardLabel(word);
	 }
	 else
	 {
		PutCode(HIBYTE(pLabelAlias->GetValue()));
		PutCode(LOBYTE(pLabelAlias->GetValue()));
	 }
  }

  //--------------------------------------
  else if ( STREQU(word, KEYWORD_RETURN) )
  {
	 if ( EOS() )
		PutCode(CODE_RETURN);
	 else
	 {
		if ( TestWord(KEYWORD_INTERRUPT) )
		{
		  PutCode(CODE_RETURNINTERRUPT);
		  ScanWord();
		}
		else
		{
		  ScanTerm();
		  PutCode(CODE_RETURN);
		}
	 }
  }

  //---------------------------------------
  else if ( STREQU(word, KEYWORD_SYSCODE) )
  {
	 // ASM oder S19-Dateiname
	 if ( TestChar('"') )
	 {
		ScanChar();
		SkipWhites();

		char filename[256];
		int i = 0;

		while ( !(EOS() || TestChar('"')) )
		  filename[i++] = line[scanpos++];
		filename[i] = 0;

		Validate(ScanChar(), '"');
		ValidateEOS();
		ScanSysCodeFromS19File(filename);
	 }
	 else
	 {
		unsigned pos = scanpos;

		// inline asm entfällt vorerst
/*		if ( STREQU(ScanWord(), KEYWORD_ASM) )
		{
		  pAsmStream = new ofstream(ASMFILENAME);
		  if ( pAsmStream->fail() )
		  {

			 ThrowError(FATAL_CANTWRITE, message);
		  }
		  bInsideAsmCode = 1;
		  ValidateEOL();
		}
		else
		{*/
		scanpos = pos;
		bInsideSysCode = 1;
		ScanSysCode();
//		}
	 }
  }

  //-----------------------------------
  else if ( STREQU(word, KEYWORD_SYS) )
  {
	 long sysadr;

	 ValidateNotEOS();

	 if ( !IsConstant(ScanWord(), sysadr) )
	 {
		sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
		ThrowError(COMPERR_CONSTEXPECTED, message);
	 }
/*	 if ( !EOS() )
		for(;;)
		{
		  ScanTerm();
		  if ( EOS() ) break;
		  Validate(ScanChar(), ',');
		}*/

	 while ( !EOS() )
	 {
		ScanTerm();
		if ( !TestChar(',') ) break;
		ScanChar();
	 }

	 PutCode(CODE_SYS);
	 PutCode(HIBYTE(sysadr));
	 PutCode(LOBYTE(sysadr));
  }

  //----------------
  // Portanweisungen
  //----------------

  //-------------------------------------
  else if ( STREQU(word, KEYWORD_DEACT) )
  {
	 ValidateNotEOS();
	 word = ScanWord();
	 if ( !IsVariable(word, variable) )
	 {
		sprintf(message, "%s %s", TXT_BITPORTVARIABLE, TXT_EXPECTED);
		ThrowError(COMPERR_PORTEXPECTED, message);
	 }

	 unsigned char index = LOBYTE(variable);
    unsigned char remoteid = LOBYTE(HIWORD(variable));

	 switch ( HIBYTE(variable) )
	 {
		case USERPORT: PutCode(CODE_DEACTPORT); PutCode(index); break;
		case USERBYTEPORT: PutCode(CODE_DEACTBYTEPORT); PutCode(index); break;
		case USERWORDPORT: PutCode(CODE_DEACTWORDPORT); break;
		case REMOTEPORT: PutDeactRemotePortCode(remoteid, index); break;

		default:
		  sprintf(message, "%s %s", TXT_BITPORTVARIABLE, TXT_EXPECTED);
		  ThrowError(COMPERR_PORTEXPECTED, message);
		break;
	 }
  }

  //-------------------------------------
  else if ( STREQU(word, KEYWORD_TOG) )
  {
	 ValidateNotEOS();
	 word = ScanWord();
	 if ( !IsVariable(word, variable) )
	 {
		sprintf(message, "%s %s", TXT_BITPORTVARIABLE, TXT_EXPECTED);
		ThrowError(COMPERR_PORTEXPECTED, message);
	 }

	 if ( HIBYTE(variable) == USERPORT )
	 {
		PutCode(CODE_TOGGLEPORT);
		PutCode(LOBYTE(variable));
	 }
	 else
	 {
		sprintf(message, "%s %s", TXT_BITPORTVARIABLE, TXT_EXPECTED);
		ThrowError(COMPERR_PORTEXPECTED, message);
	 }
  }

  //-------------------------------------
  else if ( STREQU(word, KEYWORD_PULSE) )
  {
	 ValidateNotEOS();
	 word = ScanWord();
	 if ( !IsVariable(word, variable) )
	 {
		sprintf(message, "%s %s", TXT_BITPORTVARIABLE, TXT_EXPECTED);
		ThrowError(COMPERR_PORTEXPECTED, message);
	 }

	 if ( HIBYTE(variable) == USERPORT )
	 {
		PutCode(CODE_PULSEPORT);
		PutCode(LOBYTE(variable));
	 }
	 else
	 {
		sprintf(message, "%s %s", TXT_BITPORTVARIABLE, TXT_EXPECTED);
		ThrowError(COMPERR_PORTEXPECTED, message);
	 }
  }


  // neu ab 14.07.98 sendmail und getmail für power line networking
  else if ( STREQU(word, KEYWORD_SENDMAIL) )
	 PutSendMailCode();
  else if ( STREQU(word, KEYWORD_GETMAIL) )
	 PutGetMailCode();



  //----------
  // Zuweisung
  //----------
// Aenderung am 20.11.97 wg. Syntaxerweiterung
  else if ( -1 == ScanAssignment(word) )
  {
	 // goto ohne goto-Schluesselwort
	 PutCode(CODE_GOTO);

	 CAlias* pLabelAlias = labels.Find(word);
	 if ( pLabelAlias == NULL )
	 {
		UseForwardLabel(word);
	 }
	 else
	 {
		PutCode(HIBYTE(pLabelAlias->GetValue()));
		PutCode(LOBYTE(pLabelAlias->GetValue()));
	 }
  }
}


//---------------------------------------------------------
  long CCompiler::ScanAssignment ( char* word ) throw(COMPILERERROR)
//---------------------------------------------------------
{
  char message[50];

  if ( !word ) word = ScanWord();

  long variable;

  if ( IsVariable(word, variable) )
  {
	 Validate(ScanChar(),'=');
	 ScanTerm();
	 PutStoreVarCode(variable);
	 return variable;
  }
  else
  {
	 if ( NextChar() == '=' )
	 {
		sprintf(message, "%s %s", word, TXT_UNDEFINEDVAR);
		ThrowError(COMPERR_VAREXPECTED, message);
	 }
  }
// Aenderung am 20.11.97 wg. Syntaxerweiterung
  return -1;
}



//----------------------------------------------
  void CCompiler::ScanDefinition () throw(COMPILERERROR)
//----------------------------------------------
{
  // Einlesen des neu definierten Bezeichners
  char message[50];

  char* defname = ScanWord();

  // wenn kein gueltiger Bezeichner, dann Fehler
  if ( !IsIdentifier(defname) )
  {
	 sprintf(message, "%s", TXT_INVALIDSYMBOL);
	 ThrowError(COMPERR_INVALIDSYMBOL, message);
  }

  // wenn schon definiert, dann Fehler
  if ( constants.Find(defname) || vars.Find(defname) )
  {
	 sprintf(message, "%s %s", TXT_REDEFINEDSYMBOL, defname);
	 ThrowError(COMPERR_SYMBOLREDEFINED, message);
  }

  defname = strdup(defname);
  ValidateNotEOL();
  char* valstr = ScanWord();
  long value;

  // Definition einer Konstanten durch eine andere Konstante
  if ( IsConstant(valstr, value) )
	 constants.Define(defname, value);

  // Definition einer Variablen durch eine andere Variable
  else if ( IsVariable(valstr, value) )
	 vars.Define(defname, value);

  else
  {
	 int index = GetKeywordIndex(USERDATA_KEYWORDS, sizeof(USERDATA_KEYWORDS)/
													 sizeof(USERDATA_KEYWORDS[0]), valstr);

	 if ( -1 != index )
	 {
		if ( TestChar('[') )
		{
		  ScanChar();

		  if ( !IsConstant(ScanWord(), value) )
		  {
			 sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
			 ThrowError(COMPERR_CONSTEXPECTED, message);
		  }

		  Validate(ScanChar(),']');

		  if ( EOS() )
		  {
			 Validate(value, (long)USERDATA_LIMITS[index], 1L);
			 value--;
			 value |= index << 8;
		  }
		  // remote resource
		  else
		  {
			 long remoteid;

			 Validate( ScanWord(), KEYWORD_AT);

			 if ( !IsConstant(ScanWord(), remoteid) )
			 {
				sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
				ThrowError(COMPERR_CONSTEXPECTED, message);
			 }

			 switch ( index )
			 {
				case USERPORT:
				  value--;
				  value = 1L << (short) value;         // -> Maske

				  if ( (remoteid < 4) || (remoteid > 31) ) // remote IO-Geräte-Adressen 0...3
					 value |= REMOTEPORT << 8;
				  else                        // AC-Switch, Beeper
					 value |= REMOTESTATE << 8;
				  break;

				case USERBYTEPORT:
				  value = 0xFF;

				  if ( (remoteid < 4) || (remoteid > 31) )
					 value |= REMOTEPORT << 8;
				  else                        // AC-Switch, Remote-Buttons
					 value |= REMOTESTATE << 8;
				  break;

				case ADPORT:

				  value--;
				  value |= REMOTEADPORT << 8;
				  break;

				case REMOTE12BITADPORT:
				  value--;
				  value |= REMOTE12BITADPORT << 8;
				  break;
			 }
			 value |= remoteid << 16;
		  }

		  vars.Define(defname, value);
		}
		else
		{
		  if ( index == 1 ) // auto. Byte
		  {
			 value = nextByteIndex;
			 nextByteIndex++;
			 nextWordIndex = (nextByteIndex + 1) >> 1;
		  }
		  else if ( index == 2 ) // auto. Word
		  {
			 value = nextWordIndex;
			 nextWordIndex++;
			 nextByteIndex = nextWordIndex << 1;
		  }
		  else
		  {
			 sprintf(message, "%s %s", TXT_BITNUMBER, TXT_EXPECTED);
			 ThrowError(COMPERR_MISSINGINDEX, message);
		  }

		  value |= index << 8;
		  vars.Define(defname, value);
		}
	 }
	 else
	 {
		sprintf(message, "%s %s", TXT_UNDEFINEDIDENTIFIER, defname);
		ThrowError(COMPERR_UNDEFINEDSYMBOL, message);
	 }
  }
  free(defname);
}


//---------------------------------------------
  void CCompiler::ScanTableData () throw(COMPILERERROR)
//---------------------------------------------
{
  char message[50];
  char* word;
  long value;

  while (! EOS() )
  {
	 word = ScanWord();

	 if ( STREQU(word, KEYWORD_TABEND) )
	 {
		bInsideTable = 0;
		break;
	 }
	 else if ( IsConstant(word, value) )
	 {
		PutCode(HIBYTE(value));
		PutCode(LOBYTE(value));
	 }
	 else
	 {
		sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
		ThrowError(COMPERR_CONSTEXPECTED, message);
	 }
  }
}


//-----------------------------------------------------------------
  void CCompiler::ScanTableFromFile ( char* filename ) throw(COMPILERERROR)
//-----------------------------------------------------------------
{
  char message[500];
  float value;
  int ivalue;

  ifstream is;
  char fullfilename[512];
  
  //changed \\ to / (dos to unix directories)  _CHANGE_ Diman Todorov 
  sprintf(fullfilename, "%s/%s", pSourcePath, filename); 
  is.open(fullfilename, ios::in|ios::nocreate);

  if ( is.fail() )
  {
	 sprintf(message, "%s %s", TXT_FILENOTFOUND, fullfilename);
	 ThrowError(COMPERR_FILENOTFOUND, message);
  }

  while ( !is.eof() )
  {
	 is >> value;
	 ivalue = (int) value;
	 PutCode(HIBYTE(ivalue));
	 PutCode(LOBYTE(ivalue));
  }

  is.close();
}

//-------------------------------------------
  void CCompiler::ScanSysCode () throw(COMPILERERROR)
//-------------------------------------------
{
  char message[50];
  char* word;
  long value;

  while (! EOS() )
  {
	 word = ScanWord();

	 if ( STREQU(word, KEYWORD_CODEEND) )
	 {
		bInsideSysCode = 0;
		break;
	 }
	 else if ( IsConstant(word, value) )
		PutSysCode((unsigned char)(value));
	 else
	 {
		sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
		ThrowError(COMPERR_CONSTEXPECTED, message);
	 }
  }
}

//----------------------------------------------------------------------
  void CCompiler::ScanSysCodeFromS19File ( char* filename ) throw(COMPILERERROR)
//----------------------------------------------------------------------
{
  char message[500];
  char s19line[256];
  char substr[5];
  char* ptr;
  unsigned nbytes;
  unsigned byte;

  ifstream is;
  char fullfilename[512];

  //change \\ to / (dos to unix directories) _CHANGE_ Diman Todorov
  sprintf(fullfilename, "%s/%s", pSourcePath, filename);
  is.open(fullfilename, ios::in|ios::nocreate);

  if ( is.fail() )
  {
	 sprintf(message, "%s %s", TXT_FILENOTFOUND, fullfilename);
	 ThrowError(COMPERR_FILENOTFOUND, message);
  }

  while ( !is.eof() )
  {
	 memset(s19line, 0, sizeof(s19line));
	 is.getline(s19line, 255);
	 if ( s19line[0] == 0 ) continue;

	 // erstes Zeichen muss ein S sein, dann mindestens 10 Zeichen
	 if ( ( toupper(s19line[0]) != 'S' ) || ( strlen(s19line) < 10 ) )
	 {
		sprintf(message, "%s", TXT_INVALIDFILEFORMAT);
		ThrowError(COMPERR_INVALIDFILEFORMAT, message);
	 }

	 // Codebyte-Zeile
	 if ( s19line[1] == '1' )
	 {
		substr[0] = s19line[2];
		substr[1] = s19line[3];
		substr[2] = 0;
		sscanf(substr, "%X", &nbytes);

		// ersten 2 Bytes sind Adresse, letztes Byte ist Checksumme -
		// interessiert alles nicht
		nbytes -= 3;

		// hier beginnen die Codes, vorausgesetzt wird, dass alle Adressen sich
		// auf den EEPROM-Bereich beschraenken

		ptr = s19line + 8;

		while ( nbytes )
		{
		  substr[0] = ptr[0];
		  substr[1] = ptr[1];
		  substr[2] = 0;
		  sscanf(substr, "%X", &byte);
		  PutSysCode(byte);
		  ptr += 2;
		  nbytes--;
		}
	 }
  }

  is.close();
}

//-----------------------------
 void	CCompiler::ScanAsmLine ()
//-----------------------------
{
  if ( !EOL() )
  {
	 if ( STREQU(ScanWord(), KEYWORD_CODEEND) )
	 {
		TerminateAsmCode();
		return;
	 }
  }

  (*pAsmStream) << codebuf << '\n';
}

//-----------------------------------
  void CCompiler::TerminateAsmCode ()
//-----------------------------------
{
  bInsideAsmCode = 0;
  pAsmStream->close();
  delete pAsmStream;
  pAsmStream = NULL;
  system(ASMCALL);
  ScanSysCodeFromS19File((char*)S19FILENAME);
}
