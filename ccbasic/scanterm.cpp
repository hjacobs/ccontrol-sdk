/********************************************************************

	scanterm.cpp

	Formelparser-Funktionen des Basic-Compilers

********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "compiler.hpp"
#include "68HC05B6/codedefs.h"

#include "comperr.h"
#include "textres.h"
#include "keywords.h"


// mathematische Funktionen

const char* FUNCTION_KEYWORDS[] = { "MIN", "MAX",
												"SQR", "ABS", "SGN",
												"RAND", "RXD", "CTS", "EOF" };
const int FUNCTION_PARAMS[] = { 2, 2,   1, 1, 1,   0, 0, 0, 0 };
const int FUNCTION_CODES[] = { CODE_MIN, CODE_MAX,
										 CODE_ROOT, CODE_ABS, CODE_SGN,
										 CODE_RAND, CODE_ISBYTEAVAIL, CODE_ISCTS, CODE_CHECKEOF };


//----------------------------
  void  CCompiler::ScanTerm ()
//----------------------------
{
  // ein Term liefert ein stets Rechenergebnis
  ValidateNotEOS();
  SplitLogicSum();
}


//--------------------------------
  void CCompiler::SplitLogicSum ()
//--------------------------------
{
  // eine logische Summe ist eine OR-, XOR oder NOR-Verknuepfung
  // logischer Produkte

  // erster Ausdruck
  SplitLogicProduct();

  // nachfolgende Ausdruecke
  while ( !EOS() )
  {
	 unsigned pos = scanpos;
	 int code = ScanOperator();

	 switch ( code )
	 {
		case CODE_OR: case CODE_XOR: case CODE_NOR:

		  SplitLogicProduct();
		  PutCode((unsigned char)code);
		  break;

		default:

		  scanpos = pos;
		  return;
	 }
  }
}


//------------------------------------
  void CCompiler::SplitLogicProduct ()
//------------------------------------
{
  // ein logisches Produkt ist eine AND- oder NAND-Verknuepfung
  // von Vergleichsausdrucken

  // ersten Vergleichsausdruck einlesen
  SplitCompare();

  // nachfolgende Verknuepfungen mit weiteren Vergleichsausdruecken
  while ( !EOS() )
  {
	 unsigned pos = scanpos;
	 int code = ScanOperator();

	 switch ( code )
	 {
		case CODE_AND: case CODE_NAND:
		  SplitCompare();
		  PutCode((unsigned char)code);
		  break;

		default:

		  scanpos = pos;
		  return;
	 }
  }
}


//-------------------------------
  void CCompiler::SplitCompare ()
//-------------------------------
{
  // ein Vergleichsausdruck verknuepft zwei numerische Summenausdruecke
  // und liefert ein dem Vergleichsoperator entsprechendes logisches Ergebnis,
  // vor dem Vergleichsausdruck kann ein NOT stehen

  unsigned pos;
  int Not;

  // auf vorgestelltes NOT pruefen
  pos = scanpos;
  if ( ScanOperator() == CODE_NOT )
	 Not = 1;
  else
  {
	 Not = 0;
	 scanpos = pos;
  }

  // ersten numerischen Summenausdruck einlesen
  SplitSum();

  // den Operator lesen
  pos = scanpos;
  int code = ScanOperator();

  // und auswerten
  switch ( code )
  {
	 case CODE_EQUAL: case CODE_NOTEQUAL: case CODE_LOWER:
	 case CODE_HIGHER: case CODE_LOWERSAME: case CODE_HIGHERSAME:

		// den zweiten numerischen Summenausdruck lesen
		SplitSum();
		PutCode((unsigned char)code);
		break;

	 default:

		// keine Vergleichsoperation
		scanpos = pos;
		break;
  }

  // logisch negieren, falls NOT vorangestellt war
  if ( Not )
	 PutCode(CODE_NOT);
}


//---------------------------
  void CCompiler::SplitSum ()
//---------------------------
{
  // eine Summe verbindet Produktausdruecke additiv

  // erster Sumamnd
  SplitProduct();

  //  weitere Summanden
  while( !EOS() )
  {
	 unsigned pos = scanpos;
	 int code = ScanOperator();

	 switch ( code )
	 {
		case CODE_ADD: case CODE_SUB:

		  SplitProduct();
		  PutCode((unsigned char) code);
		  break;

		default:

		  scanpos = pos;
		  return;
	 }
  }
}


//-------------------------------
  void CCompiler::SplitProduct ()
//-------------------------------
{
  // ein Produkt ist eine multiplikative Verknuepfung von Faktoren,
  // den Faktoren kann ein negatives Vorzeichen "-" vorangestellt sein

  // erster Faktor
  ScanFactor();

  // nachfolgende Faktoren
  while ( !EOS() )
  {
	 unsigned pos = scanpos;
	 int code = ScanOperator();

	 switch ( code )
	 {
		case CODE_MUL: case CODE_DIV: case CODE_MOD: case CODE_SHL: case CODE_SHR:

		  ScanFactor();
		  PutCode((unsigned char)code);
		  break;

		default:

		  scanpos = pos;
		  return;
	 }
  }
}



//-----------------------------------------
  void CCompiler::ScanFactor () throw(COMPILERERROR)
//-----------------------------------------
{
  // ein Faktor ist eine Konstante, ein Funktionsaufruf, eine Variable
  // oder ein Klammerausdruck
  SkipWhites();

  // auf vorangestelltes Minus testen
  unsigned pos = scanpos;
  int neg;

  if ( ScanOperator() == CODE_SUB )
	 neg = 1;
  else
  {
	 neg = 0;
	 scanpos = pos;
  }

  // Klammerausdruck
  if ( TestChar('(') )
  {
	 ScanChar();
	 SplitLogicSum();
	 Validate(ScanChar(), ')');
  }

  else
  {
	 char* word = ScanWord();
	 long value;

	 if ( IsConstant(word, value) )
	 {
		PutCode(CODE_LOADIM);
		PutCode(HIBYTE(value));
		PutCode(LOBYTE(value));
	 }

	 else if ( IsVariable(word, value) )
		PutLoadVarCode (value);

	 else
	 {
		int fx = GetKeywordIndex(FUNCTION_KEYWORDS,
					  sizeof(FUNCTION_KEYWORDS)/sizeof(FUNCTION_KEYWORDS[0]), word);

		if ( -1 != fx )
		{
		  // Funktion ohne Parameter
		  if ( FUNCTION_PARAMS[fx] == 0 )
		  {
			 // wenn optional eine oeffnende Klammer angegeben ist, ...
			 if ( TestChar('(') )
			 {
				ScanChar();
				// ...muss auch eine schliessende folgen
				Validate(ScanChar(), ')');
			 }
		  }
		  // Funktion mit Parametern in runden Klammern
		  else
		  {
			 Validate(ScanChar(), '(');
			 for ( int i = 0; i < FUNCTION_PARAMS[fx]; i++ )
			 {
				if (i)
				  Validate(ScanChar(), ',');
				ScanTerm();
			 }
			 Validate(ScanChar(), ')');
		  }
		  PutCode(FUNCTION_CODES[fx]);
		}
		else if ( STREQU(word, KEYWORD_INITMODEM) )
		  PutInitModemCode();
		else
		{
// Aenderung am 21.11.97 wg Syntaxerweiterung, GOSUB mit Rueckgabewert
		  PutCode(CODE_GOSUB);
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
  }

  if ( neg ) PutCode(CODE_NEG);
}





