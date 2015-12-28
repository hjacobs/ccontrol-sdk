/********************************************************************


	alias.cpp

********************************************************************/

#include <stdlib.h>
#include <string.h>

#include "alias.hpp"

// *** CAlias ***

//-----------------
  CAlias::CAlias ()
//-----------------
{
  name = NULL;
  next = NULL;
}


//------------------
  CAlias::~CAlias ()
//------------------
{
  if ( name )
    free(name);
}


//----------------------------------------------------------------
  void CAlias::Define ( const char* _name, const long int _value )
//----------------------------------------------------------------
{
  if ( _name )
    name = strdup(_name);

  value = _value;
}



// *** CAliasList ***

//-------------------------
  CAliasList::CAliasList ()
//-------------------------
{
  first = NULL;
}

//--------------------------
  CAliasList::~CAliasList ()
//--------------------------
{
  Empty();
}

//-------------------------------
  void CAliasList::Empty ( void )
//-------------------------------
{
  while ( first )
  {
	 CAlias* next = first->next;
	 delete first;
	 first = next;
  }
}

//--------------------------------------------------------------------
  void CAliasList::Define ( const char* _name, const long int _value )
//--------------------------------------------------------------------
{
  CAlias* pAlias = new CAlias();
  pAlias->Define(_name, _value);

  pAlias->next = first;
  first = pAlias;
}


//-----------------------------------------------
	 CAlias* CAliasList::Find ( const char* name )
//-----------------------------------------------
{
  CAlias* pAlias = first;

  while ( pAlias )
  {
	 if ( strcmp(pAlias->name, name) == 0 )
	  return pAlias;

	 pAlias = pAlias->next;
  }

  return NULL;
}


//-------------------------------
  CAlias* CAliasList::GetFirst ()
//-------------------------------
{
  return first;
}


//----------------------------------------------
  CAlias* CAliasList::GetNext ( CAlias* pAlias )
//----------------------------------------------
{
  return pAlias = pAlias->next;
}
