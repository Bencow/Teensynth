#include <TimerOne.h>
#include <MIDI.h>
#define TAB_SIZE 8
#define N_VOIX 1
#define N_SINE 1000 //nombre de sample dans la look up table de sin

MIDI_CREATE_DEFAULT_INSTANCE();

const int test_input_pin = 7;//pour faire des tests quand on n'a pas de MIDI

const int ledMidi = 5;
const int built_in_ledPin = 6;
const int audioPin = 8;
const int debugPin = 9;
const unsigned int oscInterruptFreq = 10000;//fréquence d'interruption = Fréquence d'echantillonnage
const float masterTune = 440.f;

int tab_note[TAB_SIZE];
int tab_entree[255];
int nb_note_on = 0;
bool found = false;
byte audio_output = 0;


int sine_table[N_SINE];

volatile long oscPeriod = 240;
volatile int oscFreq[N_VOIX];
volatile unsigned int oscCounter[N_VOIX];
int n_interruption[N_VOIX];//nombre d'interruption par période de l'oscillateur
volatile bool phase[N_VOIX];//à refaire
volatile bool gate[N_VOIX];


void setup()
{
  //Serial.begin(9600);
  
  for(int i = 0 ; i < TAB_SIZE ; ++i)
  {
    tab_note[i] = -1;
  }
  for(int i = 0 ; i < 255 ; ++i)
  {
    tab_entree[i] = 0;
  }
  for(int i = 0 ; i < N_SINE ; i++)
  {
    sine_table[i] = 255 * sin(2*PI*i/N_SINE);
  }
  for(int i = 0 ;i < N_VOIX ; i++)
  {
    gate[i] = false;
    phase[i] = false;
    oscFreq[i] = 440;
    oscCounter[i] = 0;
    n_interruption[i] = 90; //La 440 pour Fe=40000
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
  oscFreq[nb_note_on] = noteToFreq(note);
  n_interruption[nb_note_on] = oscInterruptFreq / oscFreq[nb_note_on];//nombre d'interruption par période de l'oscillateur
  
  /*
  //On ajoute la note dans le tableau
  tab_note[nb_note_on] = note;
  nb_note_on++;//on incrémente seulement après nb_note_on
  oscFreq = noteToFreq(tab_note[nb_note_on -1]);//on soustrait 1 pour convertir en indice du tableau

  tab_entree[note] = nb_note_on;
  */
  
  gate[nb_note_on] = true;
  nb_note_on++;
  //print_tab(8, note);
}

void onNoteOff(byte channel, byte note, byte velocity)
{
    digitalWrite(ledMidi, LOW);
    
    
    nb_note_on--;
    gate[nb_note_on] = false;
    
    /*
    for(int i = 0 ; i < nb_note_on ; i++)
    {
      if( tab_note[i] == note && !found )
      {
        found = true;
        
        for(int j = i ; j < nb_note_on - 1 ; j++)
         {
          tab_note[j] = tab_note[j+1];
         }
         tab_note[nb_note_on - 1] = -1;
         nb_note_on--;
      }
    }
    */
    /*
    //int j = tab_entree[note]-1;//moins 1 pour convertir en indice du tab_note
    //tab_entree[note] = 0;
    for(int j = tab_entree[note] -1 ; j < nb_note_on ; j++)
    {
      tab_note[j] = tab_note[j+1];
      tab_entree[tab_note[j]]--;
    }
    tab_note[nb_note_on - 1] = -1;
    nb_note_on--;
    
    
    if(nb_note_on <= 0)
    {
      gate = false;
    }
    else
    {
      oscFreq = noteToFreq(tab_note[nb_note_on -1]);//on soustrait 1 pour convertir en indice du tableau
      gate = true;
      
    }
    //print_tab(9, note);
    found = false;
    */
}

void print_tab(int type, int note)
{
    for(int i = 0 ; i < TAB_SIZE ; i++)
    {
      Serial.print(tab_note[i]);
      Serial.print(" ");
    }
    Serial.print(nb_note_on);
    Serial.print(type);
    Serial.print(note);
    Serial.print("\n");
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
  
  oscCounter[0]++;
  
  /*
  for(int i = 0 ; i < N_VOIX ; ++i)
  {
    result += squareWave(i);
  }*/
  audio_output = squareWave(0);

  //si l'une des voix est en train de jouer
  if(gate[0] == true)
  {
    PORTF = audio_output;
  }
  else
  {
    PORTF = 0;
  }
  
  
}
byte squareWave(int voix)
{
  if(oscCounter[voix] >= oscInterruptFreq / (2 * oscFreq[voix]))//on divise par deux pour changer toutes les 1/2 période
  {
    oscCounter[voix] = 0;
    if (phase[voix] && gate[voix])
    {
      phase[voix] = !phase[voix];
      return 255;
    }
    else
    {
      phase[voix] = !phase[voix];
      return 0;
    }
  }
  return 0;
}
/*
byte sawtooth(int voix)
{
  if(gate)
  {
    if(oscCounter  >= n_interruption )
    {
      oscCounter = 0;
    }
    PORTF = oscCounter * 255 / n_interruption;
  }
  else
  {
    PORTF = 0;
  }
}

byte sinusoide(int voix)
{
  if(gate)
  {
    if(oscCounter  >= n_interruption )
    {
      oscCounter = 0;
    }
    //int var = oscCounter*N_SINE/n_interruption;
    PORTF = sine_table[oscCounter*N_SINE/n_interruption];
    //Serial.println(var);
  }
  else
  {
    PORTF = 0;
  }
}
*/

/*
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
*/

void loop()
{
  //usbMIDI.read();
  MIDI.read();
  //playWithButton();
}
