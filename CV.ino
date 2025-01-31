//=== CV-Routines for LocoNET-UhrTaktgeber ===
#include <EEPROM.h>

// declared in LocoNET-UhrTaktgeber.ino
// enum { ID_DEVICE, ID_RESERVE_1, ID_RESERVE_2, ID_RESERVE_3, ID_RESERVE_4, ID_RESERVE_5, VERSION_NUMBER, SOFTWARE_ID, ADD_FUNCTIONS_1 };

//=== declaration of var's =======================================
#define PRODUCT_ID SOFTWARE_ID
static const uint8_t DEVICE_ID = 1;				// CV1: Device-ID
static const uint8_t SW_VERSION = 8;			// CV7: Software-Version
static const uint8_t FASTCLOCK_LN = 9;    // CV8: Software-ID

#if defined ETHERNET_BOARD
  static const uint8_t MAX_CV = 11;
#else
  static const uint8_t MAX_CV = 9;
#endif

uint8_t ui8a_CV[MAX_CV] = { UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX
#if defined ETHERNET_BOARD
                          , UINT8_MAX, UINT8_MAX
#endif
                          };  // ui8a_CV is a copy from eeprom

struct _CV_DEF // uses 6 Byte of RAM for each CV
{
	uint8_t ID;
	uint8_t DEFLT;
	uint8_t MIN;
	uint8_t MAX;
	boolean BINARY;
	boolean RO;
};

const struct _CV_DEF cvDefinition[MAX_CV] =
{ // ID               default value        minValue							maxValue							binary     r/o
	 { ID_DEVICE,       DEVICE_ID,           1,										126,									false,     true }   // normally r/o
	,{ ID_RESERVE_1,    0,                   0,									  0,										false,     true }   // (unused)
	,{ ID_RESERVE_2,    0,                   0,									  0,										false,     true }   // (unused)
	,{ ID_RESERVE_3,    0,                   0,									  0,										false,     true }   // (unused)
	,{ ID_RESERVE_4,    0,                   0,									  0,										false,     true }   // (unused)
	,{ ID_RESERVE_5,    0,                   0,									  0,										false,     true }   // (unused)
	,{ VERSION_NUMBER,  SW_VERSION,          0,										SW_VERSION,						false,     false }  // normally r/o
	,{ SOFTWARE_ID,     FASTCLOCK_LN,        FASTCLOCK_LN,        FASTCLOCK_LN,         false,     true }   // always r/o
	,{ ADD_FUNCTIONS_1, 0x07,                0,										UINT8_MAX, 						true,      false }  // additional functions 1
#if defined ETHERNET_BOARD
	,{ IP_BLOCK_3,      2,                   0,										UINT8_MAX,						false,     false }  // IP-Address part 3
	,{ IP_BLOCK_4,      106,                 0,										UINT8_MAX,						false,     false }  // IP-Address part 4
#endif
};

//=== naming ==================================================
const __FlashStringHelper *GetCVName(uint8_t ui8_cv)
{ 
  // each string should have max. 10 chars
  const __FlashStringHelper *cvName[MAX_CV] = { F("DeviceID"),
                                                F("Reserve"),
                                                F("Reserve"),
                                                F("Reserve"),
                                                F("Reserve"),
                                                F("Reserve"),
                                                F("Version"),
                                                F("SW-ID"),
                                                F("Config 1")
#if defined ETHERNET_BOARD
                                              , F("IP-Part 3"), F("IP-Part 4")
#endif
                                                };
                                    
  if(ui8_cv < MAX_CV)
    return cvName[ui8_cv];
  return F("???");
}

//=== functions ==================================================
uint8_t GetCVCount() { return MAX_CV; }

boolean AlreadyCVInitialized() { return (ui8a_CV[SOFTWARE_ID] == FASTCLOCK_LN); }

boolean IsCVUI16(uint8_t ui8_Index) { return false; }

boolean IsCVBinary(uint8_t ui8_Index) { return (ui8_Index < MAX_CV ? cvDefinition[ui8_Index].BINARY : false); }

uint8_t GetCVMaxValue(uint8_t ui8_Index) { return (ui8_Index < MAX_CV ? cvDefinition[ui8_Index].MAX : UINT8_MAX); }

uint8_t GetCV(uint8_t ui8_Index) { return (ui8_Index < MAX_CV ? ui8a_CV[ui8_Index] : UINT8_MAX); }

boolean CanEditCV(uint8_t ui8_Index) { return (ui8_Index < MAX_CV ? !cvDefinition[ui8_Index].RO : false); }

void SetCVsToDefault()
{
#if defined DEBUG
	Serial.println(F("initalizing CVs"));
#endif

	for (uint8_t ui8_Index = 0; ui8_Index < MAX_CV; ui8_Index++)
		WriteCVtoEEPROM(cvDefinition[ui8_Index].ID, cvDefinition[ui8_Index].DEFLT);

#if defined DEBUG
#if defined DEBUG_CV
	PrintCurrentCVs();
#endif  
#endif
}

void ReadCVsFromEEPROM()
{
	// start reading from the first byte (address 0) of the EEPROM
	for (uint8_t i = 0; i < MAX_CV; i++)
		ui8a_CV[i] = EEPROM.read(i);

	// check if current Version is in EEPROM:
	if (ui8a_CV[VERSION_NUMBER] < SW_VERSION)
		// write current Version into EEPROM:
		WriteCVtoEEPROM(VERSION_NUMBER, SW_VERSION);

#if defined DEBUG
#if defined DEBUG_CV
	PrintCurrentCVs();
#endif  
#endif

	if (!AlreadyCVInitialized())  // initialising has to be done
		SetCVsToDefault();
}

#if defined DEBUG
#if defined DEBUG_CV
void PrintCurrentCVs()
{
	Serial.println(F("current CVs"));
	for (uint8_t i = 0; i < MAX_CV; i++)
	{
		Serial.print(F("CV "));
		Serial.print(i + 1);
		Serial.print(F(": "));
		Serial.println(ui8a_CV[i]);
	}
}
#endif  
#endif

void WriteCVtoEEPROM(uint8_t ui8_Index, uint8_t iValue)
{
	if ((ui8_Index < MAX_CV) && (iValue <= cvDefinition[ui8_Index].MAX))
	{
		ui8a_CV[ui8_Index] = iValue;
		EEPROM.write(ui8_Index, ui8a_CV[ui8_Index]);
	}
}

boolean CheckAndWriteCVtoEEPROM(uint8_t ui8_Index, uint8_t ui8_Value)
{
	if (ui8_Index == VERSION_NUMBER)
	{
		if (ui8_Value == 0)
			SetCVsToDefault();
		else
			WriteCVtoEEPROM(VERSION_NUMBER, SW_VERSION);  // reset to default
		return true;
	}

	boolean b_WriteCV(!cvDefinition[ui8_Index].RO);
	if ((ui8_Value < cvDefinition[ui8_Index].MIN) || (ui8_Value > cvDefinition[ui8_Index].MAX))
		b_WriteCV = false;
	if (b_WriteCV)
	{
		WriteCVtoEEPROM(ui8_Index, ui8_Value);
		return true;
	}
	return false;
}
