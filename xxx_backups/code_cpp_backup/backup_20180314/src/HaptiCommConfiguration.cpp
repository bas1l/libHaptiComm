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

#include "HaptiCommConfiguration.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
using CONFIG4CPP_NAMESPACE::Configuration;
using CONFIG4CPP_NAMESPACE::ConfigurationException;

using namespace std;

//----------------------------------------------------------------------
// class HaptiCommConfigurationException
//----------------------------------------------------------------------

HaptiCommConfigurationException::HaptiCommConfigurationException(const char * str)
{
    m_str = new char[strlen(str) + 1];
    strcpy(m_str, str);
}


HaptiCommConfigurationException::HaptiCommConfigurationException(const HaptiCommConfigurationException & other)
{
    m_str = new char[strlen(other.m_str) + 1];
    strcpy(m_str, other.m_str);
}


HaptiCommConfigurationException::~HaptiCommConfigurationException()
{
    delete [] m_str;
}


const char * HaptiCommConfigurationException::c_str() const
{
    return m_str;
}



//----------------------------------------------------------------------
// class HaptiCommConfiguration
//----------------------------------------------------------------------

HaptiCommConfiguration::HaptiCommConfiguration()
{
    m_cfg = Configuration::create();
}


HaptiCommConfiguration::HaptiCommConfiguration(const char * cfgSource)
{
    m_cfg = Configuration::create();
    m_cfgSource = cfgSource;
}



HaptiCommConfiguration::~HaptiCommConfiguration()
{
    m_cfg->destroy();
}


void HaptiCommConfiguration::parse(const char * cfgSource, const char * scope) 
throw (HaptiCommConfigurationException)
{
    //Configuration * cfg = (Configuration *)m_cfg;

    m_scope = scope;
    try {
            if (cfgSource != 0 && strcmp(cfgSource, "") != 0) {
                m_cfgSource = cfgSource;
                m_cfg->parse(cfgSource);
            }
        //m_cfg->setFallbackConfiguration(m_cfg);
        //ConfigurationImpl::setFallbackConfiguration(Configuration * cfg)
        //cfg->setFallbackConfiguration(Configuration::INPUT_STRING, Configuration::getString());
        //cfg->setFallbackConfiguration(Configuration::INPUT_STRING, FallbackConfiguration::getString());
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
}

void HaptiCommConfiguration::configureWaveform(WAVEFORM * wf)
throw (HaptiCommConfigurationException)
{
    StringBuffer __m_scope = m_scope;
    
    StringBuffer scope; 
    Configuration::mergeNames(m_scope.c_str(), "waveform", scope);
    StringBuffer filter;
    
    m_scope = scope;
    try {
        Configuration::mergeNames(scope.c_str(), "tap", filter);
        int tapDuration = (int) m_cfg->lookupInt(filter.c_str(), "duration");
        
        Configuration::mergeNames(scope.c_str(), "apparent", filter);
        int appActSuperposed = (int) m_cfg->lookupInt(filter.c_str(), "nb_act_superposed");
        float appRatioCover = (float) m_cfg->lookupFloat(filter.c_str(), "ratio_covering");
        
        Configuration::mergeNames(scope.c_str(), "apparent.asc", filter);
        int appAscDuration = (int) m_cfg->lookupInt(filter.c_str(), "duration");
        
        Configuration::mergeNames(scope.c_str(), "apparent.action", filter);
        int appActionDuration = (int) m_cfg->lookupInt(filter.c_str(), "duration");
        int appActionAmplitude = (int) m_cfg->lookupInt(filter.c_str(), "amplitude");
        
        wf->configure(tapDuration, appActSuperposed, appRatioCover, 
                      appAscDuration, appActionDuration, appActionAmplitude);
    }
    catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
    
    m_scope = __m_scope;
}

//--------
// Configuration functions.
//--------
void HaptiCommConfiguration::configureDevice(DEVICE * dev)
throw (HaptiCommConfigurationException)
{
    StringBuffer __m_scope = m_scope;
    
    StringBuffer scope; 
    Configuration::mergeNames(m_scope.c_str(), "device.actuators", scope);
    StringBuffer filter;
    const char * id;
    int len;
    int i;
    
    m_scope = scope;
    try {
        Configuration::mergeNames(scope.c_str(), "uid-actuator", filter);
        m_cfg->listFullyScopedNames(m_scope.c_str(), "", Configuration::CFG_SCOPE,
                                    false, filter.c_str(), m_scopeNames);
        len = m_scopeNames.length();
        printf("There are %d actuators\n", len);
        for (i = 0; i < len; i++) {
            
            id = lookupActuatorID(m_scopeNames[i]);
            struct actuator ac = lookupActuator(m_scopeNames[i]);
            
            dev->setActuator(ac, id);
        }
    }
    catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
    
    m_scope = __m_scope;
}





//--------  
// Lookup-style functions.
//--------

struct actuator HaptiCommConfiguration::lookupActuator(const char * scopeActuator) 
const throw (HaptiCommConfigurationException){
    
    StringBuffer scopeActions;
    struct actuator ac = {};
    
    try {
        StringBuffer windingStr = m_cfg->lookupString(scopeActuator, "windingDirection");
        ac.windingDirection = (int8_t) (strcmp(windingStr.c_str(),"anticlockwise")?+1:-1); // /!\ if(0) => same string !
        ac.chan = (uint8_t) m_cfg->lookupInt(scopeActuator, "dacChannel");
        ac.name = m_cfg->lookupString(scopeActuator, "name");
        
        Configuration::mergeNames(scopeActuator, "actionValues", scopeActions);
        ac.vneutral = (uint16_t) m_cfg->lookupInt(scopeActions.c_str(), "neutral");
        ac.vpush = (uint16_t) m_cfg->lookupInt(scopeActions.c_str(), "push");
        ac.vup = (uint16_t) m_cfg->lookupInt(scopeActions.c_str(), "up");
    
        
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
    
    return ac;
}



const char * HaptiCommConfiguration::lookupActuatorID(const char * scopeActuator) 
const throw (HaptiCommConfigurationException)
{
    try {
                return m_cfg->lookupString(scopeActuator, "id");
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
}






void HaptiCommConfiguration::lookupList(const char * name, const char **& array, int & arraySize)
const throw (HaptiCommConfigurationException)
{
    //Configuration * cfg = (Configuration *)m_cfg;
    try {
            m_cfg->lookupList(m_scope.c_str(), name, array, arraySize);
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
}



int HaptiCommConfiguration::lookupInt(const char * name) 
const throw (HaptiCommConfigurationException)
{
    //Configuration * cfg = (Configuration *)m_cfg;
    try {
            return m_cfg->lookupInt(m_scope.c_str(), name);
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
}



float HaptiCommConfiguration::lookupFloat(const char * name) 
const throw (HaptiCommConfigurationException)
{
    //Configuration * cfg = (Configuration *)m_cfg;
    try {
            return m_cfg->lookupFloat(m_scope.c_str(), name);
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
}



bool HaptiCommConfiguration::lookupBoolean(const char * name) 
const throw (HaptiCommConfigurationException)
{
    //Configuration * cfg = (Configuration *)m_cfg;
    try {
            return m_cfg->lookupBoolean(m_scope.c_str(), name);
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
}



int HaptiCommConfiguration::lookupDurationMilliseconds(const char * name) 
const throw (HaptiCommConfigurationException)
{
    //Configuration * cfg = (Configuration *)m_cfg;
    try {
            return m_cfg->lookupDurationMilliseconds(m_scope.c_str(), name);
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
}



int HaptiCommConfiguration::lookupDurationSeconds(const char * name) 
const throw (HaptiCommConfigurationException)
{
    //Configuration * cfg = (Configuration *)m_cfg;
    try {
            return m_cfg->lookupDurationSeconds(m_scope.c_str(), name);
    } catch(const ConfigurationException & ex) {
            throw HaptiCommConfigurationException(ex.c_str());
    }
}

