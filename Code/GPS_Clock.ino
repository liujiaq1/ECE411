/*
 OpenLCD is an LCD with Serial/I2C/SPI interfaces.
 By: Nathan Seidle
 SparkFun Electronics
 Date: April 19th, 2015
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 This is example code that shows how to send data over SPI to the display.
*/


//Every 15 degrees of longitude equals 1 hour. So, if you are standing at 0 degrees longitude 
//and you move or travel 15 degrees east or west, you'll notice a difference of 1 hour. 
//This 1 hour difference is negative or positive, can be determined by the direction in 
//which you have traveled i.e westwards or eastwards of the Meridian longitude.


#include <SPI.h> // For writing to LCD & serial console
#include <SerLCD.h>  //https://www.arduinolibraries.info/libraries/spark-fun-ser-lcd-arduino-library
#include <SoftwareSerial.h> // For reading from GPS

// initialize the library
SerLCD lcd;

SoftwareSerial GPSSerial(2,3); // RX, TX. Only RX used for recieving GPS data

// make some custom characters to display signal strength:
byte Sig1[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b10000
};
byte Sig2[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b01000,
  0b11000
};
byte Sig3[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00100,
  0b01100,
  0b11100
};
byte Sig4[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00010,
  0b00110,
  0b01110,
  0b11110
};
byte Sig5[8] = {
  0b00000,
  0b00000,
  0b00000,
  0b00001,
  0b00011,
  0b00111,
  0b01111,
  0b11111
};
byte Sig6[8] = {
  0b00000,
  0b00000,
  0b00001,
  0b00011,
  0b00111,
  0b01111,
  0b11111,
  0b11111
};
byte Sig7[8] = {
  0b00000,
  0b00001,
  0b00011,
  0b00111,
  0b01111,
  0b11111,
  0b11111,
  0b11111
};
byte Sig8[8] = {
  0b00001,
  0b00011,
  0b00111,
  0b01111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};
int csPin = 10; // Declare RedBoard's Pin 10 connected to LCD's cs pin
int GPSPin = 2; // Declare RedBoard's Pin 2 connected to GPS
int AmbientSensor = 3; // Declare RedBoard's Pin 3 connected to Ambient Light Sensor
int DSTSwitch = 4; // Declare RedBoard's Pin 4 connected to DTS switch

int cycles = 0;
int local_hour_offset = -8; //PST is -8 hours from UTC



char buffer[20]; //"Tuesday" and stuff
char today[20];
char time[7]; //145205 = 2:52:05
char Longitude[3];
char Dir[2];
char date[7]; //123114 = Dec 31, 2014
char sats[3]; //08 = 8 satellites in view
byte SIV = 0; // converted # of satalites in view
int TimeZone = 0; // converted from Longitude and Dir
char tempString[50]; //Needs to be large enough to hold the entire string with up to 5 digits
bool dark = true;
bool DST = false;

void setup() 
{
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600); // For writing to serial monitor
  GPSSerial.begin(9600); // For reading GPS
  while (!GPSSerial) {
    ; // wait for serial port to connect. Needed for Native USB only
  }
  
  pinMode(csPin, OUTPUT); // Declare csPin (pin 10) as output to LCD
  pinMode(GPSPin, INPUT); // Declare GPSPin (pin 2) as input from GPS
  pinMode(AmbientSensor, INPUT); // Declare AmbientSensor (pin 3) as input from GPS
  pinMode(DSTSwitch, INPUT); // Declare GDTSSwitch (pin 24) as input from GPS
  
  SPI.begin();

  lcd.begin(SPI, csPin, SPISettings(100000, MSBFIRST, SPI_MODE0));

  //Clear the display
  lcd.clear();
  
  //Print a message to the LCD.
  lcd.setCursor(2, 0); // set the cursor to column 2, line 0
  lcd.print("GPS Enabled Clock");
  lcd.setCursor(0, 1); // set the cursor to column 2, line 0
  lcd.print("ECE 411     PSU 2019");
  lcd.setCursor(3, 2); // set the cursor to column 2, line 0
  lcd.print("Cheng, Jemmett,");
  lcd.setCursor(5, 3); // set the cursor to column 2, line 0
  lcd.print("Jia, & Liu");
  //delay(10000);
    lcd.saveSplash(); //Save this current text as the splash screen at next power on

  lcd.enableSplash(); //This will cause the splash to be displayed at power on

      //Clear the display
  lcd.clear();
   lcd.setBacklight(0, 80, 0); 

  // Declare the custom Signal strength characters
  lcd.createChar(0, Sig1);
  lcd.createChar(1, Sig2);
  lcd.createChar(2, Sig3);
  lcd.createChar(3, Sig4);
  lcd.createChar(4, Sig5);
  lcd.createChar(5, Sig6);
  lcd.createChar(6, Sig7);
  lcd.createChar(7, Sig8);
}

void loop() 
{
    // Check Ambient lighting and adjust brightness only on intial lighting change
    // Light probably won't be strobing so no need for interupt
    if ((dark == false) && (digitalRead(AmbientSensor)))
    {
      lcd.setFastBacklight(0, 250, 0); // Light went out. Turn on backlight
      Serial.println("DARK");
      dark = true;
    }
    if ((dark == true) && (!digitalRead(AmbientSensor)))
    {
      lcd.setFastBacklight(0, 80, 0); // Light came on. Turn off backlight
      Serial.println("LIGHT");
      dark = false;
    }
  
 if(checkGPS() == true) //Checks for new serial data
  {
    //Crack strings into usable numbers
    int year;
    byte day, month;
    crackDate(&day, &month, &year);

    byte hour, minute, second;
    crackTime(&hour, &minute, &second);

    SIV = 0;
    crackSatellites(&SIV);

    TimeZone = 0; 

    //Convert time to local time using daylight savings if necessary
    convertToLocal(&day, &month, &year, &hour);

      // Display time on row 1
      lcd.setCursor(0, 0); // set the cursor to column 0, line 0
      sprintf(buffer, "%02d:%02d:%02d           ", hour, minute, second);
      lcd.print(buffer);
      
      if (DST)
      {
        lcd.setCursor(9, 0); // set the cursor to column 19, line 0
        lcd.print("DST");
      }

      // Display Signal Strength based on the number of satellites in view
      lcd.setCursor(19, 0); // set the cursor to column 19, line 0
      if (SIV > 7)
      {
        lcd.writeChar(7);
      }
      else
      {
        lcd.writeChar(SIV-1);
      }
          
      // Display Day and Date on row 2
      lcd.setCursor(0, 1); // set the cursor to column 0, line 1
      byte DoW = day_of_week(year, month, day);
      if(DoW == 0) sprintf(buffer, "Sunday %02d/%02d/%02d   ", month, day, year);
      if(DoW == 1) sprintf(buffer, "Monday %02d/%02d/%02d   ", month, day, year);
      if(DoW == 2) sprintf(buffer, "Tuesday %02d/%02d/%02d  ", month, day, year);
      if(DoW == 3) sprintf(buffer, "Wednesday %02d/%02d/%02d", month, day, year);
      if(DoW == 4) sprintf(buffer, "Thursday %02d/%02d/%02d ", month, day, year);
      if(DoW == 5) sprintf(buffer, "Friday %02d/%02d/%02d   ", month, day, year);
      if(DoW == 6) sprintf(buffer, "Saturday");      
      if (buffer != today)
      {
        lcd.print(buffer);
        for (int i = 0; i < 20; i++) {
          today[i] = buffer[i];
        }
      }

      lcd.setCursor(0, 2);
      lcd.print("                    ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      
      delay(250); 
  }
  else
  {
    //Display no signal message
    lcd.setCursor(3, 1);
    lcd.print("No GPS Signal");
  }




}





//Looks at the incoming serial stream and tries to find time, date and sats in view
boolean checkGPS()
{
  unsigned long start = millis();

  
  //Give up after a second of polling
  while (millis() - start < 1500)
  {
    // Determine if GPS information is recievable
    if(GPSSerial.available())
    {
      
      if(GPSSerial.read() == '$') // Read characters until we get to the start of a sentence
      {        
        // Read sentence type
        char sentenceType[6];
        for(byte x = 0 ; x < 6 ; x++)
        {
          while(GPSSerial.available() == 0) 
          {
            delay(1); //Wait            
          }
          sentenceType[x] = GPSSerial.read();
        }
        sentenceType[5] = '\0';
         
        if(sentenceType[3] == 'G' && sentenceType[4] == 'A') // Continue if format is GPGGA ----------------------------------------------
        { 
          //We are now listening to the GPGGA sentence for time and number of sats
          //$Message ID,UTC Time,Latitude,N/S,Longitude,E/W,Pos Indicator,SIV,HDOP,MSL Altitude,Geoidal Seperation,Units,Age of Diff Corr,CheckSum,<CR><LF>
          //6d,9d,10d,1d,11d,1d,1d,2d,4d,4d,1d,?,?,3d,?
          //$GPGGA,145205.00,3902.08127,N,10415.90019,W,2,08,2.68,1611.4,M,-21.3,M,,0000*5C
          
          //Grab time
          for(byte x = 0 ; x < 6 ; x++)
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
            time[x] = GPSSerial.read(); //Get time characters
            

            //Error check for a time that is too short
            if(time[x] == ',')
            {
              //Serial.println("No time found");
              return(false);
            }
          }
          time[6] = '\0';
          //Serial.println(time);
          
          
          //Wait for 3 commas to go by
          for(byte commaCounter = 3 ; commaCounter > 0 ; )
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
            char incoming = GPSSerial.read(); //Get time characters
            if(incoming == ',') commaCounter--;
          }

          //Grab Longitude
          for(byte x = 0 ; x < 11 ; x++)
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
            if (x<3)
            {
              Longitude[x] = GPSSerial.read(); //Get sats in view characters
            }
          }
          Longitude[3] = '\0';
          //Serial.println(Longitude);

          //Wait for 1 comma to go by
          for(byte commaCounter = 1 ; commaCounter > 0 ; )
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
            char incoming = GPSSerial.read(); //Get time characters
            if(incoming == ',') commaCounter--;
          }



          //Grab Direction, E or W of UTC
          for(byte x = 0 ; x < 1 ; x++)
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
              Dir[x] = GPSSerial.read(); //Get sats in view characters
          }
          Dir[1] = '\0';
          //Serial.println(Dir);

          //Wait for 2 commas to go by
          for(byte commaCounter = 2 ; commaCounter > 0 ; )
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
            char incoming = GPSSerial.read(); //Get time characters
            if(incoming == ',') commaCounter--;
          }

          //Grab sats in view
          for(byte x = 0 ; x < 2 ; x++)
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
            sats[x] = GPSSerial.read(); //Get sats in view characters
          }
          sats[2] = '\0';
          //Serial.println(sats);
          
          //Once we have GPGGA, we should already have GPRMC so return
          return(true);
        }
        else if(sentenceType[3] == 'M' && sentenceType[4] == 'C')  // Continue if format is GPRMC ----------------------------------------------
        {
          //We are now listening to GPRMC for the date
          //Message ID,UTC Time,Status,Latitude,N/S,Longitude,E/W,Speed,Course,Date,Magnetic Variation, Mode, Checksum,<CR><LF>
          //6d,9d,1d,10d,1d,11d,1d,6d,6d,?,1d,3d,?
          //$GPRMC,145205.00,A,3902.08127,N,10415.90019,W,0.772,,010115,,,D*6A

          //Wait for 8 commas to go by
          for(byte commaCounter = 8 ; commaCounter > 0 ; )
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
            char incoming = GPSSerial.read(); //Get time characters
            if(incoming == ',') commaCounter--;
          }

          //Grab date
          for(byte x = 0 ; x < 6 ; x++)
          {
            while(GPSSerial.available() == 0) delay(1); //Wait
            date[x] = GPSSerial.read(); //Get date characters
            
            //Error check for a date too short
            if(date[x] == ',')
            {
              //Serial.println("No date found");
              return(false);
            }
          }
          date[6] = '\0';
          //Serial.println(date);
        }

      }
    }
  }
  
  Serial.println("No valid GPS serial data");
  return(false);
}




//Given date and hours, convert to local time using DST
void convertToLocal(byte* day, byte* month, int* year, byte* hours)
{
  // Determine if the user has declared DST
  if (digitalRead(DSTSwitch))
  {
     DST = false;
  }
  else
  {
    DST = true;
    *hours = *hours + 1; //In DST add an extra hour
  }

  // Determine time zone based on Longitude
  float Long = 0;
  Long = (Longitude[0] - '0') * 100;
  Long += (Longitude[1] - '0') * 10;
  Long += (Longitude[2] - '0');
  if (Dir[0] == 'W')
  {
    TimeZone = -1 * ceil(Long/15);
  }
  else
  {
    TimeZone = ceil(Long/15);
  }

  
  //Convert UTC hours to local current time based on time zone
  if(*hours < TimeZone)
  {
    //Go back a day in time
    *day = *day - 1;
    
    if(*day == 0)
    {
      //Determine if this changes the date
      *month = *month - 1;
      
      if(*month == 1) *day = 31;
      if(*month == 2) *day = 28; 
      if(*month == 3) *day = 31;
      if(*month == 4) *day = 30;
      if(*month == 5) *day = 31;
      if(*month == 6) *day = 30;
      if(*month == 7) *day = 31;
      if(*month == 8) *day = 31;
      if(*month == 9) *day = 30;
      if(*month == 10) *day = 31;
      if(*month == 11) *day = 30;
      if(*month == 0)
      {
        *year = *year - 1;
        *month = 12;
        *day = 31;
      }
    }
    
    *hours = *hours + 24; //Add 24 hours before subtracting local offset
  }
  *hours = *hours + TimeZone;
  if(*hours > 12) *hours = *hours - 12; //Get rid of military time
  
}

//Given the date string return day, month, year
void crackDate(byte* day, byte* month, int* year)
{
  for(byte x = 0 ; x < 3 ; x++)
  {
    byte temp = (date[ (x*2) ] - '0') * 10;
    temp += (date[ (x*2)+1 ] - '0');
    
    if(x == 0) *day = temp;
    if(x == 1) *month = temp;
    if(x == 2)
    {
      *year = temp;
      *year += 2000;
    }
  }
}

//Given the time string return hours, minutes, seconds
void crackTime(byte* hours, byte* minutes, byte* seconds)
{
  //Serial.println(time);
  for(byte x = 0 ; x < 3 ; x++)
  {
    byte temp = (time[ (x*2) ] - '0') * 10;
    temp += (time[ (x*2)+1 ] - '0');
    
    if(x == 0) *hours = temp;
    if(x == 1) *minutes = temp;
    if(x == 2) *seconds = temp;
    
  }
}



//Given the sats string return satellites in view
void crackSatellites(byte* SIV)
{
  //Serial.println(sats);
  *SIV = (sats[0] - '0') * 10;
  *SIV += (sats[1] - '0');
}



//Given the current year/month/day
//Returns 0 (Sunday) through 6 (Saturday) for the day of the week
//Assumes we are operating in the 2000-2099 century
//From: http://en.wikipedia.org/wiki/Calculating_the_day_of_the_week
char day_of_week(int year, byte month, byte day) {

  //offset = centuries table + year digits + year fractional + month lookup + date
  int centuries_table = 6; //We assume this code will only be used from year 2000 to year 2099
  int year_digits;
  int year_fractional;
  int month_lookup;
  int offset;

  //Example Feb 9th, 2011

  //First boil down year, example year = 2011
  year_digits = year % 100; //year_digits = 11
  year_fractional = year_digits / 4; //year_fractional = 2

  switch(month) {
  case 1: 
    month_lookup = 0; //January = 0
    break; 
  case 2: 
    month_lookup = 3; //February = 3
    break; 
  case 3: 
    month_lookup = 3; //March = 3
    break; 
  case 4: 
    month_lookup = 6; //April = 6
    break; 
  case 5: 
    month_lookup = 1; //May = 1
    break; 
  case 6: 
    month_lookup = 4; //June = 4
    break; 
  case 7: 
    month_lookup = 6; //July = 6
    break; 
  case 8: 
    month_lookup = 2; //August = 2
    break; 
  case 9: 
    month_lookup = 5; //September = 5
    break; 
  case 10: 
    month_lookup = 0; //October = 0
    break; 
  case 11: 
    month_lookup = 3; //November = 3
    break; 
  case 12: 
    month_lookup = 5; //December = 5
    break; 
  default: 
    month_lookup = 0; //Error!
    return(-1);
  }

  offset = centuries_table + year_digits + year_fractional + month_lookup + day;
  //offset = 6 + 11 + 2 + 3 + 9 = 31
  offset %= 7; // 31 % 7 = 3 Wednesday!

  return(offset); //Day of week, 0 to 6

  //Example: May 11th, 2012
  //6 + 12 + 3 + 1 + 11 = 33
  //5 = Friday! It works!
}