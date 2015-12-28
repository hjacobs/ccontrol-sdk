/********************************************************************

  compiler.hpp

********************************************************************/

#ifndef _COMPILER_HPP
#define _COMPILER_HPP

#include <string.h>
#include <fstream>

using namespace std;

#pragma warning(disable: 4290)

#include "alias.hpp"


#ifndef HIWORD
  #define HIWORD(dw)   ((unsigned short)(((unsigned long)(dw) >> 16) & 0xFFFF))
#endif

#ifndef LOWORD
  #define LOWORD(dw)   ((unsigned short)(dw))
#endif

#ifndef HIBYTE
  #define HIBYTE(w)  ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))
#endif

#ifndef LOBYTE
  #define LOBYTE(w)  ((unsigned char)(w))
#endif


//#define MAX_CODELEN   8100
#define MAX_CODELEN   20000


#define MAX_LINELEN   1024
#define MAX_WORDLEN  256
#define MAX_ERRMSG  512
#define MAX_SYSCODELEN 255
#define STREQU(s1,s2) (strcmp(s1,s2) == 0)

enum DATATYPES { USERBIT, USERBYTE, USERWORD, USERPORT, USERBYTEPORT,
					  USERWORDPORT, ADPORT, DAPORT, SPECIALDATA,

					  // neu ab Juni 98, Netzwerkunterstützung
					  REMOTE12BITADPORT, REMOTEADPORT, REMOTEPORT, REMOTESTATE };


//---------------------
  struct COMPILERERROR
//---------------------
{
	 unsigned linenr;
	 unsigned id;
	 char msg[256];

	 COMPILERERROR( unsigned _linenr, unsigned _id, char* message = 0 );
};



//----------------
  struct LATELABEL
//----------------
{
  public:

	 char* name;
	 unsigned linenr;
    unsigned pos;
	 unsigned codeindex;

	 LATELABEL* next;

	 static LATELABEL* first;

	 LATELABEL ( char* _name, unsigned _linenr, unsigned _pos, unsigned _codeindex);
	~LATELABEL ();

	 static void DestroyAll ();
};

//----------------
  struct FORTOLINE
//----------------
{
	char* buf;
	unsigned headadr;
	long variable;

	FORTOLINE* prev;

	static FORTOLINE* last;

	FORTOLINE ( char* line, long _variable, unsigned _headadr );
  ~FORTOLINE ();

	static void DestroyAll ();
};

//---------------
  class CCompiler
//---------------
{
	 CAliasList constants;
	 CAliasList vars;
	 CAliasList labels;
	 CAliasList* pWatchables;
	 CAliasList* pValidBreakpoints;

	 unsigned char codebuf[MAX_CODELEN];
	 unsigned codeindex;
	 unsigned linenr;
	 int bDoIncludeDebugInfo;

	 unsigned char syscodebuf[MAX_SYSCODELEN];
	 unsigned syscodeindex;

	 unsigned errcount;
	 void ThrowError( unsigned id, char* message );

	 int bInsideTable: 1;
	 int bInsideSysCode: 1;
	 int bInsideIfThenLine: 1;
	 int bInsideAsmCode: 1;
	 int bDoGenDebugCode: 1;
	 int bHasNetAccess: 1;
	 int bHasNetAccessInThisStatement: 1;
	 int bDoReceiveMail: 1;
//	 int bDoRetry: 1;

	 int nextByteIndex;
	 int nextWordIndex;

	 unsigned helper_label_count;

	 char line[MAX_LINELEN];
	 int  scanpos;

	 ofstream* pAsmStream;

	char* pSourcePath;

	// morio network stuff
	 unsigned char morioSelf;
	 char onerrorlabel[20];
	 int morioMailSlot;



	 // Grundlegende Scanoperationen
	 void SkipWhites ();
	 char  ScanChar ( int skipwhites = 1 );
	 char* ScanWord ();
	 int 	 ScanOperator ();

	 // "Strenge" Testfunktionen
	 void  Validate ( char scan, char c ) throw(COMPILERERROR);
	 void  Validate ( char* word, const char* keyword ) throw(COMPILERERROR);
	 void  Validate ( long value, long hibound = 0, long lobound = 0) throw(COMPILERERROR);
	 void  ValidateEOL () throw(COMPILERERROR);
	 void  ValidateNotEOL () throw(COMPILERERROR);
	 void  ValidateEOS () throw(COMPILERERROR);
	 void  ValidateNotEOS () throw(COMPILERERROR);

	 // Pruef- und Umwandlungsfunktionen
	 int   IsIdentifier ( char* word );
	 int 	 IsConstant ( char* word, long& result );
	 int   IsVariable ( char* word, long& result );


	 // Komplexe Scans
	 void ScanDefinition () throw(COMPILERERROR);
	 void ScanNetworkDefinition () throw(COMPILERERROR);
	 void ScanTableData () throw(COMPILERERROR);
	 void ScanTableFromFile ( char* filename ) throw(COMPILERERROR);
	 void ScanSysCode () throw(COMPILERERROR);
	 void ScanSysCodeFromS19File ( char* filename ) throw(COMPILERERROR);
	 void	ScanAsmLine();
	 void TerminateAsmCode ();

	 void ScanStatement ( char* word = NULL ) throw(COMPILERERROR);
	 void ScanInstruction ( char* word = NULL );
	 long ScanAssignment ( char* word = NULL) throw(COMPILERERROR);

	 // Formelparserfunktionen
	 void ScanTerm ();
	 void SplitLogicSum ();
	 void SplitLogicProduct ();
	 void SplitCompare ();
	 void SplitSum ();
	 void SplitProduct ();
	 void ScanFactor () throw(COMPILERERROR);

	 // Toolfunktionen
	  char NextChar ();
	 int TestWord ( const char* word );
	 int TestChar ( char c );
	 int EOL ();
	 int EOS ();
	 int GetKeywordIndex ( const char** table, int numentries, char* word );
	 void PutCode ( unsigned char code ) throw(COMPILERERROR);
	 void PutLoadVarCode ( long variable );
	 void PutStoreVarCode( long variable );
	 void PutSysCode ( unsigned char code ) throw(COMPILERERROR);
	 void PutLineInfoCode ( void );
	 void PutNumPopCode ( void );
	 void PutEmptyRXCode ( void );

	 // morio network code
	 void PutInitModemCode ( void );
	 void PutSendMailCode ( void );
	 void PutGetMailCode ( void );
	 void PutGosubCheckMailCode ( void );
	 void PutGetRemotePortCode ( unsigned char remoteid, unsigned char mask );
	 void PutSetRemotePortCode ( unsigned char remoteid, unsigned char mask );
	 void PutDeactRemotePortCode ( unsigned char remoteid, unsigned char mask );
	 void PutGetRemote12BitADPortCode ( unsigned char remoteid, unsigned char index );
	 void PutGetRemoteADPortCode ( unsigned char remoteid, unsigned char index );
	 void PutGetRemoteStateCode ( unsigned char remoteid );
	 void PutSetRemoteStateCode ( unsigned char remoteid, unsigned char mask );
	 void PutGlobalNetCode ( void );
		void PutCheckMailCode ( void );
		void PutWakeUpCode ( void );
		void PutOnErrorCode ( void );
		void PutSend3BytesCode();
		void PutSend6BytesCode();
		void PutIgnore4BytesCode ( void );
		void PutIgnoreTheRestCode ( void );

	 void MakeLabel ( char* label );
	 void UseForwardLabel ( char* label );
	 void DefineLabel ( char* label );
	 void FillInLateLabels () throw(COMPILERERROR);
	 int IsLabelUsed ( char* label );
	 unsigned int GetCodeAdr ();
	 void TestRemarkChar () throw(int);

  public:

	 CCompiler ( char* _pSourcePath );
	~CCompiler ();

	 void SetWatchablesList ( CAliasList* pList );
	 void SetValidBreakpointsList ( CAliasList* pList );
	 char* GetLineBuffer ();
	 void CompileLine ( unsigned int _linenr, int bDoIncludeDebugInfo = 0 ) throw(COMPILERERROR);
	 void End ();

	 unsigned GetErrCount ();
	 int IsOK ();

	 unsigned GetCodeLen ();
	 unsigned GetSysCodeLen() { return syscodeindex; }
	 int GetCode ( int i );
	 int GetSysCode ( int i );

	 static unsigned long MakeMapCode ( unsigned char range, unsigned char type, unsigned int index );
};

inline char* CCompiler::GetLineBuffer ()
{ return line; }

inline int CCompiler::TestChar ( char c )
{ return (NextChar() == c); }

inline int CCompiler::EOL ()
{ return (TestChar(39) || TestChar(0)); }

inline int CCompiler::EOS ()
{ return (EOL() || TestChar(':')); }

inline unsigned CCompiler::GetErrCount ()
{ return errcount; }

inline int CCompiler::IsOK ()
{ return (errcount == 0); }


inline unsigned CCompiler::GetCodeLen ()
{ return codeindex; }

inline int CCompiler::GetCode ( int i )
{ return (int) codebuf[i]; }

inline int CCompiler::GetSysCode ( int i )
{ return (int) syscodebuf[i]; }

inline void CCompiler::SetWatchablesList ( CAliasList* pList )
{ pWatchables = pList; }

inline void CCompiler::SetValidBreakpointsList ( CAliasList* pList )
{ pValidBreakpoints = pList; }


extern const char* KEYWORD_ON;
extern const char* KEYWORD_OFF;

extern const char ASMFILENAME[];
extern const char S19FILENAME[];
extern const char ASMCALL[];


extern const int MAX_USERBYTES;
extern const int MAX_USERWORDS;
extern const int MAX_USERBITS;
extern const int MAX_USERBYTEPORTS;
extern const int MAX_USERPORTS;
extern const int MAX_USERWORDPORTS;
extern const int MAX_ADPORTS;
extern const int MAX_DAPORTS;
extern const int MAX_AD12PORTS;


#endif
