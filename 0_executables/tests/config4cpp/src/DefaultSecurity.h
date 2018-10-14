//----------------------------------------------------------------------
// WARNING: This file was generated by config2cpp-nocheck. Do not edit.
//
// Description: a class providing access to an embedded
//              configuration string.
//----------------------------------------------------------------------
#ifndef DefaultSecurity_h
#define DefaultSecurity_h

#include <config4cpp/Configuration.h>


namespace CONFIG4CPP_NAMESPACE {


class DefaultSecurity
{
public:
	//--------
	// Constructor and destructor.
	//--------
	DefaultSecurity();
	~DefaultSecurity();

	//--------
	// Get the configuration string
	//--------
	const char * getString()
	{
		return m_str.c_str();
	}

private:
	//--------
	// Variables
	//--------
	CONFIG4CPP_NAMESPACE::StringBuffer m_str;

	//--------
	// The following are not implemented
	//--------
	DefaultSecurity & operator=(const DefaultSecurity &);
	DefaultSecurity(const DefaultSecurity &);
};


}; // namespace CONFIG4CPP_NAMESPACE


#endif
