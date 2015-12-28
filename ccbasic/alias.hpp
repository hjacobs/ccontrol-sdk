/********************************************************************

	alias.hpp

********************************************************************/

#ifndef _ALIAS_HPP
#define _ALIAS_HPP

class CAlias;
class CAliasList;

//------------
  class CAlias
//------------
{
	 friend class CAliasList;

	 CAlias* next;
	 long int     value;
	 char*   name;

	 void Define ( const char* _name, const long int _value );

  public:

	 CAlias();
	~CAlias();

	long int GetValue () { return value; }
	char* GetName () { return name; }
};


//----------------
  class CAliasList
//----------------
{
	 CAlias* first;

  public:

	 CAliasList ();
	~CAliasList ();

   void Empty ( void );

	 void Define ( const char* _name, const long int _value );
	 CAlias* Find ( const char* name );

	 CAlias* GetFirst ();
	 CAlias* GetNext ( CAlias*  pAlias );
};

#endif
