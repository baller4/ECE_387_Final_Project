#include <SPI.h> //for Radio
#include <RF24.h> //for Radio
#include <MotorDriver.h> //for RC Car

//radio pins
#define CE_PIN  9
#define CSN_PIN 10

const byte thisSlaveAddress[5] = {'R','x','A','A','A'}; //address of the car
const byte masterAddress[5] = {'T','X','a','a','a'}; //address of the Etch A Sketch

RF24 radio(CE_PIN, CSN_PIN); //initialize the radio
MotorDriver motor; //initialize the motors

char carData[32]; //array that will hold incoming data
int maxImageLength = 30; //30 packets max, this number can increase or decrease,
                      //but limits the size of the image that can be recieved.
char carDirections[30][32]; //must be carDirections[maxImageLength][32]
int index = 0; //which array is being copied to

char doneSignal[5] = "DONE";
boolean done = false;

void setup() {

  //Begin Serial Communication for Debugging purposes
  Serial.begin(9600);
  Serial.println("SimpleRx Starting");

  //set up for RC Car motors
  motor.begin();

  //set up for RF transiever
  radio.begin();
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(107);
  radio.setPALevel(RF24_PA_MIN);
    
  radio.openWritingPipe(masterAddress);
  radio.openReadingPipe(1, thisSlaveAddress);
  radio.startListening();
}

void loop() {
  //while the Etch A Sketch (the master) is sending image data
  while(!done) {
    readData(); //read the data
  }
  delay(5000);
  send(); //send the done signal
  done = false; //reset for another image
}

void readData() {
  if (radio.available()) {
    
    //read the data and store it in carData
    radio.read(&carData, sizeof(carData));
    
    //show the data
    showData();

    //copy the read data into the 2D Array
    memcpy(carDirections[index], carData, 32);
    index = index + 1;
    
    if(carData[0] == 'B') { //'B' is the erase/restart signal from the master
      Serial.println("ERASE");
      index = 0; //"erase"
      done = true; //done with erasing
    }
    if(carData[0] == 'A') { //'A' is the execute signal from the master
      for(int i = 0; i < index-1; i++) { //for each array except the last which only contains image length
       for(int j = 2; j < 32; j = j + 2) {
         moveCar(carDirections[i][j-2], carDirections[i][j-1]);
         delay(200);
       }
      }
      stopCar();
      index = 0;
      done = true; //done without execution
    }
    if(index == maxImageLength) { //if the maxImageLenth is reached
      for(int i = 0; i < index; i++) {
       for(int j = 2; j < 32; j = j + 2) {
         moveCar(carDirections[i][j-2], carDirections[i][j-1]);
         delay(200);
       }
      }
      stopCar();
      index = 0;
      done = true; //done without execution
    }

  }
}

//showData() is mainly used for debugging
void showData() {
  Serial.print("Data received ");
  for(int i = 0; i < 32; i++) {
    Serial.print(carData[i], DEC);
  }
  Serial.println();
}

void send() {
  
  radio.stopListening();
  bool rslt;
  rslt = radio.write(&doneSignal, sizeof(doneSignal));
      // Always use sizeof() as it gives the size as the number of bytes.
      // For example if dataToSend was an int sizeof() would correctly return 2
  radio.startListening();

  //for debugging
  Serial.print("Data Sent ");
  Serial.print(doneSignal);
  if (rslt) {
    Serial.println("  Acknowledge received");
  }
  else {
    Serial.println("  Tx failed");
  }
}

//moveCar(int x, int y) method combines the use of verticalMove(int a), turn(int t), and stopCar()
void moveCar(int x, int y) {
  if((x != 0) && (y != 0)) {
    verticalMove(y);
    turn(x);
  }
  else if(x == 0 && y == 0) {
    stopCar();
  }
  else if(x == 0) {
    verticalMove(y);
  }
  else if(y == 0) {
    turn(x);
  }
}

void verticalMove(int a) {
  a = map(a, -2, 2, -100, 100);
  motor.speed(0, a); //move motor 0
  motor.speed(1, a); //move motor 1
  //Serial.println("Vertical Move");
}

void turn(int t) {
  if(t > 0) { //right turn, moves left motor
    t = map(t, -2, 2, 0, 100);
    motor.brake(0);
    motor.speed(1, t);
  }
  if(t < 0) { //left turn, turns right motor
    t = t * -1; //to match turn speed
    t = map(t, -2, 2, 0, 100);
    motor.speed(0, t);
    motor.brake(1);
  }
  //Serial.println("Turn");
}

void stopCar() {
  motor.brake(0); //stop motor 0
  motor.brake(1); //stop motor 1
  //Serial.println("Stop");
}
