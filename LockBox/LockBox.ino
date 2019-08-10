#include <LiquidCrystal.h> 
#include <Servo.h>
#include <CapacitiveSensor.h>

/* 
 * Arduino Knock Lock Box with Piezo/Capacitive Touch Alarm System and Servo Lock
 * Built Using Projects 11, 12, 13 from the Arduino Projects Book and 
 * Steve Hoefer's "Secret Knock Detecting Door Lock" from: https://www.instructables.com/id/Secret-Knock-Detecting-Door-Lock/
 * 
 * LCD uses pins 2 - 7 
 * button uses pin 8
 * capacitive touch sensor uses pins 9 - 10
 * servo uses pin 11 
 * LEDs use pins 12 - 13 
 * piezo uses analog pin A0
 * 
 * Tinkercad circuit link: https://www.tinkercad.com/things/7rCGxMmiFTF-knock-lock-box-/editel

 Project Notes:
 - COULD PROBABLY MAKE A KNOCK/TOUCH DETECTOR BASED ON TIMINGS AND VALUES OF CAPACITIVE SENSOR, HENCE NO KNOCK HEARING BY PEOPLE NEARBY
 - capacitive sensor with tinfoil behind cardboard does not work too well 
 - surface used for knocking affects whether or not a knock is registered 
 - piezo speaker being half covered by cardboard in my box design may affect if a knock is registered 
 */


//note: order of Arduino Pins that connect to LCD 
//is order of connection pins in direction of RS to D7
LiquidCrystal lcd(6,7,2,3,4,5);// create lCD instance
Servo myServo; // create servo instance
CapacitiveSensor capSensor = CapacitiveSensor(10,9);

// Pin definitions
int greenLED = 12; // Status LED
int redLED = 13; // Status LED
int piezo = A0; // Piezo sensor for both input and output 
int buttonPin = 8; // Button for knock input

// Constants (adjust as needed)
const int capacitanceSensingThreshold = 700; 
const int knockThreshold = 2;
const int rejectValue = 25; 
const int averageRejectValue = 15;
const int knockFadeTime = 150; 
const int lockTurnTime = 650;
const int maximumKnocks = 20;  
const int nonZeroArrayVal = 6;
const int knockComplete = 1500;
const int deBounceTime = 20;

// Variables.
int secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
int knockReadings[maximumKnocks];   // When someone knocks this array fills with delays between knocks.
int secretCodeNonZeroValues[nonZeroArrayVal];
int knockReadingsNonZeroValues[nonZeroArrayVal];
int knockSensorValue;           
boolean buttonPressed = false;   
boolean locked = true; 
int toggleState; 
int buttonState;
int lastButtonState;
long unsigned lastPress; 



void setup()
{
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  Serial.begin(9600);
  lcd.begin(16,2);   
  lcd.print("Knock Lock Box");
  lcd.setCursor(0,1);
  lcd.print("LOCKED");
  myServo.attach(11);
  myServo.write(0);
  digitalWrite(greenLED, LOW);
  digitalWrite(redLED, LOW);
}



void loop()
{
  buttonState = digitalRead(buttonPin);
  if (locked == true)
  {
    alarm();
    if (buttonState == 1) 
    {
      buttonPressed = true;
      pinMode(piezo, INPUT);
      Serial.println("Button Pressed");
    }
    else
    {
      Serial.println("Nothing Pressed");
      lcd.setCursor(0,1);
      lcd.print("LOCKED   ");
      buttonPressed = false;
    }
  }

  if (locked == false)
  {
    if (buttonState == 0)
    {
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED, HIGH);
    }
    if (buttonState == 1)
    {
      locked = true;
      buttonPressed = false;
      digitalWrite(greenLED, LOW);
      digitalWrite(redLED,LOW);
      lcd.setCursor(0,1);
      lcd.print("LOCKED   ");
      myServo.write(0);
      delay(500);
    }
  }

  knockSensorValue = analogRead(piezo);
  if (buttonPressed == true)
  {
    pinMode(piezo, INPUT);
    listenToSecretKnock();
  }
}



void alarm()
{
  long sensorValue = capSensor.capacitiveSensor(30);
  if(sensorValue > capacitanceSensingThreshold)
  {
    Serial.println(sensorValue);
    digitalWrite(redLED, HIGH);
    pinMode(piezo, OUTPUT);
    tone(piezo, 330);
    delay(500);
    digitalWrite(redLED,LOW);
    noTone(piezo);
    delay(100);
  }
}



boolean validateKnock()
{
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;

  for (int i=0;i<maximumKnocks;i++)
  {
    if (knockReadings[i] > 0)
    {
      currentKnockCount++;
    }
    if (secretCode[i] > 0)
    {            
      secretKnockCount++;
    }
    if (knockReadings[i] > maxKnockInterval)
    {
      maxKnockInterval = knockReadings[i];
    }
  }
  int j = 0;
  for (int i=maximumKnocks-1;i>=0;i--)
  {
    if (knockReadings[i] > 0)
    {
      knockReadingsNonZeroValues[j] = map(knockReadings[i],0, maxKnockInterval, 0, 100);;
      Serial.print("Knock");
      Serial.println(knockReadingsNonZeroValues[j]);
      j += 1;
    }
  }
  int k = 0;
  for (int i=maximumKnocks-1;i>=0;i--)
  {
    if (secretCode[i] > 0)
    {
      secretCodeNonZeroValues[k] = secretCode[i];
      Serial.print("Code");
      Serial.println(secretCodeNonZeroValues[k]);
      k += 1;
    }
  }

  // Mid Code Check 
  // Unsure why there are 12 values in each array
  Serial.println(sizeof(secretCodeNonZeroValues));
  for (int i=sizeof(secretCodeNonZeroValues);i>=0;i--)
  {
    Serial.print(secretCodeNonZeroValues[i]);
    Serial.print("-");
  }
  Serial.println();
  Serial.println(sizeof(knockReadingsNonZeroValues));
  for (int i=sizeof(knockReadingsNonZeroValues);i>=0;i--)
  {
    Serial.print(knockReadingsNonZeroValues[i]);
    Serial.print("-");
  }
  Serial.println();
  
  int totaltimeDifferences=0;
  int timeDiff=0;
  // if statement due to during first run, 0 knocks would lead to servo rotation "unlock"
  if (knockReadingsNonZeroValues[0] != 0)
  {
    // was receveing extra values in knockReadings
    // for loop below only compares the last values in the array to 
    // the secretCode values based on the length of secretCode
    // We count backwards since extra readings at beginning of the knockReadings
    for (int a = 0; a < nonZeroArrayVal; a++)
    {
      timeDiff = abs(knockReadingsNonZeroValues[a] - secretCodeNonZeroValues[a]);
      Serial.println("Compare:");
      Serial.println(knockReadingsNonZeroValues[a]);
      Serial.println(secretCodeNonZeroValues[a]);
//      if (timeDiff > rejectValue){ // Individual value too far out of whack
//        return false;
//      }
      totaltimeDifferences += timeDiff;
    }
    // It can also fail if the whole thing is too inaccurate.
    if (totaltimeDifferences/secretKnockCount>averageRejectValue){
      return false; 
    }
    return true;
  }
  else
  {
    return false;
  }
}





void listenToSecretKnock()
{
  alarm();
  digitalWrite(greenLED, HIGH);
  if (locked = true)
  {
    lcd.setCursor(0,1);
    lcd.print("Listening");
  }
  // reset listening array 
  for (int i = 0; i < maximumKnocks; i++)
  {
    knockReadings[i]=0;
    knockReadingsNonZeroValues[i]=0;
    secretCodeNonZeroValues[i]=0;
  }
  int currentKnock = 0;
  int now = millis();
  int startTime = millis();
  delay(knockFadeTime);
  do{
    // listen for knock or timeout 
    knockSensorValue = analogRead(piezo);
    if (knockSensorValue >= knockThreshold)
    {
      now = millis();
      knockReadings[currentKnock] = now-startTime;
      currentKnock++;
      startTime=now;
      // reset for next knock
      delay(knockFadeTime);
    }
    now = millis();
    // loop as long as each knock before timeout time (knockComplete) and in max num of knocks
  } while((now-startTime < knockComplete) && (currentKnock < maximumKnocks));
  // knock now recorded 

  if (validateKnock() == true)
  {
    myServo.write(90);
    tone(piezo,392,200);
    delay(150);
    tone(piezo,440,200);
    delay(150);
    tone(piezo,494,200);
    lcd.setCursor(0,1);
    lcd.print("UNLOCKED ");
    locked = false;
    buttonPressed = false;
  }
  else
  {
    pinMode(piezo, OUTPUT);
    for (int i = 0; i < 4; i++)
    {
      delay(100);
      tone(piezo,450,200);
      digitalWrite(redLED, HIGH);
      delay(100);
      digitalWrite(redLED,LOW);
    }
  }
}
