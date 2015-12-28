/********************************************************************

	Codes für WATCH-bare Ausdrücke (Simulator)

********************************************************************/


#ifndef _WATCHCODE_H
#define _WATCHCODE_H

/*
 
  für jedes WATCH-bare Programmelement (Variable, Konstante) gibt es 
  einen DWORD-Code der wie folgt aufgebaut ist:

          HIWORD       |      LOWORD
    HIBYTE  |  LOBYTE  |

*/



// HIBYTE HIWORD codiert den Wertebereich

#define WATCHABLE_RANGE(dw)	HIBYTE(HIWORD(dw))

#define WATCHABLE_INTEGER	1
#define WATCHABLE_BYTE		2
#define WATCHABLE_BIT		3


// LOBYTE HIWORD codiert den Typ 

#define WATCHABLE_TYPE(dw)	LOBYTE(HIWORD(dw))

#define WATCHABLE_VAR			1
#define WATCHABLE_PORT			2
#define WATCHABLE_AD			3
#define WATCHABLE_DA			4
#define WATCHABLE_SPECIALDATA	5

#define WATCHABLE_INDEX(dw)	LOWORD(dw)
// das LOWORD codiert den Index oder bei WATCHABEL_SPECIALDATA die Systemvariable

#endif
