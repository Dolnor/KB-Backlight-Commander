/*
 *  Released under "The GNU General Public License (GPL-2.0)"
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but 
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along 
 *  with this program; if not, write to the Free Software Foundation, Inc., 
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <IOKit/IOCommandGate.h>
#include "BacklightCommander.h"

OSDefineMetaClassAndStructors(BacklightCommander, IOService)

/******************************************************************************
 * BacklightCommander::init
 ******************************************************************************/
bool BacklightCommander::init(OSDictionary *dict)
{
    if (!super::init(dict))
        return false;
    
    DEBUG_LOG("BacklightCommander: commander initializing\n");

    acpiDevice = NULL;
    fWorkLoop = NULL;
    fTimer = NULL;
    
    // get GMT time offset from plist
    tzOffset = 0; // default to GMT0
    if (OSNumber* zone = OSDynamicCast(OSNumber, getProperty("GMT")) ) {
        tzOffset = zone->unsigned32BitValue();
        DEBUG_LOG("BacklightCommander: plist config --> time zone GMT:%d\n", tzOffset);
    }
    
    return true;
}

/******************************************************************************
 * BacklightCommander::probe
 ******************************************************************************/
IOService *BacklightCommander::probe(IOService *provider, SInt32 *score)
{
    IOService *result = super::probe(provider, score);
    DEBUG_LOG("BacklightCommander: commander probing\n");
    return result;
}

/******************************************************************************
 * BacklightCommander::start
 ******************************************************************************/
bool BacklightCommander::start(IOService *provider)
{
    //set designated device as provider
    acpiDevice = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (NULL == acpiDevice || !super::start(provider))
        return false;
    
    DEBUG_LOG("BacklightCommander: commander version 0.1.0 starting\n");
    
    // get backlight level and timeout level from ACPI
    bLevel = evaluateACPIEntry(kBacklightLevel);
    tLevel = evaluateACPIEntry(kTimeoutLevel);
    DEBUG_LOG("BacklightCommander: acpi config --> backlight level %d, timeout level %d\n", bLevel, tLevel);

    // get enable and disable hours from ACPI
    hEnable = evaluateACPIEntry(kEnableHour);
    hDisable = evaluateACPIEntry(kDisableHour);
    DEBUG_LOG("BacklightCommander: acpi config --> enable past %02d:00, disable past %02d:00\n", hEnable, hDisable);
    
    // set ACPI-obtained properties to IOReg
    setProperty("ACPI: Backlight Level",   bLevel,  32);
    setProperty("ACPI: Backlight Timeout", tLevel,  32);
    setProperty("ACPI: Enable Backlight After",  hEnable, 32);
    setProperty("ACPI: Disable Backlight After", hDisable,32);
    
    // sleep thread for 3 second to prevent getting incorrect levels
    IOSleep(3000);
    // set desired timeout level
    setBacklightTimeoutLevel();
    // now determine if backlight is enabled or disabled at start
    getBacklightLevel();

    // setup workloop and timer
    fWorkLoop = IOWorkLoop::workLoop();
    fTimer = IOTimerEventSource::timerEventSource(this,
        OSMemberFunctionCast(IOTimerEventSource::Action, this, &BacklightCommander::onTimerAction));
    
	if (!fWorkLoop || !fTimer)
        stop(provider);
    if (fWorkLoop->addEventSource(fTimer) != kIOReturnSuccess)
        stop(provider);
    
    fTimer->setTimeoutMS(1000);
    DEBUG_LOG("BacklightCommander: workloop started\n");
    
	this->registerService(0);
    return true;
}

/******************************************************************************
 * BacklightCommander::stop
 ******************************************************************************/
void BacklightCommander::stop(IOService *provider)
{
    DEBUG_LOG("BacklightCommander: commander stopping\n");
    if (fWorkLoop)
    {
        if(fTimer){
            OSSafeRelease(fTimer);
        }
        OSSafeRelease(fWorkLoop);
    }
	
    super::stop(provider);
}

/******************************************************************************
 * BacklightCommander::getTimeOfDay
 ******************************************************************************/
void BacklightCommander::getTimeOfDay()
{
    time = timer_read_seconds();
    
    if (time != 0) {
        secs =  time%60;
        time /= 60;
        mins =  time%60;
        time /= 60;
        hours = time%24 + tzOffset;
        time /= 24;
        
        //DEBUG_LOG("BacklightCommander: time is hh:%02d mm:%02d ss:%02d\n",hours,mins,secs);
    }
}

/******************************************************************************
 * BacklightCommander::onTimerAction
 ******************************************************************************/
IOReturn BacklightCommander::onTimerAction()
{
    // get current backlight level
    getBacklightLevel();
   
    // get time of day and determine if backlight should be enabled or disabled
    // X - disable hour, Y - enable hour
    getTimeOfDay();
    if ((hours >= hEnable  || // if past Y:00
        hours <  hDisable) && // or before X:00
        !enabled) {
        if (activeBacklightLevel != bLevel) // if backlight level is not already the same as we have requested
            setBacklightEnabled();
    }
    
    if (hours >= hDisable && // if past X:00
        hours < hEnable   && // and before Y:00
        enabled) {
        if (activeBacklightLevel != 0x00) // if backlight is not already disabled
            setBacklightDisabled();
    }

    if (NULL != fTimer)
    {
        fTimer->cancelTimeout();
        //fTimer->setTimeoutMS(5000);// for testing
        fTimer->setTimeoutMS(5*60*1000); // set timer to 5min (in ms) - check 12x per hour
    }
   
    return kIOReturnSuccess;
}

/******************************************************************************
 * BacklightCommander::evaluateACPIEntry
 ******************************************************************************/
UInt32 BacklightCommander::evaluateACPIEntry (char *method)
{
    
    OSObject  *object = NULL;
    UInt32     result = NULL;
    
    if (method) {
        if (kIOReturnSuccess == acpiDevice->evaluateObject(method, &object) && object) {
            if (OSNumber  *level = OSDynamicCast(OSNumber, object)) {
                result = (UInt32)level->unsigned32BitValue();
            }
            //DEBUG_LOG("BacklightCommander: method %s returned value %d\n",method,result);
            
            return result;
        }
    }
    return 0;
}

/******************************************************************************
 * BacklightCommander::get+setBacklight & setBacklightTimeoutLevel
 ******************************************************************************/
void BacklightCommander::getBacklightLevel()
{
    activeBacklightLevel = evaluateACPIEntry(kGetBacklightLevel);
    if(activeBacklightLevel != 0x00) {
        if (activeBacklightLevel > bLevel)
            DEBUG_LOG("BacklightCommander: backlight level higher than requested, will not change\n");
        enabled = true;
    }
    else
        enabled = false;
}

void BacklightCommander::setBacklightEnabled()
{
    activeBacklightLevel = evaluateACPIEntry(kEnableBacklight);
    IOLog("BacklightCommander: backlight level changed to ");
    determineBrightnessLevel(activeBacklightLevel);
    /*
    if (activeBacklightLevel == 0x01 || activeBacklightLevel == 0x02)
        enabled = true;
     */
}

void BacklightCommander::setBacklightDisabled()
{
    activeBacklightLevel = evaluateACPIEntry(kDisableBacklight);
    IOLog("BacklightCommander: backlight level changed to ");
    determineBrightnessLevel(activeBacklightLevel);
    /*
    if (activeBacklightLevel == 0x00)
        enabled = false;
     */
}

void BacklightCommander::setBacklightTimeoutLevel()
{
    activeTimeoutLevel = evaluateACPIEntry(kSetTimeoutLevel);
    determineTimeoutLevel(activeTimeoutLevel);
}

/******************************************************************************
 * BacklightCommander::determineBrightnessLevel & determineTimeoutLevel
 ******************************************************************************/
void BacklightCommander::determineBrightnessLevel(UInt32 backlight)
{
    switch (backlight) {
        case 0x00:
            IOLog("disabled\n");
            break;
        case 0x01:
            IOLog("dim\n");
            break;
        case 0x02:
            IOLog("bright\n");
            break;
    }
}

void BacklightCommander::determineTimeoutLevel(UInt32 timeout)
{
    IOLog("BacklightCommander: backlight timeout level set to ");
    switch (timeout) {
        case 0x00:
            IOLog("never off\n");
            break;
        case 0x01:
            IOLog("5 sec\n");
            break;
        case 0x03:
            IOLog("15 sec\n");
            break;
        case 0x06:
            IOLog("30 sec\n");
            break;
        case 0x0C:
            IOLog("1 min\n");
            break;
        case 0x3C:
            IOLog("5 min\n");
            break;
        case 0xB4:
            IOLog("15 min\n");
            break;
    }
}
