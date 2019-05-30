#include <TimerOne.h>
#include <MIDI.h>
#define TAB_SIZE 8

MIDI_CREATE_DEFAULT_INSTANCE();

const int test_input_pin = 7;//pour faire des tests quand on n'a pas de MIDI

const int ledMidi = 5;
const int built_in_ledPin = 6;
const int audioPin = 8;
const int debugPin = 9;
const unsigned int oscInterruptFreq = 45000;//fréquence d'interruption = Fréquence d'echantillonnage
const float masterTune = 440.f;

byte tab_note[TAB_SIZE];//à refaire plus tard
int nb_note_on = 0;

volatile long oscPeriod = 240;
volatile int oscFreq = 440;//On commence par un la4
volatile unsigned long oscCounter = 0;
volatile bool phase = false;
volatile bool gate = false;


void setup() 
{
  //Serial.begin(9600);

  for(int i = 0 ; i < TAB_SIZE ; ++i)
  {
    tab_note[i] = -1;
  }
  //tests
  pinMode(ledMidi, OUTPUT);
  pinMode(test_input_pin, INPUT);
  pinMode(built_in_ledPin, OUTPUT);
  pinMode(audioPin, OUTPUT);
  
  //initializing the PORTF (pin 38 to 45)
  int i = 0;
  for(i = 38 ; i <= 45 ; i++)
  {
    pinMode(i, OUTPUT);
  }
  

  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(onNoteOn);
  MIDI.setHandleNoteOff(onNoteOff);
  //usbMIDI.setHandleNoteOn(onNoteOn);
  //usbMIDI.setHandleNoteOff(onNoteOff);

  Timer1.initialize(1000000 / oscInterruptFreq);
  //Timer1.attachInterrupt(oscInterrupt, 1000000 / oscInterruptFreq);
  Timer1.attachInterrupt(oscInterrupt);
}




void onNoteOn(byte channel, byte note, byte velocity)
{ 
  digitalWrite(ledMidi, HIGH);
  gate = true;
  
  //On ajoute la note dans le tableau
  tab_note[nb_note_on] = note;
  nb_note_on++;//on incrémente seulement après nb_note_on
  oscFreq = noteToFreq(tab_note[nb_note_on -1]);//on soustrait 1 pour convertir en indice du tableau
}

void onNoteOff(byte channel, byte note, byte velocity)
{
    digitalWrite(ledMidi, LOW);
    
    for(int i = 0 ; i < nb_note_on ; i++)
    {
      if( tab_note[i] == note )
      {
        for(int j = i ; j < nb_note_on - 1 ; j ++)
         {
          tab_note[j] = tab_note[j+1];
         }
         tab_note[nb_note_on - 1] = -1;
      }
    }
    nb_note_on--;
    if(nb_note_on <= 0)
    {
      gate = false;
    }
    else
    {
      gate = true;
      oscFreq = noteToFreq(tab_note[nb_note_on -1]);//on soustrait 1 pour convertir en indice du tableau
    }
}

float noteToFreq(int note)
{
  return masterTune*pow(2, (note - 69) / 12.f); 
}

int noteToOscPeriod(int note)
{
  return oscInterruptFreq/noteToFreq(note);
}

void oscInterrupt()
{
  oscCounter++;
  //sawtooth();
  squareWave();
}
void squareWave()
{
  if(oscCounter >= oscInterruptFreq / (2 * oscFreq))//on divise par deux pour changer toutes les 1/2 période
  {
    oscCounter = 0;
    if (phase && gate)
    {
      PORTF = 255;
      //digitalWrite(audioPin, HIGH);
      digitalWrite(built_in_ledPin, HIGH);
    }
    else
    {
      PORTF = 0;
      //digitalWrite(audioPin, LOW);
      digitalWrite(built_in_ledPin, LOW);
    }
    phase = !phase;
  }
}
void sawtooth_test()
{
  if(true)
  {
    PORTF = oscCounter;
    digitalWrite(built_in_ledPin, HIGH);
  }
  else
  {
    digitalWrite(built_in_ledPin, LOW);
  }
}
void sawtooth()
{
  if(gate)
  {
    //PORTF = 255 * oscFreq * (oscCounter%255);
    PORTF = oscFreq * oscCounter * 255 / oscInterruptFreq;
    //PORTF = oscFreq * 
  }
  else
  {
    PORTF = 0;
  }
}

void playWithButton()
{
  //Petit test pour jouer avec un pushbuton
  if( digitalRead(test_input_pin) == HIGH )
  {
    digitalWrite(built_in_ledPin, HIGH);
    
    //On envoie la dernière note
    gate = true;
    oscFreq = 440;
  }
  if(digitalRead(test_input_pin) == LOW)
  {
    digitalWrite(built_in_ledPin, LOW);
    gate = false;
  }
}

void loop() 
{
  //usbMIDI.read();
  MIDI.read();
  //playWithButton();
}
