// Allgemeine Deklaration
#include <Arduino.h>

// Start Variablendeklaration
int TimerStart = 0;           // Merker fuer Timer
int Wertigkeit = 3;           // Wertigkeit Temperaturauswertung x : 1
unsigned long startTime ;     // Timer Startwert 
unsigned long elapsedTime ;   // Timer abgelaufene Zeit
unsigned int WaitTime = 10;   // Wartezeit, bis der Luefter nach Ausschalten der HDD ausgeschaltet wird
int TempStart = 0;
int Fan2Speed = 0;
// Ende Variablendeklaration

// Start Initialisierung Temperatur-Auswertung
#include <OneWire.h> // http://www.arduino.cc/playground/Learning/OneWire
#include <DallasTemperature.h> // http://milesburton.com/index.php?title=Dallas_Temperature_Control_Library
 
#define ONE_WIRE_BUS 7
#define TEMPERATURE_PRECISION 12
 
OneWire oneWire(ONE_WIRE_BUS); // Einrichten des OneWire Bus um die Daten der Temperaturfühler abzurufen
DallasTemperature sensors(&oneWire); // Bindung der Sensoren an den OneWire Bus
 
DeviceAddress tempDeviceAddress; // Verzeichniss zum Speichern von Sensor Adressen
int numberOfDevices; // Anzahl der gefundenen Sensoren
// Ende Initialisierung Temperatur-Auswertung

// Start Initialisierung Lüfter-Ansteuerung
// Analog output (i.e PWM) pins. These must be chosen so that we can change the PWM frequency without affecting the millis()
// function or the MsTimer2 library. So don't use timer/counter 1 or 2. See comment in setup() function.
// THESE PIN NUMBERS MUST NOT BE CHANGED UNLESS THE CODE IN setup(), setTransistorFanSpeed() AND setDiodeFanSpeed() IS CHANGED TO MATCH!
// On the Mega we use OC1B and OC1C
const int Fan1Pin = 12;     // OC1B // Unten
const int Fan2Pin = 13;          // OC1C // Oben
const unsigned char maxFanSpeed = 80;   // this is calculated as 16MHz divided by 8 (prescaler), divided by 25KHz (target PWM frequency from Intel specification)

// Set the transistor fan speed, where 0 <= fanSpeed <= maxFanSpeed
void setFan1Speed(unsigned char fanSpeed)
{
  OCR1BH = 0;
  OCR1BL = fanSpeed;
}

// Set the diode fan speed, where 0 <= fanSpeed <= maxFanSpeed
void setFan2Speed(unsigned char fanSpeed)
{
OCR1CH = 0;
OCR1CL = fanSpeed;
}
// Ende Initialisierung Lüfter-Ansteuerung

void setup()
{   
  Serial.begin(9600);

  // Initialisierung Temperaturauswertung
  Serial.println("Abfrage mehrerer Dallas Temperatur Sensoren");
  Serial.println("-------------------------------------------");
  
  // Suche der Sensoren
  Serial.println("Suche Temperatur Sensoren...");
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();

  Serial.print("Habe ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" Sensoren gefunden.");

  // Setzen der Genauigkeit
  for(int i=0 ;i<numberOfDevices; i++) {
    if(sensors.getAddress(tempDeviceAddress, i)) {
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
      Serial.print("Sensor ");
      Serial.print(i);
      Serial.print(" hat eine genauigkeit von ");
      Serial.println(sensors.getResolution(tempDeviceAddress), DEC);
    }
  }

  // Initialisierung Lüfteransteuerung

  // Set up the PWM pins for a PWM frequency close to the recommended 25KHz for the Intel fan spec.
  // We can't get this frequency using the default TOP count of 255, so we have to use a custom TOP value.
 
  // Only timer/counter 1 is free because TC0 is used for system timekeeping (i.e. millis() function),
  // and TC2 is used for our 1-millisecond tick. TC1 controls the PWM on Arduino pins 9 and 10.
  // However, we can only get PWM on pin 10 (controlled by OCR1B) because we are using OCR1A to define the TOP value.
  // Using a prescaler of 8 and a TOP value of 80 gives us a frequency of 16000/(8 * 80) = 25KHz exactly.

  // On the Mega we use TC1 and OCR1B, OCR1C
  TCCR1A = (1 << COM1B1) | (1 << COM1B0) | (1 << COM1C1) | (1 << COM1C1) | (1 << WGM11) | (1 << WGM10);  // OC1A disconnected, OC1B = OC1C inverted fast PWM 
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);  // TOP = OCR1A, prescaler = 8
  TCCR1C = 0;
  OCR1AH = 0;
  OCR1AL = 79;  // TOP = 79

  OCR1BH = 0;
  OCR1BL = maxFanSpeed;
  OCR1CH = 0;
  OCR1CL = maxFanSpeed;
 
  TCNT1H = 0;
  TCNT1L = 0;
 
  // We have to enable the ports as outputs before PWM will work.
  pinMode(Fan1Pin, OUTPUT);
  pinMode(Fan2Pin, OUTPUT);
  
  // Fan unten mit Festdrehzahl
  setFan1Speed(35);
}

void loop()
{

  // ------------ Timer initialisieren und starten ------------
  if (TimerStart == 0) {
    startTime = millis();
    TimerStart = 1;
  }

  // ------------ Abgelaufene Zeit in Sekunden ermitteln ------------
  elapsedTime =  (millis() - startTime) / 1000 ;

  // ------------ Standardroutine ------------
  if (elapsedTime == WaitTime) {
    // Aufruf der Funktion sensors.requestTemperatures()
    // Dadurch werden alle Werte abgefragt.
    sensors.requestTemperatures();
    float tempC0 = sensors.getTempCByIndex(0);
    float tempC1 = sensors.getTempCByIndex(1);
    float tempDurchschnitt = ((Wertigkeit * tempC0) + tempC1) / (Wertigkeit + 1);

    if (tempDurchschnitt<25) {
      Fan2Speed = 15;
    }
    else { 
      Fan2Speed = (((tempDurchschnitt-25)*3)+25);
    }

    setFan2Speed(Fan2Speed);

    Serial.print("Temperatur1: ");
    Serial.println(tempC0);
    Serial.print("Temperatur2: ");
    Serial.println(tempC1);
    Serial.print("Temperaturdurchschnitt: ");
    Serial.println(tempDurchschnitt);

    TimerStart = 0;
	
  }
}
