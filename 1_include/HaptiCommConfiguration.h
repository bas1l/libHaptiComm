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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#include <config4cpp/Configuration.h>
#pragma GCC diagnostic pop

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

    void parse(const char * cfgSource, const char * scope = "");
    
    //--------
    // Configuration functions.
    //--------
    void configure(const char * cfgSource, DEVICE * dev, WAVEFORM * wf, ALPHABET * alph);
    
    void configureDevice(DEVICE * dev);
    
    void configureWaveform(WAVEFORM * wf);
    
    void configureAlphabet(ALPHABET * alph, DEVICE * dev, WAVEFORM * wf);
    
private: 
    void initVariableMove(struct variableMove * vm);
    
    //--------
    // Lookup-style functions.
    //--------
    struct actuator lookupActuator(const char * scopeActuator);
    
    const char * lookupActuatorID(const char * scopeActuator);
    
    struct motion lookupMotion(const char * scopeMotion);
    
    struct symbol lookupSymbol(const char * scopeSymbol);

    void lookupList(const char * name, const char **& array, int & arraySize);

    virtual int lookupInt(const char * name);
    
    virtual float lookupFloat(const char * name);
    
    virtual bool lookupBoolean(const char * name);
    
    virtual int lookupDurationMilliseconds(const char * name);
    
    virtual int lookupDurationSeconds(const char * name);

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
