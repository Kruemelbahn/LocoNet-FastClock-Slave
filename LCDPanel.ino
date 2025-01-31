//=== LCDPanel for LocoNET-FastClock ===
#include <Wire.h>
#include <LCDPanel.h>

//=== declaration of var's =======================================
LCDPanel lcd = LCDPanel();

uint8_t ui8_LCDPresent = 0;  // ui8_LCDPresent: 1 if I2C-LCD-Panel is found

/* mode:
  0   after init "LN-FastClock" is displayed
  1   "Status?" is displayed
  2   "Inbetriebnahme?" is displayed
  7   "FastClock" is displayed
 
 10   view current FastClock state

 20   (edit) mode for CV1
 21   (edit) mode for CV2
 22   (edit) mode for CV3
 23   (edit) mode for CV4
 24   (edit) mode for CV5
 25   (edit) mode for CV6
 26   (edit) mode for CV7
 27   (edit) mode for CV8
 28   (edit) mode for CV9
 
200   confirm display CV's
210   confirm display I2C-Scan
211   I2C-Scan

220   display IP-Address
221   display MAC-Address

*/
uint8_t ui8_LCDPanelMode = 0;  // ui8_LCDPanelMode for paneloperation

uint8_t ui8_ButtonMirror = 0;
static const uint8_t MAX_MODE = 19 + GetCVCount();

uint8_t ui8_EditValue = 0;
boolean b_Edit = false;

boolean b_IBN = false;

// used for binary-edit:
boolean b_EditBinary = false;
uint8_t ui8_CursorX = 0;

//=== functions ==================================================
boolean IBNbyLCD() { return b_IBN; }

void CheckAndInitLCDPanel()
{
  boolean b_LCDPanelDetected(lcd.detect_i2c(MCP23017_ADDRESS) == 0);
  if(!b_LCDPanelDetected && ui8_LCDPresent)
    // LCD was present is now absent:
    b_IBN = false;
    
  if(b_LCDPanelDetected && !ui8_LCDPresent)
  {
    // LCD (newly) found:
    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);

#if defined DEBUG
    Serial.println(F("LCD-Panel found..."));
    Serial.print(F("..."));
    Serial.println(GetSwTitle());
#endif

    if(AlreadyCVInitialized())
      OutTextTitle();

    ui8_LCDPanelMode = ui8_ButtonMirror = ui8_EditValue = 0;
    b_Edit = b_IBN = false;

  } // if(b_LCDPanelDetected && !ui8_LCDPresent)
  ui8_LCDPresent = (b_LCDPanelDetected ? 1 : 0);
}

void SetLCDPanelModeStatus()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("Status?"));
  ui8_LCDPanelMode = 1;
}

void SetLCDPanelModeIBN()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("Inbetriebnahme?"));
  ui8_LCDPanelMode = 2;
}

void SetLCDPanelModeCV()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("CV?"));
  ui8_LCDPanelMode = 200;
}

void SetLCDPanelModeScan()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("I2C-Scan?"));
  ui8_LCDPanelMode = 210;
}

#if defined ETHERNET_BOARD
void SetLCDPanelModeIpAdr()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("IP-Adresse"));
  lcd.setCursor(0, 1);  // set the cursor to column x, line y
  lcd.print(Ethernet.localIP());
  ui8_LCDPanelMode = 220;
}

void SetLCDPanelModeMacAdr()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("MAC-Adresse"));
  lcd.setCursor(0, 1);  // set the cursor to column x, line y
  uint8_t *mac(GetMACAddress());
  for(uint8_t i = 0; i < 6; i++)
  {
    hexout2(lcd, mac[i]);
    if((i == 1) || (i == 3))
      lcd.print('.');
  }

  ui8_LCDPanelMode = 221;
}
#endif

void InitScan()
{
   lcd.setCursor(8, 0);  // set the cursor to column x, line y
   lcd.print(' ');       // clear '?'
   ui8_EditValue = -1;
   NextScan();
}

void NextScan()
{
   ++ui8_EditValue;
   lcd.setCursor(0, 1);  // set the cursor to column x, line y
   
   if(ui8_EditValue > 127)
   {
     OutTextFertig();
     return;
   }

   uint8_t error(0);
   do
   {
     Wire.beginTransmission(ui8_EditValue);
     error = Wire.endTransmission();

     if ((error == 0) || (error==4))
     {
       lcd.print(F("0x"));
       hexout(lcd, ui8_EditValue, 2);
       lcd.print((error == 4) ? '?' : ' ');
       break;
     }

     ++ui8_EditValue;
     if(ui8_EditValue > 127)
     { // done:
       OutTextFertig();
       break;
     }
   } while (true);
}

void OutTextFertig() { lcd.print(F("fertig")); }

void OutTextTitle()
{
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(GetSwTitle()); 
    
  lcd.setCursor(0, 1);
  lcd.print(F("Version "));
  lcd.print(GetCV(VERSION_NUMBER));
#if defined DEBUG
  lcd.print(F(" Deb"));
#endif
#if defined TELEGRAM_FROM_SERIAL
  lcd.print(F("Sim"));
#endif
}

void OutTextClockStatus()
{
  lcd.setCursor(0, 1);  // set the cursor to column x, line y

	uint16_t ui16_iCount(0);
	uint8_t ui8_Rate(0);
	uint8_t ui8_Sync(0);
	if(!GetClockState(&ui16_iCount, &ui8_Rate, &ui8_Sync))
		lcd.print(F("---"));
	else
	{
		decout(lcd, ui16_iCount, 4);
		lcd.print(F("-1:"));
		decout(lcd, ui8_Rate, 2);
		lcd.print('-');
		lcd.print(ui8_Sync? '1' : '0');
		lcd.print('-');
		uint8_t ui8_FCHour(0);
		uint8_t ui8_FCMinute(0);
		GetFastClock(&ui8_FCHour, &ui8_FCMinute);
		lcd.print((ui8_FCMinute % 2) ? F("Odd ") : F("Even"));
	}
}

void DisplayCV(uint8_t iValue)
{
  lcd.noCursor();
  lcd.clear();
  lcd.setCursor(0, 0);  // set the cursor to column x, line y
  lcd.print(F("CV"));
  lcd.setCursor(3, 0);
  uint8_t i(ui8_LCDPanelMode - 19);
  if(i < 10)
    lcd.print(' ');
  lcd.print(i);
  --i;

  // show shortname:
  lcd.setCursor(6, 0);
  lcd.print(GetCVName(i));

  lcd.setCursor(0, 1);
  if(b_Edit)
    lcd.print('>');
  if(IsCVBinary(i))
    binout(lcd, iValue);
  else
    lcd.print(iValue);
  if(!b_Edit && !CanEditCV(i))
  {
    lcd.setCursor(14, 1);
    lcd.print(F("ro"));
  }
  if(b_EditBinary)
  {
    lcd.setCursor(ui8_CursorX, 1);
    lcd.cursor();
  }
}

void HandleLCDPanel()
{
  CheckAndInitLCDPanel();

  if(ui8_LCDPresent != 1)
    return;

  uint8_t ui8_bs = 1;
  if(lcd.readButtonA5() == BUTTON_A5)
    ui8_bs = 0;
  lcd.setBacklight(ui8_bs);

  uint8_t ui8_buttons = lcd.readButtons();  // reads only 5 buttons (A0...A4)
  if(!ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
    ui8_ButtonMirror = 0;
    return;
  }

  if(ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  {
    ui8_ButtonMirror = ui8_buttons;
    if(ui8_LCDPanelMode == 0)
    {
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to Status?
        SetLCDPanelModeStatus();  // mode = 1
      }
      return;
    } // if(ui8_LCDPanelMode == 0)
    //------------------------------------
    if(ui8_LCDPanelMode == 1)
    {
      // actual ui8_LCDPanelMode = "Status?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Title
        OutTextTitle();
        ui8_LCDPanelMode = 0;
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to IBN
        SetLCDPanelModeIBN(); // mode = 2
        return;
      }
      if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display current clock state
				lcd.clear();
				lcd.setCursor(0, 0);  // set the cursor to column x, line y
				lcd.print(F("Status"));
        OutTextClockStatus();
        ui8_LCDPanelMode = 10;
        return;
      }
    } // if(ui8_LCDPanelMode == 1)
    //------------------------------------
    if(ui8_LCDPanelMode == 2)
    {
      // actual ui8_LCDPanelMode = "IBN?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to Status
        SetLCDPanelModeStatus();  // mode = 1
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to ask CV / IÂ²C-Scan
        SetLCDPanelModeCV();
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to FastClock
        ui8_LCDPanelMode = 7;
        lcd.clear();
        lcd.setCursor(0, 0);  // set the cursor to column x, line y
        lcd.print(F("FastClock"));
      }
      return;
    } // if(ui8_LCDPanelMode == 2)
    //------------------------------------
    if(ui8_LCDPanelMode == 7)
    {
      // actual ui8_LCDPanelMode = "FastClock"
      if (ui8_buttons & BUTTON_UP)
      { // switch to IBN
        SetLCDPanelModeIBN(); // mode = 2
        return;
      }
    } // if(ui8_LCDPanelMode == 7)
    //------------------------------------
    if(ui8_LCDPanelMode == 10)
    {
      // actual ui8_LCDPanelMode = view current clock state
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to display "Status?"
        SetLCDPanelModeStatus();  // mode = 1
        return;
      }
    } // if(ui8_LCDPanelMode == 10)
    //------------------------------------
    if((ui8_LCDPanelMode >= 20) && (ui8_LCDPanelMode <= MAX_MODE))
    {
      // actual ui8_LCDPanelMode = view CV
      if (ui8_buttons & BUTTON_SELECT)
      { // save value for current CV
        if(b_Edit)
        {
          // save current value
          boolean b_OK = CheckAndWriteCVtoEEPROM(ui8_LCDPanelMode - 20, ui8_EditValue);
          DisplayCV(GetCV(ui8_LCDPanelMode - 20));
          lcd.setCursor(10, 1);  // set the cursor to column x, line y
          lcd.print(b_OK ? F("stored") : F("failed"));
          lcd.setCursor(ui8_CursorX, 1);
        }
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to display current CV's
        if(b_Edit)
        {
          // leave edit-mode (without save)
          b_Edit = b_EditBinary = false;
          DisplayCV(GetCV(ui8_LCDPanelMode - 20));
          return;
        }
        SetLCDPanelModeCV();
        b_IBN = false;
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to edit current CV's
        if(!b_Edit && CanEditCV(ui8_LCDPanelMode - 20))
        {
          // enter edit-mode
          ui8_EditValue = GetCV(ui8_LCDPanelMode - 20);
          b_Edit = true;
          b_EditBinary = IsCVBinary(ui8_LCDPanelMode - 20);
          ui8_CursorX = 1;
          DisplayCV(ui8_EditValue);
        }
        else if(b_EditBinary)
        { // move cursor only in one direction
          ++ui8_CursorX;
          if(ui8_CursorX == 9)
            ui8_CursorX = 1;  // rollover
          lcd.setCursor(ui8_CursorX, 1);
          lcd.cursor();
        }
      }
      else if (ui8_buttons & BUTTON_UP)
      { // switch to previous CV
        if(b_Edit)
        { // Wert verkleinern:
          if(b_EditBinary)
            ui8_EditValue &= ~(1 << (8 - ui8_CursorX));
          else
            --ui8_EditValue;
          if(ui8_EditValue == 255) // 0 -> 255
            ui8_EditValue = GetCVMaxValue(ui8_LCDPanelMode - 20);  // Unterlauf
          DisplayCV(ui8_EditValue);
          return;
        } // if(b_Edit)
        --ui8_LCDPanelMode;
        if(ui8_LCDPanelMode < 20)
          ui8_LCDPanelMode = 20;
        DisplayCV(GetCV(ui8_LCDPanelMode - 20));
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to next CV
        if(b_Edit)
        { // Wert vergrÃ¶ÃŸern:
          if(b_EditBinary)
            ui8_EditValue |= (1 << (8 - ui8_CursorX));
          else
            ++ui8_EditValue;
          if(ui8_EditValue > GetCVMaxValue(ui8_LCDPanelMode - 20))
            ui8_EditValue = 0;  // Ãœberlauf
          DisplayCV(ui8_EditValue);
          return;
        }
        ++ui8_LCDPanelMode;
        if(ui8_LCDPanelMode >= MAX_MODE)
          ui8_LCDPanelMode = MAX_MODE;
        DisplayCV(GetCV(ui8_LCDPanelMode - 20));
      }
      return;
    } // if((ui8_LCDPanelMode >= 20) && (ui8_LCDPanelMode <= MAX_MODE))
    //------------------------------------
    if(ui8_LCDPanelMode == 200)
    {
      // actual ui8_LCDPanelMode = "CV?"
      if (ui8_buttons & BUTTON_DOWN)
      { // switch to IÂ²C-Scan
        SetLCDPanelModeScan();
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to "IBN?"
        SetLCDPanelModeIBN();
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display current CV's
        ui8_LCDPanelMode = 20;
        DisplayCV(GetCV(ID_DEVICE));
        b_IBN = true;
      }
      return;
    } // if(ui8_LCDPanelMode == 200)
    //------------------------------------
    if(ui8_LCDPanelMode == 210)
    {
      // actual ui8_LCDPanelMode = "IÂ²C-Scan?"
      if (ui8_buttons & BUTTON_UP)
      { // switch to "CV?"
        SetLCDPanelModeCV();
      }
      else if (ui8_buttons & BUTTON_LEFT)
      { // switch to "Inbetriebnahme?"
        SetLCDPanelModeIBN();
      }
      else if (ui8_buttons & BUTTON_RIGHT)
      { // switch to display IÂ²C-Addresses
        ui8_LCDPanelMode = 211;
        InitScan();
      }
#if defined ETHERNET_BOARD
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to display IP-Address
        SetLCDPanelModeIpAdr();
      }
#endif
      return;
    } // if(ui8_LCDPanelMode == 210)
    //------------------------------------
    if(ui8_LCDPanelMode == 211)
    {
      // actual ui8_LCDPanelMode = "IÂ²C-Scan"
      if (ui8_buttons & BUTTON_LEFT)
      { // switch to "IÂ²C-Scan?"
        SetLCDPanelModeScan();
        return;
      }
      if (ui8_buttons & BUTTON_DOWN)
      {
        NextScan();
        return;
      }
    } // if(ui8_LCDPanelMode == 211)
    //------------------------------------
#if defined ETHERNET_BOARD
    if(ui8_LCDPanelMode == 220)
    {
      // actual ui8_LCDPanelMode = "IP-Address"
      if (ui8_buttons & BUTTON_UP)
      { // switch to "IÂ²C-Scan ?"
        SetLCDPanelModeScan();
      }
      else if (ui8_buttons & BUTTON_DOWN)
      { // switch to "MAC-Address"
        SetLCDPanelModeMacAdr();
      }
      return;
    } // if(ui8_LCDPanelMode == 220)
    if(ui8_LCDPanelMode == 221)
    {
      // actual ui8_LCDPanelMode = "MAC-Address"
      if (ui8_buttons & BUTTON_FCT_BACK)
      { // any key returns to "IP-Address"
        SetLCDPanelModeIpAdr();
      }
      return;
    } // if(ui8_LCDPanelMode == 221)
#endif
  } // if(ui8_buttons && (ui8_ButtonMirror != ui8_buttons))
  //========================================================================
  if(ui8_LCDPanelMode == 7)
  { // display FastClock time in bottom row:
    lcd.setCursor(0, 1);  // set the cursor to column x, line y
    uint8_t ui8_Hour;
    uint8_t ui8_Minute;
    if(GetFastClock(&ui8_Hour, &ui8_Minute))
    {
      if (ui8_Hour < 23)
        decout(lcd, ui8_Hour, 2);
      else
        lcd.print("??");
      lcd.print(':');
      if (ui8_Minute < 60)
        decout(lcd, ui8_Minute, 2);
      else
        lcd.print("??");
    }
    else
      lcd.print(F("--:--"));
  } // if(ui8_LCDPanelMode == 7) 
  //========================================================================
  if(ui8_LCDPanelMode == 10)
  {
		OutTextClockStatus();
	}
  //========================================================================
}


