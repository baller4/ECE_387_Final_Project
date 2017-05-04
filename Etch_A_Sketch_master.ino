#include <TFT.h> //for LCD library
#include <SPI.h> //for Radio
#include <RF24.h> //for Radio

//LCD Screen pins
#define cs   10
#define dc   9
#define rst  8

//radio pins
#define CE_RADIO  5
#define CSN_RADIO 6

TFT TFTscreen = TFT(cs, dc, rst); //initialize the LCD screen
RF24 radio(CE_RADIO, CSN_RADIO); //initialize the Radio

const byte masterAddress[5] = {'T','X','a','a','a'};
const byte slaveAddress[5] = {'R','x','A','A','A'};

char instructions[32]; //char array to send car instructions
int index = 0; //to keep track of instructions index
char drawingLength = 0; //number of instructions arrays sent
int maxImageLength = 30; //maxImageLength must match that of the slave

char doneSignal[5];

// initial position of the cursor
int xPos = TFTscreen.width() / 2;
int yPos = TFTscreen.height() / 2;

int xRelative;
int yRelative;

int erasePin = 2;
int sendPin = 4;

char execute = 'A'; //execute signal
char erase = 'B'; //erase signal

boolean drawing = true;
boolean done = false;

void setup() {
  Serial.begin(9600);
  
  //declare the erase and send pins as inputs to the arduino
  pinMode(erasePin, INPUT);
  pinMode(sendPin, INPUT);
  
  //setup the screen
  TFTscreen.begin();
  //make the background black
  TFTscreen.background(0, 0, 0); //background(blue, green, red)

  Serial.println("SimpleTx Starting");

  //RADIO SETUP
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(107);
  radio.setPALevel(RF24_PA_MIN);
  
  radio.openWritingPipe(slaveAddress);
  radio.openReadingPipe(1, masterAddress);
}

void loop() {
  
  while(drawing) {
    draw();
  }
  
  TFTscreen.background(0, 0, 255); //red to tell the user that the car is executing
  while(!done) {
    readData();
  }

  done = false;
  TFTscreen.background(0, 255, 0); //green to tell the user ready to draw again
  delay(2000);
  
  TFTscreen.background(0, 0, 0); //black
  drawing = true;
}

void draw() {
  //read the potentiometers on A0 and A1
  int xValue = analogRead(A0);
  int yValue = analogRead(A1);

  //map the read values to incremental values
  int mappedX = map(xValue, 0, 1023, 2, -2);
  int mappedY = map(yValue, 0, 1023, -2, 2);
  
  //update the x and y positions
  xPos = xPos + mappedX;
  yPos = yPos + mappedY;

  xRelative = mappedX * -1; //x increment relative to user
  yRelative = mappedY;

  //data only is saved if one or the other is not 0
  if((xRelative != 0) || (yRelative != 0)) {
    index = index + 2;
    instructions[index-2] = xRelative;
    instructions[index-1] = yRelative;
  }
  if(index == 32) { //when one packet is filled
      send(); //send the packet
      drawingLength = drawingLength + 1; //increase the drawing length
      index = 0; //reset the packet
  }

  //set the screen's boarders
  if (xPos > 159) {
    (xPos = 159);
  }
  if (xPos < 0) {
    (xPos = 0);
  }
  if (yPos > 127) {
    (yPos = 127);
  }
  if (yPos < 0) {
    (yPos = 0);
  }

  //draw the point
  TFTscreen.stroke(255, 255, 255); //(255, 255, 255) is white
  TFTscreen.point(xPos, yPos);

  //if the erasePin is pressed
  if(digitalRead(erasePin) == HIGH) {
    TFTscreen.background(0, 0, 0);
    index = 0;
    drawing = false;
    drawingLength = 0;
    sendChar(erase); //send erase signal
    reInitPos();
  }
  //if the send pin is pressed or the maxImageLength is reached
  if ((digitalRead(sendPin) == HIGH)||(drawingLength == maxImageLength)) {
    TFTscreen.background(0, 0, 0);
    index = 0;
    drawing = false;
    sendChar(execute); //send execute signal
    drawingLength = 0;
    reInitPos();
  }

  delay(33);
}

//resets x and y's postion to the middle of the screen
void reInitPos() {
  xPos = TFTscreen.width() / 2;
  yPos = TFTscreen.height() / 2;
}

void send() {
  
  radio.stopListening();
  
  bool rslt;
  rslt = radio.write(&instructions, sizeof(instructions));
      // Always use sizeof() as it gives the size as the number of bytes.
      // For example if dataToSend was an int sizeof() would correctly return 2
  radio.startListening();

  //for debugging
  Serial.print("Data Sent ");
  Serial.print(instructions);
  if (rslt) {
    Serial.println("  Acknowledge received");
  }
  else {
    Serial.println("  Tx failed");
  }
}

void sendChar(char c) {
  char cArr[] = {c};
  radio.stopListening();
  bool rslt;
  rslt = radio.write(&cArr, sizeof(cArr));
  radio.startListening();

  //for debugging
  Serial.print("Data Sent ");
  Serial.print(cArr);
  if (rslt) {
    Serial.println("  Acknowledge received");
  }
  else {
    Serial.println("  Tx failed");
  }
}

void readData() {
  if (radio.available()) {
    
    //read the data and store it in doneSignal
    radio.read(&doneSignal, sizeof(doneSignal));

    //show the read data
    showData();
  }
}

//mainly used for debugging
void showData() {
  Serial.print("Data received ");
  done = true;
  Serial.print(doneSignal);
  Serial.println();
}
