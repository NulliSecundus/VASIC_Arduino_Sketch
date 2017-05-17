#include <Statistic.h>
#include <SimpleTimer.h>
#include <LiquidCrystal.h>

SimpleTimer timer;
int readTimerID;
int writeTimerID;
char serBuff[10];
word avgTime = 1000;
const int readTime = 11;

const int loadCellPin1 = A0;
const int loadCellPin2 = A1;
word emptyWeightRead1 = 0;
word emptyWeightRead2 = 0;
word testWeightRead1 = 0;
word testWeightRead2 = 0;
float testWeightValue = 0;
const int sensorPin = A2;
const word sensorThreshold = 50;
int sensorValue;

Statistic loadCellVals1;
Statistic loadCellVals2;
float calibrationSlope1 = 0;
float calibrationSlope2 = 0;
float calibrationOffset1 = 0;
float calibrationOffset2 = 0;

const int LEDPin = 6;
const int IRLED = 7;
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
boolean greetingMessage;

int time_step = 1000 ; // every 1s
long time = 0;

void setup() {
  // initialize serial connection
  Serial.begin(9600);

  // initialize IR sensor pins for input
  pinMode(sensorPin, INPUT);
  pinMode(IRLED, OUTPUT);
  pinMode(LEDPin, OUTPUT);

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  greetingMessage = true;

  // change analog reference value from 5V to 2.56V
  //analogReference(INTERNAL2V56);
}

void loop() {
  // main loop: read serial messages and put the microcontroller into the proper mode
  // Print a message to the LCD and wait before clearing.
  while (greetingMessage) {
    lcd.setCursor(5, 0);
    lcd.print("VASIC");
    delay(5000);
    greetingMessage = false;
  }

  digitalWrite(IRLED, HIGH);

  lcd.clear();

  readBuffer();
  switch (serBuff[0]) {
    case 'T':
      sendChar('t');
      timeMode();
      break;
    case 'Z':
      sendChar('z');
      tareMode();
      break;
    case 'P':
      sendChar('p');
      calibrationMode();
      break;
    case 'M':
      sendChar('m');
      collectionMode();
  }
}

void timeMode() {
  lcdScreenPrint("Time Mode");

  while (true) {
    // read serial buffer and select the averaging time according to the index
    // sent by the host
    readBuffer();
    if (serBuff[0] == 'G') {
      switch (serBuff[1]) {
        case '0':
          avgTime = 500;
          sendChar('g');
          break;
        case '1':
          avgTime = 1000;
          sendChar('g');
          break;
        case '2':
          avgTime = 1500;
          sendChar('g');
          break;
        case '3':
          avgTime = 2000;
          sendChar('g');
          break;
        case '4':
          avgTime = 2500;
          sendChar('g');
          break;
        case '5':
          avgTime = 3000;
          sendChar('g');
          break;
        case '6':
          avgTime = 3500;
          sendChar('g');
          break;
        case '7':
          avgTime = 4000;
          sendChar('g');
          break;
        case '8':
          avgTime = 4500;
          sendChar('g');
          break;
        case '9':
          avgTime = 5000;
          sendChar('g');
          break;
      }
    } else if (serBuff[0] == 'H') {
      Serial.write('h');
      Serial.write('1'); // default average time value is 1 sec
      Serial.write('\r');
    } else if (serBuff[0] == 'X') {
      // remain in Time Mode until receive 'X' from host
      return;
    }
  }
  lcd.clear();
}

void tareMode() {
  lcdScreenPrint("Tare Mode");

  while (true) {
    // read the serial buffer and set the empty weight of the proper load cell
    // load cells are selected by 'A' or 'B' from the host
    readBuffer();
    switch (serBuff[0]) {
      case 'A':
        emptyWeightRead1 = analogRead(loadCellPin1);
        sendChar('a');
        break;
      case 'B':
        emptyWeightRead2 = analogRead(loadCellPin2);
        sendChar('b');
        break;
      case 'Q':
        // loop infinitely until 'Q' is received from the host to exit Tare Mode
        sendChar('q');
        return;
    }
  }
  lcd.clear();
}

void calibrationMode() {
  lcdScreenPrint("Calibration Mode");

  while (true) {
    // read buffer and store the number of bytes read
    // set the selected cell according to 'A' or 'B' from host
    // read 10 empty weight values and store the average in emptyWeightRead#
    // read 10 test weight value and store the average in testWeightRead#
    // read the exact test weight value sent from the host (details below)
    byte numBytes = readBuffer();
    int selectedCell;
    switch (serBuff[0]) {

      case 'A':
        selectedCell = 1;
        sendChar('a');
        break;
      case 'B':
        selectedCell = 2;
        sendChar('b');
        break;
      case 'C':
        {
          Statistic emptyBuff;
          if (selectedCell == 1) {
            emptyBuff.clear();
            for (int i = 0; i < 10; i++) {
              emptyBuff.add(analogRead(loadCellPin1));
            }
            //emptyWeightRead1 = analogRead(loadCellPin1);
            emptyWeightRead1 = emptyBuff.average();
            lcdScreenPrint("Left Side: ", 0, 0);
            lcdScreenPrint("Empty Weight", 0, 1);
            sendChar('c');
          } else if (selectedCell == 2) {
            emptyBuff.clear();
            for (int i = 0; i < 10; i++) {
              emptyBuff.add(analogRead(loadCellPin2));
            }
            //emptyWeightRead2 = analogRead(loadCellPin2);
            emptyWeightRead2 = emptyBuff.average();
            lcdScreenPrint("Right Side: ", 0, 0);
            lcdScreenPrint("Empty Weight", 0, 1);
            sendChar('c');
          }
          break;
        }
      case 'D':
        {
          lcdScreenPrint("Test Weight", 0, 0);
          Statistic testBuff;
          if (selectedCell == 1) {
            testBuff.clear();
            for (int i = 0; i < 10; i++) {
              testBuff.add(analogRead(loadCellPin1));
            }
            //testWeightRead1 = analogRead(loadCellPin1);
            testWeightRead1 = testBuff.average();
            sendChar('d');
          } else if (selectedCell == 2) {
            testBuff.clear();
            for (int i = 0; i < 10; i++) {
              testBuff.add(analogRead(loadCellPin2));
            }
            //testWeightRead2 = analogRead(loadCellPin2);
            testWeightRead2 = testBuff.average();
            sendChar('d');
          }
          break;
        }
      case 'S':
        // parse the test weight value (chars -> float)
        // calculate the appropriate calibration slope for linear approximation
        // m = (y2 - y1) / (x2 - x1), y1 = empty weight = 0
        // calculate the appropriate offset value based on the tare value and calibration slope
        // b = -1 * m * x1
        lcdScreenPrint("Test Weight Sent", 0, 0);
        testWeightValue = parseTestWeight(numBytes - 2);
        if (selectedCell == 1) {
          calibrationSlope1 = (float)testWeightValue / (testWeightRead1 - emptyWeightRead1);
          calibrationOffset1 = (-1) * calibrationSlope1 * emptyWeightRead1;
        } else if (selectedCell == 2) {
          calibrationSlope2 = (float)testWeightValue / (testWeightRead2 - emptyWeightRead2);
          calibrationOffset2 = (-1) * calibrationSlope2 * emptyWeightRead2;
        }
        sendChar('s');
        break;
      case 'Q':
        // exit Calibration Mode when receives 'Q' from host
        sendChar('q');
        return;
    }
  }
  lcd.clear();
}

void collectionMode() {
  lcdScreenPrint("Collection Mode");

  // sets up timers to call dataRead and dataWrite, but disables them immediately
  // to wait for IR sensor to be broken
  readTimerID = timer.setInterval(readTime, dataRead);
  timer.disable(readTimerID);
  writeTimerID = timer.setInterval(avgTime, dataWrite);
  timer.disable(writeTimerID);

  while (true) {
    // check for serial imput and parse using same strategy as readBuffer()
    // to check if 'Stop' button has been pressed in host ('K')

    if (millis() > time_step + time) {
      double dispReading1 = analogRead(loadCellPin1);
      dispReading1 = calibrationSlope1 * dispReading1 + calibrationOffset1;

      double dispReading2 = analogRead(loadCellPin2);
      dispReading2 = calibrationSlope2 * dispReading2 + calibrationOffset2;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Left: ");
      lcd.print(dispReading1);
      lcd.setCursor(0, 1);
      lcd.print("Right: ");
      lcd.print(dispReading2);
      time = millis();
    }

    if (Serial.available() > 0) {
      byte check = Serial.read();
      if (check == '*') {
        memset(serBuff, 0, (sizeof(serBuff) / sizeof(serBuff[0])));
        Serial.readBytesUntil('\\', serBuff, 10);
        //LED_Control(1);
      }
      if (serBuff[0] == 'K') {
        sendChar('k');
        return;
      }
    }
    // check the status of the IR sensor
    else if (getSensorStatus()) {
      // if the sensor is broken
      // check the status of the timers
      if (!timer.isEnabled(readTimerID)) {
        // clear values, send sensor status character to host, start timers
        loadCellVals1.clear();
        loadCellVals2.clear();
        sendChar('V');
        timer.enable(readTimerID);
        timer.enable(writeTimerID);
      }
      timer.run();
    }
    else {
      // if the sensor is made, check the timer status
      if (timer.isEnabled(readTimerID)) {
        // disable timers, send sensor status character to host, clear data
        timer.disable(readTimerID);
        timer.disable(writeTimerID);
        sendChar('W');
        loadCellVals1.clear();
        loadCellVals2.clear();
      }
    }
  }
  lcd.clear();
}

void LED_Control(int i) {
  if (i == 1) {
    digitalWrite(LEDPin, HIGH);
  } else {
    digitalWrite(LEDPin, LOW);
  }
}

void lcdScreenPrint(String str, int col, int row) {
  lcd.clear();
  lcd.setCursor(col, row);
  lcd.print(str);
}

void lcdScreenPrint(String str) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(str);
}

void dataRead() {
  // push load cell analog read values into dynamic array
  // called every readTime milliseconds
  loadCellVals1.add(analogRead(loadCellPin1));
  loadCellVals2.add(analogRead(loadCellPin2));
  return;
}

void dataWrite() {
  // calculate the average analog read value for each load cell
  // convert the analog read integer into a weight value using:
  // weightValue = m * x + b
  float cellAvg1 = loadCellVals1.average();
  cellAvg1 = calibrationSlope1 * cellAvg1 + calibrationOffset1;
  float cellAvg2 = loadCellVals2.average();
  cellAvg2 = calibrationSlope2 * cellAvg2 + calibrationOffset2;
  // convert the numbers to strings
  String toSend1, toSend2;
  averageToString(cellAvg1, toSend1);
  averageToString(cellAvg2, toSend2);
  // prepend the appropriate load cell identifier to the front of the string
  toSend1 = 'L' + toSend1;
  toSend2 = 'R' + toSend2;
  // send the strings to host
  //lcd.clear();
  Serial.print(toSend1);
  //lcd.setCursor(0, 0);
  //lcd.print(toSend1);
  Serial.print(toSend2);
  //lcd.setCursor(0, 1);
  //lcd.print(toSend2);

  // clear all data
  loadCellVals1.clear();
  loadCellVals2.clear();

  return;
}

void averageToString(float avg, String& averageString) {
  // use the String constructor to create a string from the avg value
  // retain 6 decimal places of the value in the string
  averageString = String(avg, 6);
  // remove everything after and including index 7,
  // leaving 6 digits + a decimal point
  averageString.remove(7);
  // append the carriage return char to the end of the string
  averageString += '\r';
  return;
}

void sendChar(byte toSend) {
  // when a character is read by the host, the '\r' char
  // indicates the end of the message.
  // Sends the given character to the host followed by a
  // carriage return character
  Serial.write(toSend);
  Serial.write('\r');
}

boolean getSensorStatus() {
  // read the status of the IR sensor input pin, compare to the threshold value
  // return true if sensor has been broken
  sensorValue = analogRead(sensorPin);
  if (sensorValue < sensorThreshold) {
    LED_Control(0);
    return true;
  } else {
    LED_Control(1);
    return false;
  }
}

byte readBuffer() {
  while (true) {
    // read all available bytes into the buffer after the check byte ('*')
    // up to but not including '\', which is the termination char for messages
    // from the host
    if (Serial.available() > 0) {
      byte check = Serial.read();
      if (check == '*') {
        memset(serBuff, 0, (sizeof(serBuff) / sizeof(serBuff[0])));
        return (Serial.readBytesUntil('\\', serBuff, 10) + 1);
      }
    }
  }
}

float parseTestWeight(int numBytes) {
  // read the test weight values into a char array
  // apply the atof (ASCII to float) function and return the result
  char array[8];
  memset(array, 0, (sizeof(array) / sizeof(array[0])));
  for (int i = 0; i < numBytes; i++) {
    array[i] = serBuff[i + 1];
  }
  return atof(array);
}
