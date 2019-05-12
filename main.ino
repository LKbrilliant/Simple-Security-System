/*	Code by  		: 	Anuradha Gunawardhana(LKBrilliant)
	version			  :	  1.1.190214
 	Date    		  :   12.02.2019
 	μC 				    :	  ATMEGA168(Arduino Pro Mini)
 	GSM_module		:	  SIM800L
	Motion_sensor	:	  RCWL-0516

 	main Function : > Check the credit balance using USSD at the start
            		  > If the credit value is in the credit limit RANGE a warning message is sent to the owner
			  		      > When an intruder was detected, wait for the 'loading period' to complete
			  		      > If the device not disarmed during the 'loading period', sound the alarm for predetermined time
                  ...........
*/

#include <SoftwareSerial.h>

#define dataPin         4
#define clockPin        6
#define latchPin        5
#define interruptPin    2
#define alarmPin       12
#define batteryPin     A0

#define crdtLimMax     20               // If the credit value is between the limit range a message is sent to the owner
#define crdtLimMin     10

#define waitSeconds    20				        // seconds wait before trigger the alarm
#define alarmMinutes	 10				        // Sound alarm for this period

SoftwareSerial SIM800L(10, 11);         //Software Serial object (Rx,Tx)

String number = "**********";
String _buffer;                         // Store the string received  form the software serial interface
int balance;
unsigned long timmer = 0;
boolean detected = 0;
boolean btryMsgSent = 0;
byte led;

// *************************LED bar patterns*************************//

byte ptn1[] = {B10000000, B01000000, B00100000, B00010000, B00001000,
               B00000100, B00000010, B00000001, B00000000, };
byte ptn2[] = {B00000000, B10000000, B11000000, B11100000, B11110000, 
               B11111000, B11111100, B11111110, B11111111, };
byte ptn3[] = {B00000000, B00011000, B00100100, B01000010, B10000001};

//*******************************************************************//

void setup() {
  pinMode(alarmPin, OUTPUT);
  digitalWrite(alarmPin, LOW);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);

  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), interruptEvent, RISING);
  
  led = B00000001;
  regWrite();

//  Serial.begin(9600);

  SIM800L.begin(9600);
  delay(20000);
  SIM800L.write("AT+CUSD=1,\"#456#\"\r\n");                       // Send USSD for checking balance(Change #456# for you carrier)
  delay(5000);
 
 // ******************String manupilations to find the available credit balance**********************//
  _buffer = readSerial();                                         // Original buffer
  _buffer.remove(68);                                             // Only get integer value of the balance
  _buffer.remove(0, 66);                                          // String value
  balance = _buffer.toInt();                                      // Integer Value
 // *************************************************************************************************//
  
  if (crdtLimMin <= balance && balance <= crdtLimMax) {           // Send SMS with balanceMessage if low on credit
    String msg = "WARNING : Your available balance is Rs.";
    msg.concat(balance);
    sendMessage(msg);
  }
  delay(2000);
//  SIM800L.write("AT+CMGD=1,4\r\n");                             // Delete all the messages on the memory

  armingPtn(30);
  flash(10);  //
  led = B00000001;
  regWrite();
  detected = 0;
}

void loop() {
  if (detected == 1) {
    flash(10);                                      // wait for the loading pattern to complete
    loadingPtn1(5);
    loadingPtn2();
    SIM800L.print("ATD "+ number + ";\r\n");        // Dial the owner
    triggerAlarm(alarmMinutes);				              // Alarm trigger for minutes
  }
  batteryCheck();
}

String readSerial() {
  int timeout = 0;
  while (!SIM800L.available() && timeout < 10000) {
    delay(1);
    timeout++;
  }
  if (SIM800L.available()) {
    delay(3000);
    return SIM800L.readString();
  }
  else return "USSD Failed";
}

void sendMessage(String message) {
  SIM800L.write("AT+CMGF=1\r\n");             		  // Set SMS format to ASCII
  delay(1000);
  SIM800L.print("AT+CMGS=\"" + number + "\"\r\n");  // New message send command
  delay(1000);
  SIM800L.println(message);                         // SMS content
  delay(1000);
  SIM800L.write((char)26);                          // Send Ctrl+Z
}

void interruptEvent() {
  detected = 1;
  timmer = millis();
}

void armingPtn(byte times){
  for(byte u = 0; u < times; u++){
    for(byte i = 0; i <= 4; i++){
      led = ptn3[i];
      regWrite();
      delay(200);
    }
  }
}

void loadingPtn1(byte times){
  for(byte u = 0; u < times; u++){
    for(byte i = 0; i <= 8; i++){
      led = ptn1[i];
      regWrite();
      delay(100);
    }
  }
}

void loadingPtn2() {
  for(byte i = 0; i <= 8; i++){
      led = ptn2[i];
      regWrite();
      delay(1000); 
    for(byte u = 0; u < 10; u++){
      led = ptn2[i+1];
      regWrite();
      delay(500 - 51*u);
      led = ptn2[i];
      regWrite();
      delay(500 - 51*u); 
    }
  }
}

void triggerAlarm(int minutes) {
  while (millis() < (timmer + (minutes * 60000))) {       // (*60000)the variable time was set by the ISR
    digitalWrite(alarmPin, HIGH);
    flash(5);
  }
  digitalWrite(alarmPin, LOW);
  led = B00000001;
  regWrite();
  detected = 0;
}

void regWrite(){                                          // Shiftregister for LED bar
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, led);
  digitalWrite(latchPin, HIGH);
}

void flash(byte times){                                   // Flashing all LEDs
  for(byte i = times; i > 0; i--){
    led = B11111111;
    regWrite();
    delay(100);
    led = B00000000;
    regWrite();
    delay(100);
  }    
}

void batteryCheck(){
   if((analogRead(batteryPin) < 300) && btryMsgSent == 0){      // 300 ~ (3-2.9V)
     sendMessage("Warning : Battery Low");
     btryMsgSent = 1;
   }
}
