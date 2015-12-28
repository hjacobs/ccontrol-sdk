/********************************************************************

	ccbasic.cpp

********************************************************************/

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>

#include "68HC05B6/codedefs.h"
#include "compiler.hpp"

#include "comperr.h"
#include "textres.h"


const char OUTFILE_EXTENSION[] = "dat";

char sourcepath[256] = "";
char infilenamedup[256] = "";

CCompiler compiler(sourcepath);

//------------------------------------
  int main ( int argc, char * argv[] )
//------------------------------------
{
  // copyright
  cout << TXT_WELCOME;
  cout << TXT_COPYRIGHT;

  // kein Quellfile angegeben
  if ( argc < 2 )
  {
	 cout << TXT_USAGE;
	 return FATAL_USAGE;
  }

  // Quellfile oeffnen
  char* pInFileName = argv[1];

  ifstream is(pInFileName);

  if ( is.fail() )
  {
	 cout << TXT_CANTREAD;
	 return FATAL_CANTREAD;
  }

  strcpy(infilenamedup, pInFileName);

  char* pLastBackSlash = strrchr(infilenamedup, '\\');
  strcpy(sourcepath, ".");

  if ( pLastBackSlash )
  {
	 *pLastBackSlash = 0;
	 strcpy(sourcepath, infilenamedup);
  }


  // Quelltext zeilenweise uebersetzen
  unsigned linenr = 1;

  while ( !is.eof() )
  {
	 memset(compiler.GetLineBuffer(), 0, MAX_LINELEN);
	 is.getline(compiler.GetLineBuffer(), MAX_LINELEN);

	 try
	 {
		compiler.CompileLine(linenr++);
	 }
	 catch (COMPILERERROR& e)
	 {		switch ( e.id )
		{
		  case COMPERR_PROGRAMTOOLONG:
		  case COMPERR_SYSPROGRAMTOOLONG:
			 cout << TXT_LINE << " "
					<< e.linenr << ": " << e.msg << '\n';
			 cout << compiler.GetErrCount() << " " << TXT_ERRORSFOUND << '\n';
			 return HAS_ERRORS;

		  default:
			 cout << TXT_LINE << " "
					<< e.linenr << ": " << e.msg << '\n';
			 break;
		}
	 }
  }

  try
  {
	 compiler.End();
  }
  catch (COMPILERERROR& e)
  {
	 cout << e.linenr << ": " << e.msg << '\n';
  }
  is.close();

  // wenn keine Compiler-Fehler, dann Codes in DAT-File schreiben
  if ( compiler.IsOK() )
  {
	 // Ausgabefilename mit DAT-Extension
	 char* pOutFileName;
	 char* pExtension;

	 pOutFileName = new char[strlen(pInFileName) +
									 strlen(OUTFILE_EXTENSION)	+ 2];
	 strcpy(pOutFileName, pInFileName);

	 pExtension = strrchr(pOutFileName, '.');
	 if ( pExtension )
		strcpy(pExtension + 1, OUTFILE_EXTENSION);
	 else
	 {
		strcat(pOutFileName, ".");
		strcat(pOutFileName, OUTFILE_EXTENSION);
	 }

	 // Ausgabefile oeffnen
	 ofstream os(pOutFileName);

	 if ( os.fail() )
	 {
		is.close();
		cout << TXT_CANTWRITE;
		return FATAL_CANTWRITE;
	 }

	 os << "CCTRL-BASIC";
	 unsigned i;

	 // BASIC-Codes
	 os << '\n' << compiler.GetCodeLen()  << '\n';
	 for ( i = 0; i < compiler.GetCodeLen(); i++ )
	 {
		os << (int)compiler.GetCode(i) << ' ';

		if ( 31 == (i % 32) )
		  os << '\n';

		if ( os.fail() )
		{
		  cout << TXT_CANTWRITE;
		  is.close();
		  os.close();
		  return FATAL_CANTWRITE;
		}
	 }

	 // SYS-Codes
	 os << '\n'  << compiler.GetSysCodeLen() << '\n';
	 for ( i = 0; i < compiler.GetSysCodeLen(); i++ )
	 {
		os << (int)compiler.GetSysCode(i) << ' ';

		if ( 31 == (i % 32) )
		  os << '\n';

		if ( os.fail() )
		{
		  cout << TXT_CANTWRITE;
		  is.close();
		  os.close();
		  return FATAL_CANTWRITE;
		}
	 }

	 os.close();

//	 if ( argc > 2 )
//	 {
//		cout << TXT_CALLINGLOADER;
//TODO: finish ccdownload
//		execl("ccupload", pOutFileName);
//	 }
//	 else
		cout << compiler.GetCodeLen() << " BASIC Byte(s), "
			  << compiler.GetSysCodeLen() << " SYS Byte(s) " << TXT_READY;

	 delete pOutFileName;
  }
  else
  {
	 cout << compiler.GetErrCount() << " " << TXT_ERRORSFOUND << '\n';
	 return HAS_ERRORS;
  }

  return 0;
}

