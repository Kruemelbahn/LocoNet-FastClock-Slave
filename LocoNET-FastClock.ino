/*
 * LocoNET-FastClock
 
used I2C-Addresses:
  - 0x20 LCD-Panel
  - 0x3D FastClock-Interface
  - 0x70 FastClock-LED-Display
  
discrete In/Outs used for functionalities:
  -  0    (used  USB)
  -  1    (used  USB)
	-  2
	-  3
	-  4
	-  5
  -  6 Out used   by HeartBeat
  -  7 Out used   by LocoNet [TxD]
  -  8 In  used   by LocoNet [RxD]
  -  9 Out used   by Status-Led (on if minute is odd), not with 4x7-Segment-LED-Display
  - 10
  - 11
  - 12
  - 13
  - 14
  - 15
  - 16
  - 17
  - 18     (used by I2C: SDA)
  - 19     (used by I2C: SCL)

 *************************************************** 
 *  Copyright (c) 2018 Michael Zimmermann <http://www.kruemelsoft.privat.t-online.de>
 *  All rights reserved.
 *
 *  LICENSE
 *  -------
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************
 */

#include <LocoNet.h>    // requested for notifyFastClock
#include "4x7Segment.h"

//=== global stuff =======================================
//#define DEBUG 1   // enables Outputs (debugging informations) to Serial monitor
                  // note: activating SerialMonitor in Arduino-IDE
                  //       will create a reset in software on board!
                  // please comment out also includes in system.ino

//#define DEBUG_MEM 1 // enables memory status on serial port (saves 350Bytes of code :-)

//#define FAST_CLOCK_LOCAL 1  //use local ports for slave clock if no I2C-clock is found.

//#define TELEGRAM_FROM_SERIAL 1  // enables receiving telegrams from SerialMonitor
                                // instead from LocoNet-Port (which is inactive then)

#define LCD     // used in GlobalOutPrint.ino

#define ENABLE_LN             (1)
#define ENABLE_LN_E5          (1)
#define ENABLE_LN_FC_MODUL    (1)
#define ENABLE_LN_FC_LED      (GetCV(ADD_FUNCTIONS_1) & 0x04)
#define ENABLE_LN_FC_INTERN   (GetCV(ADD_FUNCTIONS_1) & 0x08)
#define ENABLE_LN_FC_INVERT   (GetCV(ADD_FUNCTIONS_1) & 0x20)
#define ENABLE_LN_FC_SLAVE    (1)
#define ENABLE_LN_FC_MASTER   (0)
#define ENABLE_LN_FC_JMRI     (GetCV(ADD_FUNCTIONS_1) & 0x10)

#define UNREFERENCED_PARAMETER(P) { (P) = (P); }

#define MANUFACTURER_ID  13   // NMRA: DIY
#define DEVELOPER_ID  58      // NMRA: my ID, should be > 27 (1 = FREMO, see https://groups.io/g/LocoNet-Hackers/files/LocoNet%20Hackers%20DeveloperId%20List_v27.html)

//=== declaration of var's =======================================
#define PIN_LED_MINUTE_ODD        9

uint16_t ui16_TimeMirror;

SevenSegment sevenSeg = SevenSegment();

//========================================================
// give CV a unique name (copy is in CV.ino);
enum { ID_DEVICE, ID_RESERVE_1, ID_RESERVE_2, ID_RESERVE_3, ID_RESERVE_4, ID_RESERVE_5, VERSION_NUMBER, SOFTWARE_ID, ADD_FUNCTIONS_1
#if defined ETHERNET_BOARD
     , IP_BLOCK_3, IP_BLOCK_4
#endif
     };
//========================================================
const __FlashStringHelper *GetSwTitle()
{ 
  return F("LN-FastClock");
}
//========================================================

#include <HeartBeat.h>
HeartBeat oHeartbeat;

//========================================================
void setup()
{
#if defined DEBUG || defined TELEGRAM_FROM_SERIAL
  // initialize serial and wait for port to open:
  Serial.begin(57600);
#endif

  ReadCVsFromEEPROM();
  
  CheckAndInitLCDPanel();

  InitLocoNet();
  
  if(ENABLE_LN_FC_MODUL)
    InitFastClock();

  if(!ENABLE_LN_FC_LED)
    pinMode(PIN_LED_MINUTE_ODD, OUTPUT);

  ui16_TimeMirror = 0xFFFF;
}

void loop()
{
  // light the Heartbeat LED
  oHeartbeat.beat();
  // generate blinken
  Blinken();

  //=== do LCD handling ==============
  // can be connected every time
  // panel only necessary for setup CV's (or some status informations):
  HandleLCDPanel();

  //=== do LocoNet handling ==========
  HandleLocoNetMessages();

	//=== do FastClock handling ===
	HandleFastClock();
  if(ENABLE_LN_FC_LED && !GetFastClockState())
    sevenSeg.showDash();
	
#if defined DEBUG
  #if defined DEBUG_MEM
    ViewFreeMemory();  // shows memory usage
    ShowTimeDiff();    // shows time for 1 cycle
  #endif
#endif
}

/*=== will be called from LocoNetFastClockClass
			if telegram is OPC_SL_RD_DATA [0xE7] or OPC_WR_SL_DATA [0xEF] and clk-state != IDLE ==================*/
void notifyFastClock( uint8_t Rate, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Sync )
{
#if defined DEBUG
  Serial.print(F("notifyFastClock "));
  Serial.print(Hour);
  Serial.print(":");
  decout(Serial, Minute, 2);
  Serial.print("[Rate=");
  Serial.print(Rate);
  Serial.print("][Sync=");
  Serial.print(Sync);
	Serial.println("]");
#endif    
  SetFastClock(Rate, Day, Hour, Minute, Sync);

  // functionalty only with LocoNET-FastClock
  if(!ENABLE_LN_FC_LED)
    digitalWrite(PIN_LED_MINUTE_ODD, (Minute % 2) ? /*Odd*/HIGH : /*Even*/LOW);
  else
  {
    uint16_t time((Hour * 100) + Minute);
    if(ui16_TimeMirror != time)
    {
      sevenSeg.printInt(time);
      ui16_TimeMirror = time;
    }
  }
}

void notifyFastClockFracMins(uint16_t FracMins)
{
  HandleFracMins(FracMins);
}
