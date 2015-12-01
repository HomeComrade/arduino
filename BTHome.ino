//#define UNO
#define MICRO

#include <Servo.h>
#include <Adafruit_NeoPixel.h>

#ifdef UNO
  #include <SoftwareSerial.h>
  //Bluetooth serial
  //SoftwareSerial btSerial(2, 3); // RX, TX
#endif

#include <avr/pgmspace.h>

#define byte uint8_t

#include "Arrays.h"

//Pin to clear the register
const int CLEAR_PIN = 11;
//Pin connected to clock pin (SH_CP) of 74HC595
const int CLOCK_PIN = 10;
//Pin connected to latch pin (ST_CP) of 74HC595
const int LATCH_PIN = 9;
//Pin connected to Data in (DS) of 74HC595
const int DATA_PIN = 8;

const int COLUMN_COUNT = 24;
const int ROW_COUNT = 15;
const int LETTER_WIDTH = 4;
const int BUS_SCHEDULES_COUNT = 2;
const int BUSSES_SHOWN = 2;

char BUS_TIMES[BUS_SCHEDULES_COUNT][BUSSES_SHOWN][4] = {
  {
    { '1', '2', '3', '4' },
    { '5', '6', '7', '8' },
  },
  {
    { '9', '0', '1', '2' },
    { '3', '4', '5', '6' },
  },
};

const char BUS_SCHEDULE_CHARS[BUS_SCHEDULES_COUNT] = {
  'b', 'v'
};

byte ROW_STATE[COLUMN_COUNT] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

byte FRAME[ROW_COUNT][COLUMN_COUNT] = {
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
};

//BT message information
String BUS_RECEIVED_STRING = "";
String BUTTON_RECEIVED_STRING = "";
String LIGHTS_RECEIVED_STRING = "";
const char BUS_MESSAGE_CHAR = '!';
const char BUTTON_MESSAGE_CHAR = '#';
const char LIGHTS_MESSAGE_CHAR = ']';
const int MESSAGE_TYPE_BUS = 1;
const int MESSAGE_TYPE_BUTTON = 2;
const int MESSAGE_TYPE_LIGHTS = 3;
const int BUS_MESSAGE_LENGTH = 23;
const int BUTTON_MESSAGE_LENGTH = 1;
const int LIGHTS_MESSAGE_LENGTH = 11;
const int MAX_MESSAGE_LENGHT = BUS_MESSAGE_LENGTH;

int CURRENT_MESSAGE_TYPE = MESSAGE_TYPE_BUS;

boolean TIME_COLON_BLINK = false;
long COLON_BLINK_TIMER = millis();

int CURRENT_BUS_SCHEDULE = 0;
long BUS_SCHEDULE_TIMER = millis();

//Servo variables
const char BUTTON_PRESS_TYPE_CHAR_SINGLE = '1';
const char BUTTON_PRESS_TYPE_CHAR_HOLD = '2';
const int BUTTON_PRESS_TYPE_NULL = 0;
const int BUTTON_PRESS_TYPE_SINGLE = 1;
const int BUTTON_PRESS_TYPE_HOLD = 2;
//const int SERVO_DIRECTION_FORWARD = 1;
//const int SERVO_DIRECTION_BACKWARD = 2;

const int SERVO_PIN = 12;
const int SERVO2_PIN = 6;

Servo servo1;
Servo servo2;

int CURRENT_BUTTON_TYPE = BUTTON_PRESS_TYPE_NULL;
//int CURRENT_SERVO_DIRECTION = SERVO_DIRECTION_FORWARD;
boolean SERVO_PUSHING = false;
int SERVO_NEXT_MOVE_TIME = 0;
long SERVO_TIMER = millis();

//NeoPixel variables
const int NEOPIXEL_PIN = 7;
const int NEOPIXEL_COUNT = 27;

int NEOPIXEL_COLORS[3] = { 0, 0, 0 };

Adafruit_NeoPixel neoPixel = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  //set pins to output because they are addressed in the main loop
  pinMode(CLEAR_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);  
  pinMode(CLOCK_PIN, OUTPUT);
  
  Serial.begin(9600);
  
  #ifdef UNO
    btSerial.begin(9600);
  #endif
  #ifdef MICRO
    Serial1.begin(9600);
  #endif
  
  // Always start by sentting SRCLR high
  digitalWrite(CLEAR_PIN, HIGH);
  
  // Setup servo
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(SERVO2_PIN, OUTPUT);
  servo1.attach(SERVO_PIN);
  servo2.attach(SERVO2_PIN);
  
  // Setup NeoPixel
  neoPixel.begin();
  showNeopixelLights(); // Initialize all pixels to 'off'
}

void loop() {
  long currentTime = millis();
  
  if ((currentTime - BUS_SCHEDULE_TIMER) > 5000) {
    BUS_SCHEDULE_TIMER = currentTime;
    CURRENT_BUS_SCHEDULE++;
    
    if (CURRENT_BUS_SCHEDULE >= BUS_SCHEDULES_COUNT) {
      CURRENT_BUS_SCHEDULE = 0;
    }
  }
  
  setupBusTimesFrame();
  
  //printBusTimes();
  //printFrame();
  //delay(4000);
  
  for (int row = 0; row < ROW_COUNT; row++) {
    clearRowState();
    
    for (int column = 0; column < COLUMN_COUNT; column++) {
      ROW_STATE[column] = FRAME[row][column];
    }
    
    writeRow(row);
  }
  
  //Last, we want to even out the brightness of the rows. We clear the last row to make sure
  //it doesnt hold a charge longer than the other rows since next we have to pull strings
  //from BT and that can take a bit of time
  //ADDITIONAL:: There is a strange bug that requires clearing the last row twice. Whatever.
  clearRowState();
  writeRow(14);
  writeRow(14);
  
  #ifdef UNO
  if (btSerial.available()) {
  #endif
  #ifdef MICRO
  if (Serial1.available()) {
  #endif
    //Serial.println("Received data");
    
    #ifdef UNO
    while (btSerial.available()) {
    #endif
    #ifdef MICRO
    while (Serial1.available()) {
    #endif
      
      #ifdef UNO
      char inChar = (char)btSerial.read();
      #endif
      #ifdef MICRO
      char inChar = (char)Serial1.read();
      #endif
      
      if (inChar == BUS_MESSAGE_CHAR) {
        CURRENT_MESSAGE_TYPE = MESSAGE_TYPE_BUS;
        continue;
      }
      else if (inChar == BUTTON_MESSAGE_CHAR) {
        CURRENT_MESSAGE_TYPE = MESSAGE_TYPE_BUTTON;
        continue;
      }
      else if (inChar == LIGHTS_MESSAGE_CHAR) {
        CURRENT_MESSAGE_TYPE = MESSAGE_TYPE_LIGHTS;
        continue;
      }
      
      if (CURRENT_MESSAGE_TYPE == MESSAGE_TYPE_BUS) {
        if (BUS_RECEIVED_STRING.length() > BUS_MESSAGE_LENGTH) {
          BUS_RECEIVED_STRING = "";
        }
        else {
          BUS_RECEIVED_STRING += inChar;
        }
      }
      else if (CURRENT_MESSAGE_TYPE == MESSAGE_TYPE_BUTTON) {
        if (BUTTON_RECEIVED_STRING.length() > BUTTON_MESSAGE_LENGTH) {
          BUTTON_RECEIVED_STRING = "";
        }
        else {
          BUTTON_RECEIVED_STRING += inChar;
        }
      }
      else if (CURRENT_MESSAGE_TYPE == MESSAGE_TYPE_LIGHTS) {
        if (LIGHTS_RECEIVED_STRING.length() > LIGHTS_MESSAGE_LENGTH) {
          LIGHTS_RECEIVED_STRING = "";
        }
        else {
          LIGHTS_RECEIVED_STRING += inChar;
        }
      }
    }
    
    //Serial.print("BUS_RECEIVED_STRING: ");
    //Serial.print(BUS_RECEIVED_STRING);
    //Serial.print(" BUTTON_RECEIVED_STRING: ");
    //Serial.print(BUTTON_RECEIVED_STRING);
    //Serial.print(" LIGHTS_RECEIVED_STRING: ");
    //Serial.println(LIGHTS_RECEIVED_STRING);
    
    if (BUS_MESSAGE_LENGTH == BUS_RECEIVED_STRING.length()) {
      int currentBusStop = 0;
      int currentBusPosition = 0;
      int currentBusCharPosition = 0;
      
      for (int i = 0; i < BUS_MESSAGE_LENGTH; i++) {
        char messageChar = BUS_RECEIVED_STRING.charAt(i);
        
        if (isDigit(messageChar)) {
          BUS_TIMES[currentBusStop][currentBusPosition][currentBusCharPosition] = messageChar;
          currentBusCharPosition++;
        }
        else {
          if (':' == messageChar) {
            //nothing to do. this seperates bus number from minutes till arrival
          }
          else if (',' == messageChar) {
            currentBusPosition++;
            currentBusCharPosition = 0;
          }
          else if (';' == messageChar) {
            currentBusStop++;
            currentBusPosition = 0;
            currentBusCharPosition = 0;
          }
        }
      }
      
      BUS_RECEIVED_STRING = "";
    }
    
    if (BUTTON_MESSAGE_LENGTH == BUTTON_RECEIVED_STRING.length()) {
      boolean foundButtonType = false;
      
      if (BUTTON_RECEIVED_STRING.charAt(0) == BUTTON_PRESS_TYPE_CHAR_SINGLE) {
        CURRENT_BUTTON_TYPE = BUTTON_PRESS_TYPE_SINGLE;
        foundButtonType = true;
      }
      else if (BUTTON_RECEIVED_STRING.charAt(0) == BUTTON_PRESS_TYPE_CHAR_HOLD) {
        CURRENT_BUTTON_TYPE = BUTTON_PRESS_TYPE_HOLD;
        foundButtonType = true;
      }
      
      if (foundButtonType) {
        SERVO_TIMER = currentTime;
        SERVO_PUSHING = true;
        SERVO_NEXT_MOVE_TIME = 0;
      }
      
      BUTTON_RECEIVED_STRING = "";
    }
    
    if (LIGHTS_MESSAGE_LENGTH == LIGHTS_RECEIVED_STRING.length()) {
      //do work to extract the RGB numbers and show the strip
      String tmpColor = "";
      int tmpColorPosition = 0;
      
      for (int i = 0; i < LIGHTS_RECEIVED_STRING.length(); i++) {
        char messageChar = LIGHTS_RECEIVED_STRING.charAt(i);
        
        if (isDigit(messageChar)) {
          tmpColor += messageChar;
        }
        else {
          if (',' == messageChar) {
            tmpColor = "";
            tmpColorPosition++;
            
            if (tmpColorPosition > 2) {
              tmpColorPosition = 0;
            }
          }
        }
        
        if (tmpColor.length() == 3) {
            NEOPIXEL_COLORS[tmpColorPosition] = tmpColor.toInt();
        }
      }
      
      showNeopixelLights();
      
      LIGHTS_RECEIVED_STRING = "";
    }
  }
  
  if (CURRENT_BUTTON_TYPE != BUTTON_PRESS_TYPE_NULL) {
    if ((currentTime - SERVO_TIMER) > SERVO_NEXT_MOVE_TIME) {
      SERVO_TIMER = currentTime;
      SERVO_NEXT_MOVE_TIME = 1000;//only move every 10 millis by default
      
      if (CURRENT_BUTTON_TYPE == BUTTON_PRESS_TYPE_HOLD) {
        SERVO_NEXT_MOVE_TIME = 5000;//5 seconds
      }
      
      SERVO_PUSHING = !SERVO_PUSHING;
      
      if (!SERVO_PUSHING) {
        CURRENT_BUTTON_TYPE = BUTTON_PRESS_TYPE_NULL;//if we have made it back to 0 position, our "press" is complete
      }
    }
  }
  
  //always push our servos so they stay in position
  if (SERVO_PUSHING) {
    moveServos(true);
  }
  else {
   moveServos(false);
  }
}

void printBusTimes() {
  for (int i = 0; i < BUS_SCHEDULES_COUNT; i++) {
    for (int y = 0; y < BUSSES_SHOWN; y++) {
      for (int z = 0; z < 4; z++) {
        Serial.print(BUS_TIMES[i][y][z]);
      }
      Serial.println("");
    }
    Serial.println("");
  }
}

void printFrame() {
  for (int row = 0; row < ROW_COUNT; row++) {
    for (int column = 0; column < COLUMN_COUNT; column++) {
      Serial.print(FRAME[row][column]);
    }
    Serial.println();
  }
  Serial.println();
}

char intToChar(int i) {
  return (char)(((int)'0')+i);
}

void setupBusTimesFrame() {
  for (int row = 0; row < ROW_COUNT; row++) {
    int stateRow = 0;
    
    if (row < 7) {
      stateRow = row;
    }
    else {
      stateRow = row - 8;
    }
    
    for (int column = 0; column < COLUMN_COUNT; column++) {
      byte state = 0;
      int slotPosition = (row < 8 ? 0 : 1);
      
      if (column > 9 && column < 14) {
        if (row > 4 && row < 10) {
          state = getState(BUS_SCHEDULE_CHARS[CURRENT_BUS_SCHEDULE], (row - 3), (column - 10));
        }
      }
      else if (7 == row) {//row 7 is fucked up and should be empty outside of the schedule letter
        continue;
      }
      else if (4 == column || 9 == column || 14 == column || 19 == column) {
        state = 0;
      }
      else if (column < 4) {
        state = getState(BUS_TIMES[CURRENT_BUS_SCHEDULE][slotPosition][0], stateRow, column);
      }
      else if (column < 9) {
        state = getState(BUS_TIMES[CURRENT_BUS_SCHEDULE][slotPosition][1], stateRow, (column - 5));
      }
      else if (column < 19) {
        state = getState(BUS_TIMES[CURRENT_BUS_SCHEDULE][slotPosition][2], stateRow, (column - 15));
      }
      else if (column < 25) {
        state = getState(BUS_TIMES[CURRENT_BUS_SCHEDULE][slotPosition][3], stateRow, (column - 20));
      }
      
      //assign to the frame
      FRAME[row][column] = state;
    }
  }
}

byte getState(char letter, int row, int column) {
  byte state = 0;
  
  if (letter == '0') {
    state = pgm_read_byte(&CHARS[0][row][column]);
  }
  else if (letter == '1') {
    state = pgm_read_byte(&CHARS[1][row][column]);
  }
  else if (letter == '2') {
    state = pgm_read_byte(&CHARS[2][row][column]);
  }
  else if (letter == '3') {
    state = pgm_read_byte(&CHARS[3][row][column]);
  }
  else if (letter == '4') {
    state = pgm_read_byte(&CHARS[4][row][column]);
  }
  else if (letter == '5') {
    state = pgm_read_byte(&CHARS[5][row][column]);
  }
  else if (letter == '6') {
    state = pgm_read_byte(&CHARS[6][row][column]);
  }
  else if (letter == '7') {
    state = pgm_read_byte(&CHARS[7][row][column]);
  }
  else if (letter == '8') {
    state = pgm_read_byte(&CHARS[8][row][column]);
  }
  else if (letter == '9') {
    state = pgm_read_byte(&CHARS[9][row][column]);
  }
  else if (letter == 'a') {
    state = pgm_read_byte(&CHARS[10][row][column]);
  }
  else if (letter == 'b') {
    state = pgm_read_byte(&CHARS[11][row][column]);
  }
  else if (letter == 'v') {
    state = pgm_read_byte(&CHARS[12][row][column]);
  }
  
  return state;
}

void printRowState() {
  for (int i = 0; i < COLUMN_COUNT; i++) {
    Serial.print(ROW_STATE[i]);
  }
  Serial.println();
}

void clearRowState() {
  for (int i = 0; i < COLUMN_COUNT; i++) {
    ROW_STATE[i] = 0;
  }
}

//used only if we are blinking row by row only
void writeRow(int row) {
  byte bitsToSend0 = 0;
  byte bitsToSend1 = 0;
  byte bitsToSend2 = 0;
  byte bitsToSend3 = 0;
  byte bitsToSend4 = 0;
  
  if (row <= 7) {
    bitWrite(bitsToSend1, row, HIGH);
  }
  else {
    bitWrite(bitsToSend0, (row - 8), HIGH);
  }
  
  for (int i = 0; i < COLUMN_COUNT; i++) {
    if (0 == ROW_STATE[i]) {
      continue;
    }
    
    if (i <= 7) {
      bitWrite(bitsToSend4, i, HIGH);
    }
    else if (i <= 15) {
      bitWrite(bitsToSend3, (i - 8), HIGH);
    }
    else if (i <= 23) {
      bitWrite(bitsToSend2, (i - 16), HIGH);
    }
  }
  
  digitalWrite(LATCH_PIN, LOW);
  
  // shift the bits out
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bitsToSend0);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bitsToSend1);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bitsToSend2);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bitsToSend3);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, bitsToSend4);
  
  // turn on the output so the LEDs can light up:
  digitalWrite(LATCH_PIN, HIGH);
}

void moveServos(boolean push) {
  if (push) {
    Serial.println("Move servo, push = true");
    //push = 120
    servo1.write(110);
    //900 = push
    servo2.write(100);
  }
  else {
    Serial.println("Move servo, push = false");
    //resting = 170
    servo1.write(200);
    //60 = resting
    servo2.write(60);
  }
  //delay(10);
}

void showNeopixelLights() {
  for (int i = 0; i < NEOPIXEL_COUNT; i++) {
    neoPixel.setPixelColor(i, NEOPIXEL_COLORS[0], NEOPIXEL_COLORS[1], NEOPIXEL_COLORS[2]);
  }
  
  neoPixel.show();
}
