//----------------------------------------------------------------------
// Copyright 2011 Ciaran McHale.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions.
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.  
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//----------------------------------------------------------------------

#ifndef HAPTICOMM_CONFIGURATION_H_
#define HAPTICOMM_CONFIGURATION_H_

#include <config4cpp/Configuration.h>
#include "device.h"
#include "waveform.h"
#include "alphabet.h"
using namespace CONFIG4CPP_NAMESPACE;

class HaptiCommConfigurationException
{
public:
    //--------
    // Constructors and destructor
    //--------
    HaptiCommConfigurationException(const char * str);
    HaptiCommConfigurationException(const HaptiCommConfigurationException & other);
    ~HaptiCommConfigurationException();

    const char * c_str() const; // Accessor

private:
    char * m_str;

    //--------
    // Not implemented
    //--------
    HaptiCommConfigurationException();
    HaptiCommConfigurationException operator=(const HaptiCommConfigurationException &);
};


class HaptiCommConfiguration
{
public:
    //--------
    // Constructor and destructor
    //--------
    HaptiCommConfiguration();
    HaptiCommConfiguration(const char * cfgSource);
    ~HaptiCommConfiguration();

    void parse(const char * cfgSource, const char * scope = "") 
        throw (HaptiCommConfigurationException);
    
    
    //--------
    // Configuration functions.
    //--------
    void configure(const char * cfgSource, DEVICE * dev, WAVEFORM * wf, ALPHABET * alph)
        throw (HaptiCommConfigurationException);
    
    void configureDevice(DEVICE * dev)
        throw (HaptiCommConfigurationException);
    
    void configureWaveform(WAVEFORM * wf)
        throw (HaptiCommConfigurationException);
    
    void configureAlphabet(ALPHABET * alph, DEVICE * dev, WAVEFORM * wf)
        throw (HaptiCommConfigurationException);
   
private: 
    void initVariableMove(struct variableMove * vm);
    
    //--------
    // Lookup-style functions.
    //--------
    struct actuator lookupActuator(const char * scopeActuator) 
        const throw (HaptiCommConfigurationException);
    
    const char * lookupActuatorID(const char * scopeActuator) 
        const throw (HaptiCommConfigurationException);
    
    struct motion lookupMotion(const char * scopeMotion) 
        throw (HaptiCommConfigurationException);
    
    struct symbol lookupSymbol(const char * scopeSymbol) 
        throw (HaptiCommConfigurationException);

    void lookupList(const char * name, const char **& array, int & arraySize) 
        const throw (HaptiCommConfigurationException);

    virtual int lookupInt(const char * name) 
        const throw(HaptiCommConfigurationException);
    
    virtual float lookupFloat(const char * name) 
        const throw(HaptiCommConfigurationException);
    
    virtual bool lookupBoolean(const char * name) 
        const throw(HaptiCommConfigurationException);
    
    virtual int lookupDurationMilliseconds(const char * name) 
        const throw(HaptiCommConfigurationException);
    
    virtual int lookupDurationSeconds(const char * name) 
        const throw(HaptiCommConfigurationException);

    //--------
    // Instance variables
    //--------
    CONFIG4CPP_NAMESPACE::Configuration *   m_cfg;
    StringBuffer                            m_cfgSource;
    StringBuffer                            m_scope;
    StringVector                            m_scopeNames;

    //--------
    // The following are not implemented
    //--------
    HaptiCommConfiguration(const HaptiCommConfiguration &);
    HaptiCommConfiguration & operator=(const HaptiCommConfiguration &);
};


#endif
