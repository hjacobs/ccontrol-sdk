#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "68HC05B6/codedefs.h"
#include "compiler.hpp"


//  Operator-Keywords
const char* OPERATOR_KEYWORDS[] = { "NOT", "AND", "NAND", "OR", "NOR", "XOR",
											  "MOD", "SHL", "SHR" };
const int OPERATOR_CODES[] = { CODE_NOT, CODE_AND, CODE_NAND, CODE_OR, CODE_NOR,
										 CODE_XOR, CODE_MOD, CODE_SHL, CODE_SHR };


// *** Basic Scans ***

//-----------------------------
  void CCompiler::SkipWhites ()
//-----------------------------
{
  while ( line[scanpos] )
  {
	 if ( line[scanpos] > ' ' )
		break;

	 scanpos++;
  }
}

//-------------------------------------------
  char CCompiler::ScanChar ( int skipwhites )
//-------------------------------------------
{
  if (skipwhites) SkipWhites();
  return line[scanpos++];
}


//----------------------------
  char* CCompiler::ScanWord ()
//----------------------------
{
  static char word[MAX_WORDLEN];

  SkipWhites();

  int i = 0;

  if (  TestChar('&') || TestChar('-') )
  {
	 word[i++] = toupper(line[scanpos++]);
  }

  while ( isalnum(line[scanpos]) || TestChar('_') || TestChar('#') )
  {
	 word[i++] = toupper(line[scanpos++]);
  }

  word[i] = 0;

  return word;
}

//------------------------------
  int CCompiler::ScanOperator ()
//------------------------------
{
  SkipWhites();
  unsigned pos = scanpos;

  if ( isalpha(line[scanpos]) )
  {
	 int codeindex = GetKeywordIndex(OPERATOR_KEYWORDS, sizeof(OPERATOR_KEYWORDS)/sizeof(OPERATOR_KEYWORDS[0]), ScanWord());
	 if ( codeindex != -1 )
		return OPERATOR_CODES[codeindex];
  }

  else
  {
	 switch ( ScanChar() )
	 {
		 case '+': return CODE_ADD;
		 case '-': return CODE_SUB;
		 case '*': return CODE_MUL;
		 case '/': return CODE_DIV;

		 case '=':

			if ( TestChar('>') )
			{ ScanChar(); return CODE_HIGHERSAME; }

			else if ( TestChar('<') )
			{ ScanChar(); return CODE_LOWERSAME; }

			else
			  return CODE_EQUAL;

		case '<':

			if ( TestChar('>') )
			{ ScanChar(); return CODE_NOTEQUAL; }

			else if ( TestChar('=') )
			{ ScanChar(); return CODE_LOWERSAME; }

			else
			  return CODE_LOWER;

		case '>':

		  if ( TestChar('<') )
		  { ScanChar(); return CODE_NOTEQUAL; }

		  else if ( TestChar('=') )
		  { ScanChar(); return CODE_HIGHERSAME; }

		  else
			 return CODE_HIGHER;
	 }
  }

  scanpos = pos;
  return -1;
}
