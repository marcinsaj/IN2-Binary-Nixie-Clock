// IN-2 Binary Nixie Clock by Marcin Saj https://nixietester.com
// https://github.com/marcinsaj/IN2-Binary-Nixie-Clock
//
// Photon IN-2 Binary Nixie Clock Example
//
// This example demonstrates how to connect clock to Particle Cloud and 
// synchronizing (ones per day) RTC DS3231 time module with cloud time 
// 
// Serial monitor is required to debug synchronization
//
// Hardware:
// WiFi signal
// Particle Photon - https://docs.particle.io/photon/
// IN-2 Binary Nixie Clock - https://nixietester.com/project/in-2-binary-nixie-clock/
// Nixie Power Supply Module, 2 x Nixie Tube Driver V2 & RTC DS3231 module
// Nixie clock require 12V, 1A power supply
// Schematic IN-2 Binary Nixie Clock - work in progress
// Schematic Nixie Tube Driver V2 - work in progress
// Schematic Nixie Power Supply Module - work in progress
// DS3231 RTC datasheet: https://datasheets.maximintegrated.com/en/ds/DS3231.pdf

SYSTEM_THREAD(ENABLED);         // Enable automatic connection to WiFi and multi-threading 
#include <RTClibrary.h>

RTC_DS3231 rtcModule;           // RTC module library declaration

int EN_NPS = D5;                // Nixie Power Supply enable pin - "ON" = 0, "OFF" = 1 
int DIN_PIN = D2;               // Nixie driver (shift register) serial data input pin             
int CLK_PIN = D4;               // Nixie driver clock input pin
int EN_PIN = D3;                // Nixie driver enable input pin

// Set your Time Zone e.g. +4, -2, -4.5 (4.5 = 4 hours na 30 minutes) 
// https://en.wikipedia.org/wiki/List_of_time_zones_by_country
double const timeZone = +1;

// Choose Time Format 12/24 hour
int const timeHourFormat = 12;

// Each day at 3AM, the RTC DS3231 time will be synchronize with WiFi time
int const timeToSynchronizeTime = 3;
bool timeToSynchronizeTimeFlag = false;

// Bit numbers declaration for nixie tubes display
//           32  16   8   4   2   1
int H1[] = {26, 24, 45, 15, 17, 12};     // "1" Hours
int H0[] = {27, 25, 44, 14, 16, 13};     // "0" Hours
int M1[] = {34, 28, 43, 19, 10,  8};     // "1" Minutes
int M0[] = {35, 29, 42, 18, 11,  9};     // "0" Minutes
int S1[] = {36, 39, 41, 21,  2,  0};     // "1" Seconds
int S0[] = {37, 38, 40, 20,  3,  1};     // "0" Seconds

// 18 bits for "1", 18 bits for "0" - check clock schematic
// 8 bits for gaps - nixie drivers not connected outputs 
// 2 bits for nixie driver gaps - check driver schematic 

// Nixie Display bit array
bool nixieBitArray[46]; 

int currentHour = 0;
int currentMinute = 0;
int currentSecond = 0;

// Millis delay time variable 
unsigned long previous_millis = 0;
unsigned long current_millis = 0;


void setup() 
{
    Serial.begin(9600);
    delay(5000);                    // Wait for console opening   
    
    rtcModule.begin();              // Setup DS3231 RTC module
    
    pinMode(EN_NPS, OUTPUT);
    digitalWrite(EN_NPS, HIGH);     // Turn OFF nixie power supply module 

    pinMode(DIN_PIN, OUTPUT);
    digitalWrite(DIN_PIN, LOW);  
    
    pinMode(CLK_PIN, OUTPUT);
    digitalWrite(CLK_PIN, LOW);         
  
    pinMode(EN_PIN, OUTPUT);
    digitalWrite(EN_PIN, LOW);  
    
    Serial.println("############################################################");
    Serial.println("-------------- Photon IN-2 Binary Nixie Clock --------------");
    Serial.println("############################################################");
    Serial.println();

    Time.zone(timeZone);            // Set user time zone
    SynchronizeTimeWiFi();          // Time synchronization
    digitalWrite(EN_NPS, LOW);      // Turn ON nixie power supply module
}

void loop ()
{
    // Millis time start
    current_millis = millis();

    // Wait 1 second
    if(current_millis - previous_millis >= 1000)
    {
        // Get time from RTC and display on nixie tubes
        DisplayTime();
        previous_millis = current_millis;      
    }
    
    // Check if it's time to synchronize the time  
    if(timeToSynchronizeTimeFlag == true)
    {
        // Time synchronization
        SynchronizeTimeWiFi();    
    }
}

void SynchronizeTimeWiFi()
{
    Particle.syncTime();
    
    // Check what time format has been chosen 12/24
    if(timeHourFormat == 12)
    {
        currentHour = Time.hourFormat12();
    }
    else if (timeHourFormat == 24)
    {
        currentHour = Time.hour();
    }
    
    currentMinute = Time.minute();
    currentSecond = Time.second();
    
    // Year, Month and Day are not needed
    rtcModule.adjust(DateTime(0, 0, 0, currentHour, currentMinute, currentSecond));
    timeToSynchronizeTimeFlag = false;
    
    Serial.println();
    Serial.println("############################################################");
    Serial.println("------------------- Time synchronization -------------------");
    Serial.println("############################################################");
    Serial.println();
    delay(3000);
}

void DisplayTime()
{
    DateTime now = rtcModule.now();
    
    currentHour = now.hour();
    currentMinute = now.minute();
    currentSecond = now.second();

    NixieDisplay(currentHour, currentMinute, currentSecond);
    
    // Check if it's time to synchronize the time
    if(currentHour == timeToSynchronizeTime && currentMinute == 0 && currentSecond == 0 && Time.isAM())
    {
        timeToSynchronizeTimeFlag = true;  
    }
    
    Serial.print("Time: ");
    if(currentHour < 10)   Serial.print("0");
    Serial.print(currentHour);
    Serial.print(":");
    if(currentMinute < 10) Serial.print("0");
    Serial.print(currentMinute);  
    Serial.print(":");
    if(currentSecond < 10) Serial.print("0");
    Serial.print(currentSecond); 
    
    if(timeHourFormat == 12)
    {
        if(Time.isAM() == true) Serial.print(" AM");
        else if(Time.isPM() == true) Serial.print(" PM");
    }
    
    Serial.println();
}
 
void NixieDisplay(int hours, int minutes, int seconds)
{
    bool bitTime = 0;

    for (int i = 45; i >= 0; i--)
    {
        nixieBitArray[i] = 0;                           // Clear bit array   
    }
    
    for(int i = 5; i >= 0; i--)
    {
        bitTime = hours & 0x00000001;                   // Extraction of individual bits 0/1
        hours = hours >> 1;                             // Bit shift

        if(bitTime == 1) nixieBitArray[H1[i]] = 1;      // Set corresponding bit     
        else nixieBitArray[H0[i]] = 1;

        bitTime = minutes & 0x00000001;                 // Extraction of individual bits 0/1
        minutes = minutes >> 1;                         // Bit shift

        if(bitTime == 1) nixieBitArray[M1[i]] = 1;      // Set corresponding bit      
        else nixieBitArray[M0[i]] = 1;

        bitTime = seconds & 0x00000001;                 // Extraction of individual bits 0/1
        seconds = seconds >> 1;                         // Bit shift

        if(bitTime == 1) nixieBitArray[S1[i]] = 1;      // Set corresponding bit     
        else nixieBitArray[S0[i]] = 1;
    }
    
    ShiftOutData();
}

void ShiftOutData()
{
    // Ground EN pin and hold low for as long as you are transmitting
    digitalWrite(EN_PIN, 0); 
    // Clear everything out just in case to
    // prepare shift register for bit shifting
    digitalWrite(DIN_PIN, 0);
    digitalWrite(CLK_PIN, 0);  

    // Send data to the nixie drivers 
    for (int i = 45; i >= 0; i--)
    {    
        // Send current bit 
        if(nixieBitArray[i] == 1) digitalWrite(DIN_PIN, HIGH);
        else digitalWrite(DIN_PIN, LOW);     
        // Register shifts bits on upstroke of CLK pin 
        digitalWrite(CLK_PIN, 1);
        // Set low the data pin after shift to prevent bleed through
        digitalWrite(CLK_PIN, 0);  
    }   

    // Return the EN pin high to signal chip that it 
    // no longer needs to listen for data
    digitalWrite(EN_PIN, 1);
    
    // Stop shifting
    digitalWrite(CLK_PIN, 0);    
}
