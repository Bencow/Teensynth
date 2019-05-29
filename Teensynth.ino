#include <TimerOne.h>
#include <MIDI.h>
#define TAB_SIZE 10

MIDI_CREATE_DEFAULT_INSTANCE();

const int test_input_pin = 7;//pour faire des tests quand on n'a pas de MIDI

const int ledMidi = 5;
const int built_in_ledPin = 6;
const int audioPin = 8;
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
  Serial.begin(9600);

  for(int i = 0 ; i < TAB_SIZE ; ++i)
  {
    tab_note[i] = -1;
  }
  //tests
  pinMode(ledMidi, OUTPUT);
  pinMode(test_input_pin, INPUT);
  pinMode(built_in_ledPin, OUTPUT);
  pinMode(audioPin, OUTPUT);

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
  //digitalWrite(ledMidi, HIGH);
  oscFreq = noteToFreq(note);
  gate = true;
  
  if (velocity == 0) onNoteOff(channel, note, velocity);
  else //Si on enfonce bien une touche
  {
    //On ajoute la note dans le tableau
    tab_note[nb_note_on] = note;
    nb_note_on++;//on incrémente seulement après nb_note_on
  }
  
  
  if( nb_note_on > 0 )//Si il y a au moins une note en train d'être jouée
  {
    //On envoie la dernière note
    gate = true;
    oscFreq = noteToFreq(tab_note[nb_note_on -1]);//on soustrait 1 pour convertir en indice du tableau
  }
}

void onNoteOff(byte channel, byte note, byte velocity)
{
    //digitalWrite(ledMidi, LOW);
    //gate = false;
    
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
  sawtooth_test();

}
void squareWave()
{
  if(oscCounter >= oscInterruptFreq / (2 * oscFreq))
  {
    oscCounter = 0;
    if (phase && gate)
    {
      digitalWrite(audioPin, HIGH);
      digitalWrite(built_in_ledPin, HIGH);
    }
    else 
    {
      digitalWrite(audioPin, LOW);
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
  //MIDI.read();
  playWithButton();
}
