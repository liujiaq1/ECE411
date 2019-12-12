/*
 OpenLCD is an LCD with Serial/I2C/SPI interfaces.
 By: Nathan Seidle
 SparkFun Electronics
 Date: April 19th, 2015
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 This is example code that shows how to send data over SPI to the display.
 
 To get this code to work, attached an OpenLCD to an Arduino Uno using the following pins:
 CS (OpenLCD) to 10 (Arduino)
 SDI to 11
 SDO to 12 (optional)
 SCK to 13
 VIN to 5V
 GND to GND
Command cheat sheet:
 ASCII / DEC / HEX
 '|'    / 124 / 0x7C - Put into setting mode
 Ctrl+c / 3 / 0x03 - Change width to 20
 Ctrl+d / 4 / 0x04 - Change width to 16
 Ctrl+e / 5 / 0x05 - Change lines to 4
 Ctrl+f / 6 / 0x06 - Change lines to 2
 Ctrl+g / 7 / 0x07 - Change lines to 1
 Ctrl+h / 8 / 0x08 - Software reset of the system
 Ctrl+i / 9 / 0x09 - Enable/disable splash screen
 Ctrl+j / 10 / 0x0A - Save currently displayed text as splash
 Ctrl+k / 11 / 0x0B - Change baud to 2400bps
 Ctrl+l / 12 / 0x0C - Change baud to 4800bps
 Ctrl+m / 13 / 0x0D - Change baud to 9600bps
 Ctrl+n / 14 / 0x0E - Change baud to 14400bps
 Ctrl+o / 15 / 0x0F - Change baud to 19200bps
 Ctrl+p / 16 / 0x10 - Change baud to 38400bps
 Ctrl+q / 17 / 0x11 - Change baud to 57600bps
 Ctrl+r / 18 / 0x12 - Change baud to 115200bps
 Ctrl+s / 19 / 0x13 - Change baud to 230400bps
 Ctrl+t / 20 / 0x14 - Change baud to 460800bps
 Ctrl+u / 21 / 0x15 - Change baud to 921600bps
 Ctrl+v / 22 / 0x16 - Change baud to 1000000bps
 Ctrl+w / 23 / 0x17 - Change baud to 1200bps
 Ctrl+x / 24 / 0x18 - Change the contrast. Follow Ctrl+x with number 0 to 255. 120 is default.
 Ctrl+y / 25 / 0x19 - Change the TWI address. Follow Ctrl+x with number 0 to 255. 114 (0x72) is default.
 Ctrl+z / 26 / 0x1A - Enable/disable ignore RX pin on startup (ignore emergency reset)
 '-'    / 45 / 0x2D - Clear display. Move cursor to home position.
        / 128-157 / 0x80-0x9D - Set the primary backlight brightness. 128 = Off, 157 = 100%.
        / 158-187 / 0x9E-0xBB - Set the green backlight brightness. 158 = Off, 187 = 100%.
        / 188-217 / 0xBC-0xD9 - Set the blue backlight brightness. 188 = Off, 217 = 100%.
         For example, to change the baud rate to 115200 send 124 followed by 18.
 '+'    / 43 / 0x2B - Set Backlight to RGB value, follow + by 3 numbers 0 to 255, for the r, g and b values.
         For example, to change the backlight to yellow send + followed by 255, 255 and 0.
*/


//Every 15 degrees of longitude equals 1 hour. So, if you are standing at 0 degrees longitude 
//and you move or travel 15 degrees east or west, you'll notice a difference of 1 hour. 
//This 1 hour difference is negative or positive, can be determined by the direction in 
//which you have traveled i.e westwards or eastwards of the Meridian longitude.


#include <SPI.h> // For writing to LCD
#include <SoftwareSerial.h> // For reading from GPS

SoftwareSerial GPSSerial(2,3); // RX, TX. Only RX used for recieving GPS data

int csPin = 10; // Declare RedBoard's Pin 10 connected to LCD's cs pin
int GPSPin = 2; // Declare RedBoard's Pin 2 connected to GPS

int cycles = 0;
int local_hour_offset = 8; //PST is -8 hours from UTC



char buffer[20]; //"Tuesday" and stuff
char time[7]; //145205 = 2:52:05
char date[7]; //123114 = Dec 31, 2014
char sats[3]; //08 = 8 satellites in view
byte SIV = 0; // converted # of satalites in view
char tempString[50]; //Needs to be large enough to hold the entire string with up to 5 digits


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
  
  digitalWrite(csPin, HIGH); //By default, don't be selecting OpenLCD
  
  SPI.begin(); //Start SPI communication
  //SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
  SPI.setClockDivider(SPI_CLOCK_DIV128); //Slow down the master a bit



  digitalWrite(csPin, LOW); //Drive the CS pin low to select OpenLCD
  SPI.transfer(8);
    SPI.transfer('|'); //Put LCD into setting mode
    SPI.transfer('-'); //Send clear display command
    SPI.transfer('+');
    SPI.transfer(250);
  digitalWrite(csPin, HIGH); //Release the CS pin to de-select OpenLCD
  
  spiSendString(" GPS Enabled Clock\rECE 411     PSU 2019    Cheng, Jia,\r   Jemmett, & Liu");
  delay(5250);



}

void loop() 
{


  
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
    
    //Convert time to local time using daylight savings if necessary
    convertToLocal(&day, &month, &year, &hour);
    
    //For debugging
//    Serial.print("Date: "); 
//    Serial.print(month); 
//    Serial.print("/"); 
//    Serial.print(day); 
//    Serial.print("/"); 
//    Serial.print(year);
//    Serial.print(" ");
//
//    Serial.print("Time: "); 
//    Serial.print(hour); 
//    Serial.print(":"); 
//    Serial.print(minute); 
//    Serial.print(":"); 
//    Serial.print(second);
//    Serial.print(" ");
//    
//    Serial.print("SIV: "); 
//    Serial.print(SIV);
//   
//    Serial.println();

    //Update display with stuff
    //Let's just display the time, but on 15 seconds and 45 seconds, display something else
    if( (second == 15) || (second == 45))
    { 
      if(second == 15) //On every 0:15, Day of week and Go home!
      {
        byte DoW = day_of_week(year, month, day);
        if(DoW == 0 || DoW == 6) //Sunday or Saturday
        {
          sprintf(buffer, " GoHome");
        }
        else //Display the day of the week
        {
          if(DoW == 1) sprintf(buffer, " Monday");
          if(DoW == 2) sprintf(buffer, " Tuesday");
          if(DoW == 3) sprintf(buffer, " Wednesday");
          if(DoW == 4) sprintf(buffer, " Thursday");
          if(DoW == 5) sprintf(buffer, " Friday!");
        }
      }
      else if(second == 45) //On every 0:45, Date
      {
        sprintf(buffer, " Date: %02d/%02d/%02d", month, day, year % 100);
      }

      //spiSendString(buffer);
      //delay(1000); //Display this message for 1 second
    }
    else //Just print the time
    {
      //Let's count down to new years!
      //sprintf(buffer, "%02d%02d%02d", 11 - hour, 59 - minute, 59 - second);

      sprintf(buffer, "%02d:%02d:%02d", hour, minute, second);
      
    } 
    sprintf(buffer, "%02d:%02d:%02d\r%02d/%02d/%02d\r# Satellite = %02d", hour, minute, second, month, day, year, SIV);
    spiSendString(buffer);
    delay(250);
  }
  
  //Turn on stat LED if we have enough sats for a lock
//  if(SIV > 3)
//    digitalWrite(statLED, HIGH);
//  else
//    digitalWrite(statLED, LOW);
//


}

//Sends a string over SPI
void spiSendString(char* data)
{
  digitalWrite(csPin, LOW); //Drive the CS pin low to select OpenLCD
  SPI.transfer('|'); //Put LCD into setting mode
  SPI.transfer('-'); //Send clear display command
  for(byte x = 0 ; data[x] != '\0' ; x++) //Send chars until we hit the end of the string
    SPI.transfer(data[x]);
  digitalWrite(csPin, HIGH); //Release the CS pin to de-select OpenLCD
}



//Looks at the incoming serial stream and tries to find time, date and sats in view
boolean checkGPS()
{
  unsigned long start = millis();

  
  //Give up after a second of polling
  while (millis() - start < 1500)
  {
    //Serial.println("checkGPS: Loop while millis() - start < 1500");

    
    if(GPSSerial.available())
    {
      
      if(GPSSerial.read() == '$') //Spin until we get to the start of a sentence
      {
        //Get "GPGGA,"
        
        char sentenceType[6];
        for(byte x = 0 ; x < 6 ; x++)
        {
          //Serial.println("checkGPS: For loop ");
          while(GPSSerial.available() == 0) 
          {
            delay(1); //Wait
            
          }
          sentenceType[x] = GPSSerial.read();
        }
        sentenceType[5] = '\0';
         //Serial.println(sentenceType); 

         
        if(sentenceType[3] == 'G' && sentenceType[4] == 'A') // Continue if format is GPGGA
        { 
          //We are now listening to the GPGGA sentence for time and number of sats
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
          
          
          //Wait for 6 commas to go by
          for(byte commaCounter = 6 ; commaCounter > 0 ; )
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
        else if(sentenceType[3] == 'M' && sentenceType[4] == 'C')
        {
          //We are now listening to GPRMC for the date
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
  //Since 2007 DST starts on the second Sunday in March and ends the first Sunday of November
  //Let's just assume it's going to be this way for awhile (silly US government!)
  //Example from: http://stackoverflow.com/questions/5590429/calculating-daylight-savings-time-from-only-date

  boolean dst = false; //Assume we're not in DST

  if(*month > 3 && *month < 11) dst = true; //DST is happening!

  byte DoW = day_of_week(*year, *month, *day); //Get the day of the week. 0 = Sunday, 6 = Saturday

  //In March, we are DST if our previous Sunday was on or after the 8th.
  int previousSunday = *day - DoW;

  if (*month == 3)
  {
    if(previousSunday >= 8) dst = true; 
  } 

  //In November we must be before the first Sunday to be dst.
  //That means the previous Sunday must be before the 1st.
  if(*month == 11)
  {
    if(previousSunday <= 0) dst = true;
  }
  if(dst == true) *hours = *hours + 1; //If we're in DST add an extra hour


  //Convert UTC hours to local current time using local_hour
  if(*hours < local_hour_offset)
  {
    //Go back a day in time
    *day = *day - 1;
    
    if(*day == 0)
    {
      //Jeesh. Figure out what month this drops us into
      *month = *month - 1;
      
      if(*month == 1) *day = 31;
      if(*month == 2) *day = 28; //Not going to deal with it
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
  *hours = *hours - local_hour_offset;
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
