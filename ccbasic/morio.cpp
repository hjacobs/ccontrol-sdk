#include <stdio.h>

#include "compiler.hpp"
#include "68HC05B6/codedefs.h"
#include "comperr.h"
#include "textres.h"



static char LABEL_WAKEUP[]	     = "__label_wakeup";
static char LABEL_ONMAIL[]      = "ONMAIL";
static char LABEL_CHECKMAIL[]   = "__label_checkmail";
static char LABEL_NOMAIL[]      = "__label_nomail";
static char LABEL_ONERROR[]     = "__label_onerror";
static char LABEL_SEND3BYTES[]  = "__label_send3bytes";
static char LABEL_SEND6BYTES[]  = "__label_send6bytes";
static char LABEL_IGNORE4BYTES[]  = "__label_ignore4";
static char LABEL_IGNORETHEREST[] = "__label_ignore2";

static const char KEYWORD_SELF[] = "SELF";
static const char KEYWORD_MAILSLOT[] = "MAILSLOT";

const int SLOTSIZE = 5;
const unsigned char WAKEUP = 0xAA;



//-------------------------------------------------------------
  void CCompiler::ScanNetworkDefinition () throw(COMPILERERROR)
//-------------------------------------------------------------
{
  char message[50];

  // self = id [mailslot = bytenr]
  long value;

  // self
  Validate(ScanWord(), KEYWORD_SELF);
  Validate(ScanChar(), '=');

  if ( !IsConstant(ScanWord(), value) )
  {
	 sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
	 ThrowError(COMPERR_CONSTEXPECTED, message);
  }
  morioSelf = (unsigned char) value;

  // optional mailslot
  if ( !EOS() )
  {
	 Validate(ScanWord(), KEYWORD_MAILSLOT);
	 Validate(ScanChar(), '=');

	 if ( !IsConstant(ScanWord(), value) )
	 {
		sprintf(message, "%s %s", TXT_CONSTANT, TXT_EXPECTED);
		ThrowError(COMPERR_CONSTEXPECTED, message);
	 }
	 Validate(value, MAX_USERBYTES-SLOTSIZE, 1L);

	 morioMailSlot = (unsigned char)(value - 1);
	 bDoReceiveMail = 1;
  }
}


//-----------------------------------
  void CCompiler::PutInitModemCode ()
//-----------------------------------
{
  char LABEL_INITFAILED[20];
  MakeLabel(LABEL_INITFAILED);

  bHasNetAccess = 1;

  // WAKEUP an Modem senden, warten, eventuelle Antwort ignorieren;
  // die 5 auf dem Rechenstack wird als WAKEUP-Byte und für das 100ms-Delay
  // benutzt!
  PutCode(CODE_LOADIM); PutCode(0); PutCode(5);        // TOS=5
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_WAIT);
  PutEmptyRXCode();

  // Control-Frame an Modem senden 0-self-self-0-0-0-0-0
  PutCode(CODE_LOADIM); PutCode(0); PutCode(0);
  PutCode(CODE_AND);                                   // beseitigt die 5, TOS=0
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf);// TOS=self
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);                                   // beseitigt self, TOS=0

  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);

  // 100ms warten
  PutCode(CODE_LOADIM); PutCode(0); PutCode(5);        // TOS=5
  PutCode(CODE_WAIT);
  PutCode(CODE_AND);                                   // beseitigt die 5, TOS=0

  // testen, ob ein Antwortbyte kommt, wenn nicht, dann Fehler
  PutCode(CODE_ISBYTEAVAIL);
  PutCode(CODE_OR);                                    // TOS=0 or TOS=FFFF

  PutCode(CODE_IFNOTGOTO); UseForwardLabel(LABEL_INITFAILED);
  PutCode(CODE_NOT);                                   // TOS=0

  // Antwortbyte ignorieren
  PutCode(CODE_GETBYTE); PutCode(CODE_AND);            // TOS=0

  // testen, ob noch ein Antwortbyte kommt, ...
  PutCode(CODE_ISBYTEAVAIL);
  PutCode(CODE_OR);                                    // TOS=0 or TOS=FFFF
  PutCode(CODE_IFNOTGOTO); UseForwardLabel(LABEL_INITFAILED);

  // ... dieses muss gleich self sein
  PutCode(CODE_GETBYTE);
  PutCode(CODE_AND);                                   // TOS=Antwortbyte
  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf);
  PutCode(CODE_EQUAL);

  DefineLabel(LABEL_INITFAILED);
  PutEmptyRXCode();
}


//----------------------------------
  void CCompiler::PutSendMailCode ()
//----------------------------------
{
  char label[20];

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);

  // scan and send mail frame
  // command, receiver address, sender address, x, data1, data2, data3, x
  ScanTerm(); PutCode(CODE_SENDBYTE);
  Validate(ScanChar(), ',');
  ScanTerm(); PutCode(CODE_SENDBYTE);
  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf); PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);

  if ( !EOS() )
  {
	 Validate(ScanChar(), ',');
	 ScanTerm();
  }
  PutCode(CODE_SENDBYTE);

  if ( !EOS() )
  {
	 Validate(ScanChar(), ',');
	 ScanTerm();
  }
  PutCode(CODE_SENDBYTE);

  if ( !EOS() )
  {
	 Validate(ScanChar(), ',');
	 ScanTerm();
  }
  PutCode(CODE_SENDBYTE);

  PutCode(CODE_SENDBYTE);

  // Antwort, erstes Byte = 0 -> Fehler
  PutCode(CODE_GETBYTE);
  PutCode(CODE_LOADIM); PutCode(0); PutCode(0);
  PutCode(CODE_EQUAL);
  PutCode(CODE_IFNOTGOTO); MakeLabel(label); UseForwardLabel(label);

  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_ONERROR);
  PutCode(CODE_GOTO); UseForwardLabel(onerrorlabel);

  DefineLabel(label);

  // alle weiteren Bytes des Antwortframes ignorieren
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_IGNORETHEREST);
}


//---------------------------------------
  void CCompiler::PutGetMailCode ( void )
//---------------------------------------
{
  // nur für explizite Mail-Abfrage per GETMAIL-Befehl
  char message[50];

  if ( !bDoReceiveMail )
  {
	 sprintf(message, "%s", TXT_NOMAILSLOT);
	 ThrowError(COMPERR_NOMAILSLOT, message);
  }

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup, it returns 0 on TOS
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);

  // send command-0-frame
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);

  PutCode(CODE_GETBYTE);
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_IGNORETHEREST);
}


//----------------------------------------------
  void CCompiler::PutGosubCheckMailCode ( void )
//----------------------------------------------
{
  // siehe Ende von ScanStatement (compscan.cpp)
  if ( bDoReceiveMail )
  {
	 PutCode(CODE_GOSUB); UseForwardLabel(LABEL_CHECKMAIL);
  }
}


//--------------------------------------------------------------
  void CCompiler::PutGetRemotePortCode ( unsigned char remoteid,
													  unsigned char mask )
//--------------------------------------------------------------
{
  char label[20];

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup, it returns 0 on TOS
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);

  // send command-1-frame
  PutCode(CODE_LOADIM); PutCode(0); PutCode(1);         // command 1
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);                                    // simple pop, TOS=0

  PutCode(CODE_LOADIM); PutCode(0); PutCode(remoteid);  // remote id
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf); // self-x-x
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_SEND3BYTES);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(mask);      // mask-x-x
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_SEND3BYTES);

  // Antwort, erstes Byte = 0 -> Fehler
  PutCode(CODE_GETBYTE);
  PutCode(CODE_EQUAL);                                  // TOS = (byte==0)?
  PutCode(CODE_IFNOTGOTO); MakeLabel(label); UseForwardLabel(label);

  // Fehler
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_ONERROR);
  PutCode(CODE_GOTO); UseForwardLabel(onerrorlabel);

  // weiter, wenn kein Fehler
  DefineLabel(label);

  PutCode(CODE_NOT);                                    // TOS FFFF->0
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_IGNORE4BYTES);

  PutCode(CODE_GETBYTE);
  PutCode(CODE_AND);                                   // TOS=0

  // Daten lesen und auf Rechenstack bringen
  PutCode(CODE_GETBYTE);                                // data
  PutCode(CODE_OR);                                     // TOS=data

  PutCode(CODE_LOADIM); PutCode(0); PutCode(0);
  PutCode(CODE_GETBYTE);
  PutCode(CODE_AND);
  PutCode(CODE_OR);

  if ( mask != 0xFF )
  {
	 PutCode(CODE_IFNOTGOTO); MakeLabel(label); UseForwardLabel(label);

	 PutCode(CODE_LOADIM); PutCode(0xFF); PutCode(0xFF);  // set to FFFF
	 PutCode(CODE_OR);

	 DefineLabel(label);
  }
}


//--------------------------------------------------------------
  void CCompiler::PutSetRemotePortCode ( unsigned char remoteid,
													  unsigned char mask )
//--------------------------------------------------------------
{
  char label[20];

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup, it returns 0 on TOS
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);

  // send command-2-frame
  PutCode(CODE_LOADIM); PutCode(0); PutCode(2);         // command 2
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);                                    // simple pop, TOS=0

  PutCode(CODE_LOADIM); PutCode(0); PutCode(remoteid);  // remote id
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf);      // self-x-x
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_SEND3BYTES);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(mask);      // mask
  PutCode(CODE_SENDBYTE);                               //
  PutCode(CODE_AND);

  PutCode(CODE_OR);
  PutCode(CODE_SENDBYTE);                               // data

  PutCode(CODE_SENDBYTE);                               // x

  // Antwort, erstes Byte = 0 -> Fehler
  PutCode(CODE_GETBYTE);
  PutCode(CODE_LOADIM); PutCode(0); PutCode(0);
  PutCode(CODE_IFNOTGOTO); MakeLabel(label); UseForwardLabel(label);

  // Fehler
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_ONERROR);
  PutCode(CODE_GOTO); UseForwardLabel(onerrorlabel);

  // weiter, wenn kein Fehler
  DefineLabel(label);

  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_IGNORETHEREST);
}


//--------------------------------------------------------------
  void CCompiler::PutDeactRemotePortCode ( unsigned char remoteid,
														 unsigned char mask )
//--------------------------------------------------------------
{
  char label[20];

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup, it returns 0 on TOS
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);

  // send command-3-frame
  PutCode(CODE_LOADIM); PutCode(0); PutCode(3);         // command 3
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);                                    // simple pop, TOS=0

  PutCode(CODE_LOADIM); PutCode(0); PutCode(remoteid);  // remote id
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf);      // self-x-x
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_SEND3BYTES);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(mask);      // mask
  PutCode(CODE_SENDBYTE);                               //
  PutCode(CODE_AND);

  PutCode(CODE_OR);
  PutCode(CODE_SENDBYTE);                               // data

  PutCode(CODE_SENDBYTE);                               // x

  // Antwort, erstes Byte = 0 -> Fehler
  PutCode(CODE_GETBYTE);
  PutCode(CODE_LOADIM); PutCode(0); PutCode(0);
  PutCode(CODE_IFNOTGOTO); MakeLabel(label); UseForwardLabel(label);

  // Fehler
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_ONERROR);
  PutCode(CODE_GOTO); UseForwardLabel(onerrorlabel);

  // weiter, wenn kein Fehler
  DefineLabel(label);

  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_IGNORETHEREST);
}




//---------------------------------------------------------------------
  void CCompiler::PutGetRemote12BitADPortCode ( unsigned char remoteid,
																unsigned char index )
//---------------------------------------------------------------------
{
  char label[20];

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);


  // 12bit ad0 -> CMD 04
  if ( index )
  {
  }
  // 12bit ad1 -> CMD 05
  else
  {
  }
}


//----------------------------------------------------------------
  void CCompiler::PutGetRemoteADPortCode ( unsigned char remoteid,
														 unsigned char index )
//----------------------------------------------------------------
{
  char label[20];

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);

  // Kommando-06-Frame senden
  PutCode(CODE_LOADIM); PutCode(0); PutCode(6);         // command 6
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(remoteid);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf);
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_SEND6BYTES);

  // Antwort, erstes Byte = 0 -> Fehler
  PutCode(CODE_GETBYTE);
  PutCode(CODE_EQUAL);
  PutCode(CODE_IFNOTGOTO); MakeLabel(label); UseForwardLabel(label);

  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_ONERROR);
  PutCode(CODE_GOTO); UseForwardLabel(onerrorlabel);

  DefineLabel(label);

  // Bytes 1...3 des Antwortframes ignorieren
  PutCode(CODE_NOT);
  PutCode(CODE_GETBYTE); PutCode(CODE_AND);
  PutCode(CODE_GETBYTE); PutCode(CODE_AND);

  // entsprechenden AD-Wert aus dem Antwortframe filtern
  switch ( index )
  {
	 case 0:
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE);                                // data
		PutCode(CODE_OR);                                     // TOS=data
		PutCode(CODE_LOADIM);  PutCode(0); PutCode(0);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_OR);
	 break;

	 case 1:
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE);                                // data
		PutCode(CODE_OR);                                     // TOS=data
		PutCode(CODE_LOADIM);  PutCode(0); PutCode(0);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_OR);
	 break;

	 case 2:
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE);                                // data
		PutCode(CODE_OR);                                     // TOS=data
		PutCode(CODE_LOADIM);  PutCode(0); PutCode(0);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_OR);
	 break;

	 case 3:
		PutCode(CODE_GETBYTE);                                // data
		PutCode(CODE_OR);                                     // TOS=data
		PutCode(CODE_LOADIM);  PutCode(0); PutCode(0);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_GETBYTE); PutCode(CODE_AND);
		PutCode(CODE_OR);
	 break;
  }
}


//----------------------------------------------------------------
  void CCompiler::PutGetRemoteStateCode ( unsigned char remoteid )
//----------------------------------------------------------------
{
  char label[20];

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);

  // Kommando-07-Frame senden
  PutCode(CODE_LOADIM); PutCode(0); PutCode(7);         // command 7
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(remoteid);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf);
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_SEND6BYTES);

  // Antwort, erstes Byte = 0 -> Fehler
  PutCode(CODE_GETBYTE);
  PutCode(CODE_EQUAL);
  PutCode(CODE_IFNOTGOTO); MakeLabel(label); UseForwardLabel(label);

  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_ONERROR);
  PutCode(CODE_GOTO); UseForwardLabel(onerrorlabel);

  DefineLabel(label);

  // Bytes 1...5 des Antwortframes ignorieren
  PutCode(CODE_NOT);
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_IGNORE4BYTES);

  PutCode(CODE_GETBYTE);
  PutCode(CODE_AND);                                   // TOS=0

  // Daten lesen und auf Rechenstack bringen
  PutCode(CODE_GETBYTE);                                // data
  PutCode(CODE_OR);                                     // TOS=data

  PutCode(CODE_LOADIM); PutCode(0); PutCode(0);
  PutCode(CODE_GETBYTE);
  PutCode(CODE_AND);
  PutCode(CODE_OR);
}


//---------------------------------------------------------------
  void CCompiler::PutSetRemoteStateCode ( unsigned char remoteid,
														unsigned char mask )
//---------------------------------------------------------------
{
  char label[20];

  bHasNetAccess = 1;
  bHasNetAccessInThisStatement = 1;

  // wakeup
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_WAKEUP);

  // Kommando-08-Frame senden
  PutCode(CODE_LOADIM); PutCode(0); PutCode(8);         // command 8
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);                                    // simple pop, TOS=0

  PutCode(CODE_LOADIM); PutCode(0); PutCode(remoteid);  // remote id
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(morioSelf);      // self-x-x
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_SEND3BYTES);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(mask);      // mask
  PutCode(CODE_SENDBYTE);                               //
  PutCode(CODE_AND);

  PutCode(CODE_OR);
  PutCode(CODE_SENDBYTE);                               // data

  PutCode(CODE_SENDBYTE);                               // x

  // Antwort, erstes Byte = 0 -> Fehler
  PutCode(CODE_GETBYTE);
  PutCode(CODE_LOADIM); PutCode(0); PutCode(0);
  PutCode(CODE_IFNOTGOTO); MakeLabel(label); UseForwardLabel(label);

  // Fehler
  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_ONERROR);
  PutCode(CODE_GOTO); UseForwardLabel(onerrorlabel);

  // weiter, wenn kein Fehler
  DefineLabel(label);

  PutCode(CODE_GOSUB); UseForwardLabel(LABEL_IGNORETHEREST);
}


// *** Funktionen, die globale Netzwerk-Unterfunktionen erzeugen ***

//--------------------------------
  void CCompiler::PutWakeUpCode ()
//--------------------------------
{
  DefineLabel(LABEL_WAKEUP);

  // load 0 (TOS=0) for later simple AND-pops and answer check
  PutCode(CODE_LOADIM); PutCode(0); PutCode(0);

  PutCode(CODE_LOADIM);  PutCode(0);   PutCode(2);
  PutCode(CODE_WAIT);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM); PutCode(0); PutCode(WAKEUP);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  // Antwort in MailSlot speichern ***

  // byte 1: command
  PutCode(CODE_GETBYTE);
  if ( bDoReceiveMail )
  {
	 PutCode(CODE_STOREBYTE);
	 PutCode(morioMailSlot);
  }
  PutCode(CODE_AND);

  // byte 2: ignore
  PutCode(CODE_GETBYTE);
  PutCode(CODE_AND);

  // byte 3: sender
  PutCode(CODE_GETBYTE);
  if ( bDoReceiveMail )
  {
	 PutCode(CODE_STOREBYTE);
	 PutCode(morioMailSlot+1);
  }
  PutCode(CODE_AND);

  // byte 4: ignore
  PutCode(CODE_GETBYTE);
  PutCode(CODE_AND);

  // byte 5: mail data 1
  PutCode(CODE_GETBYTE);
  if ( bDoReceiveMail )
  {
	 PutCode(CODE_STOREBYTE);
	 PutCode(morioMailSlot+2);
  }
  PutCode(CODE_AND);

  // byte 6: mail data 2
  PutCode(CODE_GETBYTE);
  if ( bDoReceiveMail )
  {
	 PutCode(CODE_STOREBYTE);
	 PutCode(morioMailSlot+3);
  }
  PutCode(CODE_AND);

  // byte 7: mail data 3
  PutCode(CODE_GETBYTE);
  if ( bDoReceiveMail )
  {
	 PutCode(CODE_STOREBYTE);
	 PutCode(morioMailSlot+4);
  }
  PutCode(CODE_AND);

  // byte 8: ignore
  PutCode(CODE_GETBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_LOADIM);  PutCode(0);   PutCode(2);
  PutCode(CODE_WAIT);
  PutCode(CODE_AND);

  PutCode(CODE_RETURN);
}


//-----------------------------------
  void CCompiler::PutCheckMailCode ()
//-----------------------------------
{
  DefineLabel(LABEL_CHECKMAIL);

  // command = 0 -> kein Mail
  PutCode(CODE_LOADBYTE); PutCode(morioMailSlot);
  PutCode(CODE_IFNOTGOTO); UseForwardLabel(LABEL_NOMAIL);

  PutCode(CODE_GOTO); UseForwardLabel(LABEL_ONMAIL);

  DefineLabel(LABEL_NOMAIL);

  PutCode(CODE_RETURN);
}


//---------------------------------
  void CCompiler::PutOnErrorCode ()
//---------------------------------
{
  DefineLabel(LABEL_ONERROR);

  if ( bDoReceiveMail )
  {
	 PutCode(CODE_GOSUB); UseForwardLabel(LABEL_IGNORETHEREST);
	 PutCode(CODE_GOTO); UseForwardLabel(LABEL_CHECKMAIL);
  }
  else
  {
	 PutCode(CODE_GOTO); UseForwardLabel(LABEL_IGNORETHEREST);
  }
}


//-----------------------------------
  void CCompiler::PutSend3BytesCode()
//-----------------------------------
{
  // assumes 0 on TOS!

  DefineLabel(LABEL_SEND3BYTES);

  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_RETURN);
}


//-----------------------------------
  void CCompiler::PutSend6BytesCode()
//-----------------------------------
{
  // assumes 0 on TOS!

  DefineLabel(LABEL_SEND6BYTES);

  PutCode(CODE_SENDBYTE); PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE); PutCode(CODE_SENDBYTE);
  PutCode(CODE_SENDBYTE); PutCode(CODE_SENDBYTE);
  PutCode(CODE_AND);

  PutCode(CODE_RETURN);
}


//--------------------------------------------
  void CCompiler::PutIgnore4BytesCode ( void )
//--------------------------------------------
{
  // assumes 0 on TOS!
  DefineLabel(LABEL_IGNORE4BYTES);

  PutCode(CODE_GETBYTE); PutCode(CODE_AND);
  PutCode(CODE_GETBYTE); PutCode(CODE_AND);
  PutCode(CODE_GETBYTE); PutCode(CODE_AND);
  PutCode(CODE_GETBYTE); PutCode(CODE_AND);

  PutCode(CODE_RETURN);
}


//---------------------------------------------
  void CCompiler::PutIgnoreTheRestCode ( void )
//---------------------------------------------
{
  DefineLabel(LABEL_IGNORETHEREST);

  PutCode(CODE_GETBYTE); PutCode(CODE_GETBYTE);
  PutCode(CODE_GETBYTE); PutCode(CODE_GETBYTE);
  PutCode(CODE_GETBYTE); PutCode(CODE_GETBYTE);
  PutCode(CODE_GETBYTE);

  PutCode(CODE_RETURN);
}


//----------------------------------------
 void CCompiler::PutGlobalNetCode ( void )
//----------------------------------------
{
  PutWakeUpCode();
  PutOnErrorCode();
  PutSend3BytesCode();
  PutSend6BytesCode();
  PutIgnore4BytesCode();
  PutIgnoreTheRestCode();

  if ( bDoReceiveMail )
	 PutCheckMailCode();
}
