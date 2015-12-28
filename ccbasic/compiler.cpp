/********************************************************************

  compiler.cpp

********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "compiler.hpp"
#include "68HC05B6/codedefs.h"
#include "watchcod.h"

// fuer Fehlermelungen
#include "comperr.h"
#include "textres.h"


#define PROGSTART	4

// *** vordefinierte Bezeichner, Schlüsselworte usw. ***


// Bezeichner der Systemvariablen (Spezialdaten)

const char* SPECDATA_KEYWORDS[] = { "YEAR", "MONTH", "DAY", "DOW",
												"HOUR", "MINUTE", "SECOND", "TIMER",
												"TICKS","FREQ1", "FREQ2", "FILEFREE" };

const int FIRST_SPECIALDATA_WORD_INDEX = 7; // ab Timer beginnen WORD-Daten, vorher Bytes

const char* KEYWORD_FREQ = "FREQ";

// Vordefinierte Konstanten
const char* KEYWORD_ON = "ON";
const int KEYWORD_ON_VALUE = 0xFFFF;
const char* KEYWORD_OFF =	"OFF";
const int KEYWORD_OFF_VALUE = 0;

const char* BAUDRATE_KEYWORDS[] = { "R1200", "R2400", "R4800", "R9600" };
const int BAUDRATE_VALUES[] = { 0xDB, 0xD2, 0xC9, 0xC0 };

// sonstige Schluesselworte
const char KEYWORD_DEFINE[] = "DEFINE";
const char KEYWORD_NETWORK[] = "NETWORK";

//-------------------------------------------
  CCompiler::CCompiler ( char* _pSourcePath )
//-------------------------------------------
{
  memset(this, 0, sizeof(CCompiler));

  constants.Define(KEYWORD_ON, KEYWORD_ON_VALUE);
  constants.Define(KEYWORD_OFF, KEYWORD_OFF_VALUE);

  int i;

  for ( i = 0; i < sizeof(BAUDRATE_KEYWORDS)/sizeof(BAUDRATE_KEYWORDS[0]); i++)
	 constants.Define(BAUDRATE_KEYWORDS[i], BAUDRATE_VALUES[i]);

  for ( i = 0; i < sizeof(SPECDATA_KEYWORDS)/sizeof(SPECDATA_KEYWORDS[0]); i++)
	 vars.Define(SPECDATA_KEYWORDS[i], (SPECIALDATA << 8) + i);

  // FREQ = FREQ1
  vars.Define(KEYWORD_FREQ, (SPECIALDATA << 8) + 9);

  if ( NULL ==  _pSourcePath )
	 pSourcePath = "";
  else
	 pSourcePath = _pSourcePath;
}


//------------------------
  CCompiler::~CCompiler ()
//------------------------
{
  LATELABEL::DestroyAll();
}


//------------------------------------------------------------------------------
  COMPILERERROR::COMPILERERROR ( unsigned _linenr, unsigned _id, char* message )
//------------------------------------------------------------------------------
{
  linenr = _linenr;
  id = _id;

  if ( message )
	 strcpy(msg, message);
  else
	 msg[0] = 0;
}


//--------------------------------------------------------
  void CCompiler::ThrowError( unsigned id, char* message )
//--------------------------------------------------------
{
  errcount++;
  throw COMPILERERROR(linenr, id, message);
}


// *** "strenge" Testfunktionen ***

//-----------------------------------------------------------
  void CCompiler::Validate ( char scan, char c ) throw(COMPILERERROR)
//-----------------------------------------------------------
{
  char message[50];

  if ( scan != c )
  {
	 sprintf(message, "%c %s", c, TXT_EXPECTED);
	 ThrowError(COMPERR_MISSINGCHAR, message);
  }
}

//-----------------------------------------------------------------------------
  void CCompiler::Validate ( long value, long hibound, long lobound ) throw(COMPILERERROR)
//-----------------------------------------------------------------------------
{
  char message[50];

  if ( lobound && hibound )
	 if ( (value < lobound) || (value > hibound) )
	 {
		sprintf(message, "%d %s", (int)value, TXT_OUTOFRANGE);
		ThrowError(COMPERR_OUTOFRANGE, message);
	 }
}

//--------------------------------------------------------------------------
  void  CCompiler::Validate ( char* word, const char* keyword ) throw(COMPILERERROR)
//--------------------------------------------------------------------------
{
  char message[50];

  if ( ! STREQU(word, keyword) )
  {
	 sprintf(message, "%s %s", keyword, TXT_EXPECTED);
	 ThrowError(COMPERR_MISSINGKEYWORD, message);
  }
}

//--------------------------------------------
  void  CCompiler::ValidateEOL () throw(COMPILERERROR)
//--------------------------------------------
{
  char message[50];

  if ( ! EOL() )
  {
	 sprintf(message, "%s %s", TXT_EOL, TXT_EXPECTED);
	 ThrowError(COMPERR_EOLEXPECTED, message);
  }
}


//-----------------------------------------------
  void  CCompiler::ValidateNotEOL () throw(COMPILERERROR)
//-----------------------------------------------
{
  char message[50];

  if ( EOL() )
  {
	 sprintf(message, "%s %s", TXT_UNEXPECTED, TXT_EOL);
	 ThrowError(COMPERR_UNEXPECTEDEOL, message);
  }
}

//--------------------------------------------
  void  CCompiler::ValidateEOS () throw(COMPILERERROR)
//--------------------------------------------
{
  char message[50];

  if ( !EOS() )
  {
	 sprintf(message, "%s %s", TXT_EOS, TXT_EXPECTED);
	 ThrowError(COMPERR_EOSEXPECTED, message);
  }
}


//-----------------------------------------------
  void  CCompiler::ValidateNotEOS () throw(COMPILERERROR)
//-----------------------------------------------
{
  char message[50];

  if ( EOS() )
  {
	 sprintf(message, "%s %s", TXT_UNEXPECTED, TXT_EOS);
	 ThrowError(COMPERR_UNEXPECTEDEOS, message);
  }
}

// *** Pruef- und Konvertierfunktionen ***

//------------------------------------------
  int CCompiler::IsIdentifier ( char* word )
//------------------------------------------
{
  if ( ! (isalpha(word[0]) || (word[0] =='_')) )
	 return 0;

  int i = 1;

  while ( word[i] )
  {
	 if ( ! (isalnum(word[i]) || (word[i] =='_')) )
		return 0;
	 i++;
  }

  return 1;
}


//------------------------------------------------------
  int CCompiler::IsConstant ( char* word, long& result )
//------------------------------------------------------
{
  // Test auf negatives Vorzeichen
  int neg = 0;
  if ( word[0] == '-' )
  {
	 neg = 1;
	 word++;
  }

  // Test auf symbolische Konstante
  if ( isalpha( word[0]) )
  {
	 CAlias* pConstAlias = constants.Find(word);

	 if ( pConstAlias )
	 {
		result = pConstAlias->GetValue();
		if ( neg ) result = -result;
		return 1;
	 }
	 else
		return 0;
  }

  // Test und Umwandlung einer Hex- oder Binzahl
  else if ( word[0] == '&' )
  {
	 int value;
	 int i = 0;
	 word++;

	 // Hex
	 if ( word[0] == 'H' )
	 {
		word++;

		while ( isxdigit(word[i]) )
		  i++;
		if ( word[i] )
		  return 0;

		sscanf(word, "%X", &value);
		if ( neg ) value = -value;
		result = (long)value;
	 }

	 // Bin
	 else if ( word[0] == 'B' )
	 {
		word++;
		value = 0;

		int i = 0;
		while ( (word[i] == '0') || (word[i] == '1') )
		{
		  value <<= 1;
		  value += (word[i] == '1');
		  i++;
		}
		if ( neg ) value = -value;

		if ( word[i] )
		  return 0;
		result = (long)value;
	 }

	 // Fehler
	 else
		return 0;
  }
  // Test und Umwandlung einer Dezimalzahl
  else
  {
    int value;
	 // Test, ob nur gueltige Zeichen (Dezimalziffern)
	 int i = 0;
	 while ( isdigit(word[i]) )
		i++;
	 if ( word[i] )
		return 0;

	 // Konvertieren

	 sscanf(word, "%d", &value);
	 if ( neg ) value = -value;
	 result = (long) value;
  }

  return 1;
}



//-----------------------------------------------------
  int CCompiler::IsVariable ( char* word, long& result )
//-----------------------------------------------------
{
  CAlias* pVarAlias = vars.Find(word);

  if ( pVarAlias )
  {
	 result = pVarAlias->GetValue();
	 return 1;
  }

  return 0;
}

//--------------------------------------------
  int CCompiler::TestWord ( const char* word )
//--------------------------------------------
{
  unsigned pos = scanpos;
  char* nextword = ScanWord();
  scanpos = pos;

  return ( STREQU(nextword, word) );
}





// *** Tools  ***

//---------------------------------------------------------------------------------
  int CCompiler::GetKeywordIndex ( const char** table, int numentries, char* word )
//---------------------------------------------------------------------------------
{
  int i;

  for ( i = 0; i < numentries; i++ )
	 if ( STREQU(table[i], word) )
		return i;

  return -1;
}


//---------------------------
  char CCompiler::NextChar ()
//---------------------------
{
  int i = scanpos;

  while ( line[i] )
  {
	if ( line[i] > ' ' )
	  break;
	i++;
  }

  return line[i];
}

//-----------------------------------------------------------
  void CCompiler::PutCode ( unsigned char code ) throw(COMPILERERROR)
//-----------------------------------------------------------
{
  char message[50];

  codebuf[codeindex++] = code;

  if ( codeindex == MAX_CODELEN )
  {
	 sprintf(message, "%s", TXT_PROGRAMTOOLONG);
	 ThrowError(COMPERR_PROGRAMTOOLONG, message);
  }
}


//-------------------------------------
  unsigned int CCompiler::GetCodeAdr ()
//-------------------------------------
{
  return PROGSTART + codeindex;
}


//------------------------------------------------
  void CCompiler::PutLoadVarCode ( long variable )
//------------------------------------------------
{
  unsigned char index = LOBYTE(variable);
  unsigned char remoteid = LOBYTE(HIWORD(variable));

  switch ( HIBYTE(variable) )
  {
	 case USERBIT: PutCode(CODE_LOADBIT); PutCode(index); break;
	 case USERBYTE: PutCode(CODE_LOADBYTE); PutCode(index);	break;
	 case USERWORD: PutCode(CODE_LOADWORD); PutCode(index); break;
	 case USERPORT: PutCode(CODE_GETPORT); PutCode(index); break;
	 case USERBYTEPORT: PutCode(CODE_GETBYTEPORT); PutCode(index); break;
	 case USERWORDPORT: PutCode(CODE_GETWORDPORT); break;
	 case ADPORT: PutCode(CODE_ADC); PutCode(index); break;
	 case SPECIALDATA: PutCode(CODE_LOADSPECIALDATA); PutCode(index); break;

	 // network
	 case REMOTEPORT:        PutGetRemotePortCode(remoteid, index); break;
	 case REMOTE12BITADPORT: PutGetRemote12BitADPortCode(remoteid, index); break;
	 case REMOTEADPORT:      PutGetRemoteADPortCode(remoteid, index); break;
	 case REMOTESTATE:       PutGetRemoteStateCode(remoteid); break;
  }
}

//-------------------------------------------------
  void CCompiler::PutStoreVarCode ( long variable )
//-------------------------------------------------
{
  unsigned char index = LOBYTE(LOWORD(variable));
  unsigned char remoteid = LOBYTE(HIWORD(variable));

  switch ( HIBYTE(LOWORD(variable)) )
  {
	 case USERBIT: PutCode(CODE_STOREBIT); PutCode(index); break;
	 case USERBYTE: PutCode(CODE_STOREBYTE); PutCode(index);	break;
	 case USERWORD: PutCode(CODE_STOREWORD); PutCode(index); break;
	 case USERPORT: PutCode(CODE_SETPORT); PutCode(index); break;
	 case USERBYTEPORT: PutCode(CODE_SETBYTEPORT); PutCode(index); break;
	 case USERWORDPORT: PutCode(CODE_SETWORDPORT); break;
	 case DAPORT: PutCode(CODE_DAC); PutCode(index); break;
	 case SPECIALDATA: PutCode(CODE_STORESPECIALDATA); PutCode(index); break;

	 // network
	 case REMOTEPORT:  PutSetRemotePortCode(remoteid, index); break;
	 case REMOTESTATE: PutSetRemoteStateCode (remoteid, index); break;
  }
}

//-----------------------------------------
  void CCompiler::MakeLabel ( char* label )
//-----------------------------------------
{
  sprintf(label, "__label_%d", helper_label_count++);
}


//------------------------------------------------
  void CCompiler::UseForwardLabel ( char* label )
//------------------------------------------------
{
  new LATELABEL(label, linenr, scanpos, codeindex);
  PutCode(0); PutCode(0);
}

//-------------------------------------------
  void CCompiler::DefineLabel ( char* label )
//-------------------------------------------
{
  labels.Define(label, GetCodeAdr());
}


//------------------------------------------------
  void CCompiler::FillInLateLabels () throw(COMPILERERROR)
//------------------------------------------------
{
  LATELABEL* l = LATELABEL::first;

  while (l)
  {
	 CAlias* pLabelAlias = labels.Find(l->name);

	 if ( pLabelAlias )
	 {
		codebuf[l->codeindex] = HIBYTE(pLabelAlias->GetValue());
		codebuf[l->codeindex+1] = LOBYTE(pLabelAlias->GetValue());
	 }
	 else
	 {
		COMPILERERROR e(l->linenr, COMPERR_UNDEFINEDLABEL);
		sprintf(e.msg, "%s %s", TXT_UNDEFINEDLABEL, l->name);
		errcount++;
		throw e;
	 }

	 l = l->next;
  }
}


//------------------------------------------
  int CCompiler::IsLabelUsed ( char* label )
//------------------------------------------
{
  LATELABEL* l = LATELABEL::first;

  while (l)
  {
	 if ( STREQU(label, l->name) )
		return 1;
	 else
		l = l->next;
  }
  return 0;
}


//--------------------------------------------
  void CCompiler::TestRemarkChar () throw(int)
//--------------------------------------------
{
  int i=0;

  if ( TestChar(96) ) throw i;
}


//--------------------------------------------------------------
  void CCompiler::PutSysCode ( unsigned char code ) throw(COMPILERERROR)
//--------------------------------------------------------------
{
  char message[50];

  syscodebuf[syscodeindex++] = code;

  if ( syscodeindex == MAX_SYSCODELEN )
  {
	 sprintf(message, "%s", TXT_SYSPROGRAMTOOLONG);
	 ThrowError(COMPERR_SYSPROGRAMTOOLONG, message);
  }
}


//----------------------------------------
  void CCompiler::PutLineInfoCode ( void )
//----------------------------------------
{
  if ( bDoIncludeDebugInfo )
  {
    PutCode(CODE_LINENR);
    PutCode(HIBYTE((unsigned short)linenr));
	 PutCode(LOBYTE((unsigned short)linenr));
  }
  if ( pValidBreakpoints )
  {
     pValidBreakpoints->Define(NULL, linenr);
  }
}


//--------------------------------
  void CCompiler::PutNumPopCode ()
//--------------------------------
{
  // super hack! springt in die MAX-funktion,
  // so dass A=B und NumPop ausgeführt werden Adr 0x1498
  PutCode(CODE_SYS);
  PutCode(0x14);
  PutCode(0x98);
}


//--------------------------------
  void CCompiler::PutEmptyRXCode ()
//--------------------------------
{
  // ruft im Controller RS232_EmptyRX() auf, Adresse 0x0CE0 siehe CCBASIC.MAP
  PutCode(CODE_SYS);
  PutCode(0x0C);
  PutCode(0xE0);
}



//---------------------------------------------------------------------------------------------------
  void CCompiler::CompileLine ( unsigned int _linenr, int _bDoIncludeDebugInfo ) throw(COMPILERERROR)
//---------------------------------------------------------------------------------------------------
{
  char message[50];

  linenr = _linenr;
  bDoIncludeDebugInfo = _bDoIncludeDebugInfo;

  scanpos = 0;
  bInsideIfThenLine = 0;

  while ( !EOL() )
  {
	 if ( bInsideTable )
		ScanTableData();
	 else if ( bInsideSysCode )
		ScanSysCode();
	 else if ( bInsideAsmCode )
		ScanAsmLine();
	 else
	 {
		char* word = ScanWord();

		// DEFINE Zeile
		if ( STREQU(word, KEYWORD_DEFINE) )
		  ScanDefinition();

		// NETWORK Zeile
		else if ( STREQU(word, KEYWORD_NETWORK) )
		  ScanNetworkDefinition();

		// Zeilennummer
		else if ( isdigit(word[0]) )
		{
		  // Zeilennummer bereits definiert
		  if ( labels.Find(word) )
		  {
			 sprintf(message, "%s %s", TXT_REDEFINEDLABEL, word);
			 ThrowError(COMPERR_LABELREDEFINED, message);
		  }

		  DefineLabel(word);
		  if (!	EOL() )
			 ScanStatement();
		}

		// Label
		else if ( (word[0] == '#') && word[1] )
		{
		  if ( labels.Find(word+1) )
		  {
			 sprintf(message, "%s %s", TXT_REDEFINEDLABEL, word);
			 ThrowError(COMPERR_LABELREDEFINED, message);
		  }

		  DefineLabel(word+1);

		  if (!	EOL() )
			 ScanStatement();
		}

		// normales Statement
		else
		  ScanStatement(word);
	 }

	 if ( !TestChar(':') )
		ValidateEOL();
	 else
		ScanChar();
  }
}


//----------------------
  void CCompiler::End ()
//----------------------
{
  PutCode(CODE_END);

  if ( bHasNetAccess )
	 PutGlobalNetCode ();

  FillInLateLabels();

  // WATCH-Symbolliste erstellen
  if ( pWatchables )
  {
	 CAlias* pAlias = vars.GetFirst();

    while ( pAlias )
	 {
	  long variable = pAlias->GetValue();
	  long hSymbol;

		switch ( HIBYTE(variable) )
		{
		 case USERBIT: hSymbol = MakeMapCode(WATCHABLE_BIT, WATCHABLE_VAR, LOBYTE(variable)); break;
		 case USERBYTE: hSymbol = MakeMapCode(WATCHABLE_BYTE, WATCHABLE_VAR, LOBYTE(variable)); break;
		 case USERWORD: hSymbol = MakeMapCode(WATCHABLE_INTEGER, WATCHABLE_VAR, LOBYTE(variable)); break;
		 case USERPORT: hSymbol = MakeMapCode(WATCHABLE_BIT, WATCHABLE_PORT, LOBYTE(variable)); break;
		 case USERBYTEPORT: hSymbol = MakeMapCode(WATCHABLE_BYTE, WATCHABLE_PORT, LOBYTE(variable)); break;
		 case USERWORDPORT: hSymbol = MakeMapCode(WATCHABLE_INTEGER, WATCHABLE_PORT, LOBYTE(variable)); break;
		 case ADPORT: hSymbol = MakeMapCode(WATCHABLE_BYTE, WATCHABLE_AD, LOBYTE(variable)); break;
		 case DAPORT: hSymbol = MakeMapCode(WATCHABLE_BYTE, WATCHABLE_DA, LOBYTE(variable)); break;

		 case SPECIALDATA:
			hSymbol = MakeMapCode(
							(LOBYTE(variable) < FIRST_SPECIALDATA_WORD_INDEX)? WATCHABLE_BYTE: WATCHABLE_INTEGER,
							WATCHABLE_SPECIALDATA, LOBYTE(variable));
			break;
		}

	  pWatchables->Define(pAlias->GetName(), hSymbol);
	  pAlias = vars.GetNext(pAlias);
	 }
  }


  if ( bInsideAsmCode )
	 TerminateAsmCode();
}



//----------------------------------------------------------------------------------------------------
  unsigned long CCompiler::MakeMapCode ( unsigned char range, unsigned char type, unsigned int index )
//----------------------------------------------------------------------------------------------------
{
  unsigned long mapcode;

  mapcode = range;
  mapcode <<= 8;
  mapcode |= type;
  mapcode <<= 16;
  mapcode |= index;

  return mapcode;
}


// *** Verwaltung der am Ende der Compilierung nachzutragenden Sprungmarken ***

LATELABEL* LATELABEL::first = NULL;

//----------------------------------------------------------
  LATELABEL::LATELABEL ( char* _name, unsigned _linenr,
								 unsigned _pos, unsigned _codeindex)
//----------------------------------------------------------
{
  name = strdup(_name);
  linenr = _linenr;
  pos = _pos;
  codeindex = _codeindex;

  next = first;
  first = this;
}

//-----------------------
 LATELABEL::~LATELABEL ()
//-----------------------
{
  free(name);
}

//-----------------------------
  void LATELABEL::DestroyAll ()
//-----------------------------
{
  while ( first )
  {
	 LATELABEL* l = first;
	 first = l->next;
	 delete l;
  }
}


// *** FOR TO STEP Line Stack ***

FORTOLINE* FORTOLINE::last;

//----------------------------------------------------------------------
  FORTOLINE::FORTOLINE ( char* line, long _variable, unsigned _headadr )
//----------------------------------------------------------------------
{
  buf = strdup(line);
  variable = _variable;
  headadr = _headadr;
  prev = last;
  last = this;
}


//------------------------
  FORTOLINE::~FORTOLINE ()
//------------------------
{
  last = prev;
  free(buf);
}

//-----------------------------
  void FORTOLINE::DestroyAll ()
//-----------------------------
{
  while ( last )
  {
	 FORTOLINE* l = last;
	 last = l->prev;
	 delete l;
  }
}
