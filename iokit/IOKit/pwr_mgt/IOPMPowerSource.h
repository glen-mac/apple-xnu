/*
 * Copyright (c) 1998-2005 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_OSREFERENCE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code 
 * as defined in and that are subject to the Apple Public Source License 
 * Version 2.0 (the 'License'). You may not use this file except in 
 * compliance with the License.  The rights granted to you under the 
 * License may not be used to create, or enable the creation or 
 * redistribution of, unlawful or unlicensed copies of an Apple operating 
 * system, or to circumvent, violate, or enable the circumvention or 
 * violation of, any terms of an Apple operating system software license 
 * agreement.
 *
 * Please obtain a copy of the License at 
 * http://www.opensource.apple.com/apsl/ and read it before using this 
 * file.
 *
 * The Original Code and all software distributed under the License are 
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * Please see the License for the specific language governing rights and 
 * limitations under the License.
 *
 * @APPLE_LICENSE_OSREFERENCE_HEADER_END@
 */
 
#ifndef _IOPMPowerSource_h_
#define _IOPMPowerSource_h_

#include <libkern/c++/OSObject.h>
#include <IOKit/pwr_mgt/IOPM.h>
#include <IOKit/IOTypes.h>
#include <IOKit/IOReturn.h>
#include <IOKit/IOService.h>

enum {
    kSecondsPerHour = 3600,
    kTenMinutesInSeconds = 600
};

/* class IOPMPowerSource
 *
 * See IOKit/pwr_mgt/IOPM.h for power source keys relevant to this class. These
 * report-type keys are required for calls to IOPMPowerSource::setReportables(),
 * and they define the IORegistry interface through which data is passed back
 * up to the rest of the system.
 *
 * A subclassing driver that doesn't want to do anything fancy should:
 *   1. Subclass IOPMPowerSource
 *   3. Install its own battery change notifications or polling routine that can
 *          converse with actual battery hardware.
 *   4. When battery state changes, change the relevant member variables
 *          through setCurrentCapacity() style accessors.
 *   5. Call updateStatus() on itself when all such settings have been updated.
 *   
 * The subclass driver should also initially populate its settings and call
 * updateStatus() on launch.
 *
 *
 * Settings
 *
 * ExternalConnected
 * Type: bool
 * IORegistry Key: kIOPMPSExternalConnectedKey
 * True if computer is drawing external power 
 *
 * ExternalChargeCapable
 * Type: bool
 * IORegistry Key: kIOPMPSExternalChargeCapableKey
 * True if external power is capable of charging internal battery
 *
 * BatteryInstalled
 * Type: bool
 * IORegistry Key: kIOPMPSBatteryInstalledKey
 * True if a battery is present; false if removed
 *
 * IsCharging
 * Type: bool
 * IORegistry Key: kIOPMPSIsChargingKey
 * True if battery is charging itself from external power
 *
 * AtWarnLevel
 * Type: bool
 * IORegistry Key: kIOPMPSAtWarnLevelKey
 * True if draining battery capacity and past warn level
 *
 * AtCriticalLevel
 * Type: bool
 * IORegistry Key: kIOPMPSAtCriticalLevelKey
 * True if draining battery capacity and past critical level
 *
 * CurrentCapacity
 * MaxCapacity
 * Type: unsigned int
 * IORegistry Key: kIOPMPSCurrentCapacityKey, kIOPMPSMaxCapacityKey
 * Capacity measured in mAh
 *
 * TimeRemaining
 * Type: int
 * IORegistry Key: kIOPMPSTimeRemainingKey
 * Time remaining measured in minutes
 *
 * Amperage
 * Type: int
 * IORegistry Key: kIOPMPSAmperageKey
 * Current is measured in mA
 *
 * Voltage
 * Type: unsigned int
 * IORegistry Key: kIOPMPSVoltageKey
 * Voltage measured in mV
 *
 * CycleCount
 * Type: unsigned int
 * IORegistry Key: kIOPMPSCycleCountKey
 * Number of charge/discharge cycles
 *
 * AdapterInfo
 * Type: int
 * IORegistry Key: kIOPMPSAdapterInfoKey
 * Power adapter information
 *
 * Location
 * Type: int
 * IORegistry Key: kIOPMPSLocationKey
 * Clue about battery's location in machine - Left vs. Right
 *
 * ErrorCondition
 * Type: OSSymbol
 * IORegistry Key: kIOPMPSErrorConditionKey
 * String describing error state of battery
 *
 * Manufacturer
 * Type: OSSymbol
 * IORegistry Key: kIOPMPSManufacturerKey
 * String describing battery manufacturer
 *
 * Model
 * Type: OSSymbol
 * IORegistry Key: kIOPMPSModelKey
 * String describing model number
 *
 * Serial
 * Type: OSSymbol
 * IORegistry Key: kIOPMPSSerialKey
 * String describing serial number  or unique info
 *
 * LegacyIOBatteryInfo
 * Type: OSDictionary
 * IORegistry Key: kIOPMPSLegacyBatteryInfoKey
 * Dictionary conforming to the OS X 10.0-10.4 
 */

class IOPMPowerSource : public IOService
{
    OSDeclareDefaultStructors(IOPMPowerSource)

    friend class IOPMPowerSourceList;

 protected:
    // Tracking for IOPMPowerSourceList
    IOPMPowerSource         *nextInList;
    
    OSDictionary            *properties;

    const OSSymbol *externalConnectedKey;
    const OSSymbol *externalChargeCapableKey;
    const OSSymbol *batteryInstalledKey;
    const OSSymbol *chargingKey;
    const OSSymbol *warnLevelKey;
    const OSSymbol *criticalLevelKey;
    const OSSymbol *currentCapacityKey;
    const OSSymbol *maxCapacityKey;
    const OSSymbol *timeRemainingKey;
    const OSSymbol *amperageKey;
    const OSSymbol *voltageKey;
    const OSSymbol *cycleCountKey;
    const OSSymbol *adapterInfoKey;
    const OSSymbol *locationKey;
    const OSSymbol *errorConditionKey;
    const OSSymbol *manufacturerKey;
    const OSSymbol *modelKey;
    const OSSymbol *serialKey;
    const OSSymbol *batteryInfoKey;

 public:

/*! @function powerSource
    @abstract Creates a new IOPMPowerSource nub. Must be attached to IORegistry,
        and registered by provider.
*/
    static IOPMPowerSource *powerSource(void);

    virtual bool init(void);
    
    virtual void free(void);

/*! @function updateStatus
    @abstract Must be called by physical battery controller when battery state
                has changed significantly.
    @discussion The system will not poll this object for battery updates. Rather \
    the battery's controller must call updateStatus() every time state changes \
    and the settings will be relayed to higher levels of power management. \
    The subclassing driver should override this only if the driver needs to add \
    new settings to the base class.
*/
    virtual void updateStatus(void);

/* Public accessors for battery state
 */
    bool externalConnected(void);
    bool externalChargeCapable(void);
    bool batteryInstalled(void);
    bool isCharging(void);
    bool atWarnLevel(void);
    bool atCriticalLevel(void);

    unsigned int currentCapacity(void);
    unsigned int maxCapacity(void);
    unsigned int capacityPercentRemaining(void);
    int timeRemaining(void);
    int amperage(void);
    unsigned int voltage(void);
    unsigned int cycleCount(void);
    int adapterInfo(void);
    int location(void);
    
    OSSymbol *errorCondition(void);
    OSSymbol *manufacturer(void);
    OSSymbol *model(void);
    OSSymbol *serial(void);
    OSDictionary *legacyIOBatteryInfo(void);
    
protected:
/* Protected "setter" methods for subclasses
 * Subclasses should use these setters to modify all battery properties.
 * 
 * Subclasses must follow all property changes with a call to updateStatus() 
 * to flush settings changes to upper level battery API clients.
 *
 */
    void setExternalConnected(bool);
    void setExternalChargeCapable(bool);
    void setBatteryInstalled(bool);
    void setIsCharging(bool);
    void setAtWarnLevel(bool);
    void setAtCriticalLevel(bool);

    void setCurrentCapacity(unsigned int);
    void setMaxCapacity(unsigned int);    
    void setTimeRemaining(int);
    void setAmperage(int);    
    void setVoltage(unsigned int);
    void setCycleCount(unsigned int);
    void setAdapterInfo(int);
    void setLocation(int);

    void setErrorCondition(OSSymbol *);
    void setManufacturer(OSSymbol *);
    void setModel(OSSymbol *);
    void setSerial(OSSymbol *);
    void setLegacyIOBatteryInfo(OSDictionary *);
    
};

#endif
