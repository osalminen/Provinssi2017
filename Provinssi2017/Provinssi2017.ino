/*
 * Arduino program for the Provinssi 2017 liike-energiateos project, tested on Arduino Uno, 
 * might work with other Arduinos.
 * 
 * Can handle up to ~1.1kHz frequencies, or around 130 bikes spinning at 500 RPM, 
 * for the RPM calculations, after that the unsigned int rolls over.
 * Minimum signal voltage is 2.7V for reliable operations
 * D13 can't be used for reading the RPM data for some reason
 * 
 * Length of sampleDur determines the accuracy of the device by 1/sampledur * 60 RPM
 * sampleDur also determines how 'unresponsive' the device is, Arduino reads serial data only after every loop so 
 * in theory you could 'hang' the device by making sampleDur way too big. This is why it's limited to 20 seconds,
 * which should be more than enough (resolution of 0.05 Hz or 3 RPM).
 * 
 * @version 1.4
 * @author Otto Salminen <otto.salminen@lut.fi>
 */

#include <avr/wdt.h> //Needed for watchdog
#include <avr/interrupt.h> //Needed for ISR_ALIASOF(), to compress the code by few lines
#include <EEPROM.h> //For using eeprom
#include <avr/pgmspace.h> //For moving char arrays to prog mem instead of sram
#define P(str) (strcpy_P(p_buffer, PSTR(str)), p_buffer) //Simplification of moving strings to progmem, saves the time of rewriting things into arrays and look-up tables
char p_buffer[100]; //Buffer for the string outputs

//Constants to tune things out
const byte hallPin[] = {2, 3, 4, 5, 6, 7, 8, A0, A1}; //Pins for the pci interrupts
const byte socketPin = 11; //Pin controlling the relay
const byte motorPin = 12; //Pin controlling the relay

//Variables of all sorts
bool relayState = false; //Toggle relay
bool statsState = false; //Shows info every 1 minute
byte sampleDur; //Defines the duration in seconds for collecting rpm data
byte hysteresis; //Multiplier of 60 RPM hysteresis
byte hattuvakio; //Used to scale up the measured RPM when measured from the pedals instead of the flywheel
byte NumberOfBikes; //Number of bikes
byte count;
unsigned int setRpm; //RPM/bike to trigger the relays
volatile unsigned int revs; //revs is double the value when using pinchange interrupt due to the event triggering on falling and rising edge
float totRpm; //Total rpm value for all of the bikes


//Setup function for the serial communications, all the pins and timers.
void setup(void) {
  cli(); //disable global interrupts
  
  //Serial communications
  Serial.begin(9600); //For bluetooth connection
  Serial.setTimeout(1000); //Timeout if ending character missing
  Serial.println(P("\nStarting up..."));

  //Pin setup
  pinMode(socketPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  //Take relay pins low just in case they are doing something weird
  digitalWrite(socketPin, relayState);
  digitalWrite(motorPin, relayState); 
  pinMode(LED_BUILTIN, OUTPUT); //Just to turn the led off <- might be the reason why D13 didn't work as intended
  
  //PCI setup
  byte NumberOfElements = sizeof(hallPin)/sizeof(hallPin[0]); //Get the amount of pins set up
  //Init pins connected to hall sensors
  for (byte i = 0; i < NumberOfElements; i++){
    pinSetup(hallPin[i]);
    digitalWrite(hallPin[i], HIGH);
    }
    
  //Timers and key variables
  variableSetup(); //Load variable values from the memory
  watchdogSetup(); //Setup watchdog for automatic reset on hang
  timerSetup(); //Setup timer1
  Serial.println(P("Setup completed"));

  sei(); //enable global interrupts
}


//PCI setup
void pinSetup(byte pin) {
  *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // find the correct registry and enable pin as input
  PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear interrupt flags for the pin
  PCICR  |= bit (digitalPinToPCICRbit(pin)); // enables the pin as pci
}


//Setup variables
void variableSetup(void){
  //If some random values in EEPROM, load defaults. Really bad at checking for errors really, don't trust this.
  if (EEPROM.read(3) > 20){
    eeDefault();
  }
  else{
    Serial.println(P("Variables loaded from memory:"));
    setRpm = eeRead(1);
    NumberOfBikes = eeRead(2);
    sampleDur = eeRead(3);
    hysteresis = eeRead(4);
    hattuvakio = eeRead(5);
  }
}


//Setup watchdog timer
void watchdogSetup(void){
  wdt_reset(); // Reset the wdt timer
  WDTCSR |= (1<<WDCE) | (1<<WDE); //Configuration mode
  //Interrupt, then go to system reset mode, 8000msec timeout
  WDTCSR = (1<<WDIE) | (1<<WDE) | (1<<WDP3) | (0<<WDP2) | (0<<WDP1) | (1<<WDP0);
}


// Setup Timer1 for interrupt every 4000 msec
void timerSetup(void){
  TCCR1A = 0;     //TCCR1A register to 0
  TCCR1B = 0;     //TCCR1B register to 0
  OCR1A = 62500; //compare match register to desired timer count
  //turn on CTC mode, CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << WGM12) | (1 << CS10) | (1 << CS12);
  TIMSK1 |= (1 << OCIE1A); //enable timer compare interrupt
}

//Functions for playing around with EEPROM
//Read values from EEPROM
byte eeRead(byte addr){
  byte value = EEPROM.read(addr);
  Serial.print(addr);
  Serial.print(P("\t"));
  Serial.print(value, DEC);
  Serial.println();
  delay(100);
  return value;
}

//Write value to EEPROM
void eeWrite(byte addr, byte value){
  EEPROM.update(addr, value);
 }

//Default 'safe' values for the parameters
void eeDefault(void){
    Serial.println(P("\nFlashing memory with default values"));
    EEPROM.update(1, 100);
    EEPROM.update(2, 9);
    EEPROM.update(3, 1);
    EEPROM.update(4, 2);
    EEPROM.update(5, 3);
}


//Interrupt vectors
ISR (PCINT1_vect) {revs++;} //PCI for A0 to A5, connect bikes with sensors on flywheel to these
ISR (PCINT2_vect) {revs += hattuvakio;}  // PCI for D0 to D7, connect bikes with sensors on pedals to these
ISR (PCINT0_vect, ISR_ALIASOF(PCINT2_vect)); // PCI for D8 to D13, connect bikes with sensors on pedals to these

//Watchdog event
ISR (WDT_vect){ 
  Serial.println(P("Something went wrong"));
  eeDefault(); //Load default values before restarting
  Serial.println(P("Restarting..."));
  } 
 
//Info screen every minute just for fun
ISR (TIMER1_COMPA_vect){ 
  Serial.println("Ping");
  if (count < 15){ //
    count++;
  }
  else if(statsState){
    stats();
    count = 0;
  }
}


//Main loop for the program
void loop(void) {
  //Start taking in sample data for the rpm calculations
  revs = 0; //reset counter
  wait(sampleDur); //Close enough for 1 second delay

  //Calculate RPM
  cli(); //Disable global interrupts during calculations
  totRpm = ((revs/2)/(float)sampleDur)*60.0; //super mathematics
  sei(); //Enable global interrupts

  //Relay operations
  if (totRpm/NumberOfBikes >= setRpm){
    //Turn the relay on
    relayState = true;
  }
  else if (relayState && ((setRpm * NumberOfBikes) - totRpm) <= (hysteresis*60)){
    //Keep the relay on within hysteresis
    //Do nothing
  }
  else{
    //Turn the relay off
    relayState = false;
  }
  
  digitalWrite(socketPin, relayState);
  digitalWrite(motorPin, relayState);
}


//Function for waiting periods without watchdog timing out
void wait(byte laps){
  for (byte i = 0; i < laps; i++){
    delay(1000);
    wdt_reset(); //reset watchdog timer every 1 second to make waiting scalable over 8 seconds (not needed, but it's still there)
  }
}
