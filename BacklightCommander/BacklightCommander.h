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

#ifndef __BacklightCommander__
#define __BacklightCommander__

#define BacklightCommander BacklightCommander

#include <kern/clock.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOTimerEventSource.h>

#ifdef DEBUG_MSG
#define DEBUG_LOG(args...)  IOLog(args)
#else
#define DEBUG_LOG(args...)
#endif

/******************************************************************************
 * UNIX timestamp in second and ACPI methods to evaluate
 ******************************************************************************/
inline UInt32 timer_read_seconds()
{
    clock_sec_t secs;
    clock_usec_t microsecs;
    
    clock_get_calendar_microtime(&secs, &microsecs);
    
    return (UInt32)secs;
}

char kBacklightLevel   [] = "BLVL"; // Backlight Brightness Level Set
char kTimeoutLevel     [] = "BLTO"; // Backlight Timeout X sec/min Set
char kEnableHour       [] = "HENA"; // Hour of day to enable backlight  (>17:00)
char kDisableHour      [] = "HDIS"; // Hour of day to disable backlight (>09:00)

char kGetBacklightLevel[] = "BGET"; // Backlight Brightness Level Get
char kEnableBacklight  [] = "BLON"; // Backlight Enable
char kDisableBacklight [] = "BOFF"; // Backlight Disable
char kSetTimeoutLevel  [] = "TSET"; // Backlight Timeout Level Set

/******************************************************************************
 * Class definitions
 ******************************************************************************/

class BacklightCommander : public IOService
{
    typedef IOService super;
	OSDeclareDefaultStructors(BacklightCommander)

public:
	virtual bool            init(OSDictionary *dictionary = 0);
    virtual                 IOService *probe(IOService *provider, SInt32 *score);
    virtual bool            start(IOService *provider);
	virtual void            stop(IOService *provider);

private:
	IOACPIPlatformDevice*   acpiDevice;
    IOWorkLoop*             fWorkLoop;
	IOTimerEventSource*     fTimer;
    
    // current backlight data
    UInt8                   activeBacklightLevel;
    UInt8                   activeTimeoutLevel;
    
    // local time data
    UInt32                  time;
    int                     secs;
    int                     mins;
    int                     hours;
    UInt32                  tzOffset;
    
    // desired backligh data
    UInt32                  bLevel;
    UInt32                  tLevel;
    UInt32                  hEnable;
    UInt32                  hDisable;
    
    // workloop ontimer action
    IOReturn onTimerAction(void);
    
    UInt32                  evaluateACPIEntry (char *method);
    
    // controls for keyboard backlight
    void                    determineBrightnessLevel(UInt32 backlight);
    void                    determineTimeoutLevel(UInt32 timeout);
    
    bool                    enabled;
    void                    getBacklightLevel();
    void                    setBacklightEnabled();
    void                    setBacklightDisabled();
    void                    setBacklightTimeoutLevel();
    
    void                    getTimeOfDay();
    
};

#endif // __BacklightCommander__