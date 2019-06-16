#include <TimerOne.h>
#include <MIDI.h>

#define TAB_SIZE 8
#define N_VOIX 1
#define N_SINE 1000 //nombre de sample dans la look up table de sin
#define N_WAVESHAPE 3 //carré, dente de scie et sinusoïde

MIDI_CREATE_DEFAULT_INSTANCE();

//const int test_input_pin = ;//pour faire des tests quand on n'a pas de MIDI

const int ledMidi = 1;
const int built_in_ledPin = 6;
//const int audioPin = ;
const int debugPin = 3;
const int knobPin = A0;
const int waveShapeButtonPin = 4;
const int lfoStateButtonPin = 5;
const int lfoLedPin = 0;

int wave_current_state = 0;
int wave_prev_state = 0;
int waveshape_osc = 1;//forme d'onde de l'oscillateur (est la même pour toutes les voix)

bool lfo_current_state;//var pour détec
bool lfo_prev_state;

int count = 0;

const unsigned int oscInterruptFreq = 10000;//fréquence d'interruption = Fréquence d'echantillonnage
const float masterTune = 440.f;

int tab_note[TAB_SIZE];
volatile int nb_note_on = 0;
bool found = false;
volatile int audio_output = 0;

int sine_table[N_SINE];

//VOICES variables
int nombre_de_voix_activee = 1;
volatile long oscPeriod = 240;
volatile int oscFreq[N_VOIX];
volatile int n_interruption[N_VOIX];//nombre d'interruption par période de l'oscillateur
volatile unsigned int oscCounter[N_VOIX];
volatile bool gate[N_VOIX];

//LFO
bool lfo_on = false;
volatile int lfoFreq = 1;
volatile int lfo_periode = 1000;//en milisecondes
volatile int lfo_waveshape = 1;
volatile int lfo_counter = 0;
volatile int lfo_n_interruption = oscInterruptFreq;//équivaut à une seconde






void setup()
{
  Serial.begin(9600);
  //digitalWrite(built_in_ledPin, HIGH);
  
  for(int i = 0 ; i < TAB_SIZE ; ++i)
  {
    tab_note[i] = -1;
  }
  for(int i = 0 ; i < N_SINE ; i++)
  {
    sine_table[i] = 255 * sin(2*PI*i/N_SINE);
  }
  for(int i = 0 ;i < N_VOIX ; i++)
  {
    gate[i] = false;
    oscFreq[i] = 440;
    oscCounter[i] = 0;
    n_interruption[i] = 90; //La 440 pour Fe=40000
  }
  //LFO
  lfoFreq = 1;//Hz
  //calcul du nombre d'interruption par période de l'oscillateur
  lfo_n_interruption = oscInterruptFreq / lfoFreq;

  //tests
  pinMode(ledMidi, OUTPUT);
  pinMode(waveShapeButtonPin, INPUT);
  pinMode(built_in_ledPin, OUTPUT);
  pinMode(knobPin, INPUT);
  
  
  //initializing the PORTC (pin 38 to 45)
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
  noInterrupts();
  //On ajoute la note dans le tableau
  tab_note[nb_note_on] = note;
  nb_note_on++;//on incrémente seulement après nb_note_on
  
  //On assigne les voix aux différentes notes
  assignation_voix();
  interrupts();
}

void assignation_voix()
{
  //On assigne les voix aux note les plus à droite dans tab_note
  //Pour chaque voix
  for(int i = 0 ; i < N_VOIX ; ++i)
  {
    if(nb_note_on > i)
    {
      //On envoie la note à la voix i
      sendNoteToVoice(tab_note[i], i);
    }
    else
    {
      //shut down the voice i
      gate[i] = false;
    }
  }
}

void sendNoteToVoice(int note, int voice)
{
  oscFreq[voice] = noteToFreq(note);
  //calcul du nombre d'interruption par période de l'oscillateur
  n_interruption[voice] = oscInterruptFreq / oscFreq[voice];
  gate[voice] = true;
}

void onNoteOff(byte channel, byte note, byte velocity)
{
    digitalWrite(ledMidi, LOW);
    noInterrupts();
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
    assignation_voix();
    interrupts();
    found = false;
}

//debug
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
bool oneVoiceIsPlaying()
{
  for(int i = 0 ; i < N_VOIX ; ++i)
  {
    if(gate[i])
    {
      return true;
    }
  }
  return false;
}
void oscInterrupt()
{  
  for(int i = 0 ; i < N_VOIX ; ++i)
  {
    if(gate[i])
    {
      oscCounter[i]++;
    }
    
    if(waveshape_osc == 1)
    {
      audio_output += squareWave(i);
    }
    else if(waveshape_osc == 2)
    {
      audio_output += sawtooth(i);
    }
    else if(waveshape_osc == 3)
    {
      audio_output += sinusoide(i);
    }
  }
  lfo_counter++;
  
  //si l'une des voix est en train de jouer
  if(oneVoiceIsPlaying())
  {
    PORTC = audio_output;
  }
  else
  {
    PORTC = 0;
  }
  audio_output = 0;
}

byte get_LFO_value()
{
  //return lfo_sinusoide();
  //Serial.println(lfo_squareWave());
  return lfo_squareWave();
}
byte lfo_squareWave()
{
  //si le compteur dépasse le nombre d'interruption par période
  if(lfo_counter >= lfo_n_interruption)
  {
    //on le réinitialise
    lfo_counter = 0;
  }
  if(lfo_counter >= lfo_n_interruption/2)
  {
    return 255;
  }
  else
  {
    return 0;
  }
}

byte lfo_sinusoide()//ne marche pas
{
  //balance une frequence de 1 Hz
  //if(oscCounter[voix]  >= n_interruption[voix] )
  if(lfo_counter >= 10000 )
  {
    lfo_counter = 0;
  }
  return sine_table[lfo_counter * N_SINE / lfo_n_interruption];
}

byte lfo_sawtooth()
{
  if(lfo_counter  >= lfo_n_interruption)
  {
    lfo_counter = 0;
  }
  return lfo_counter * 255 / lfo_n_interruption;
}

byte squareWave(int voix)
{
  if(oscCounter[voix] >= n_interruption[voix])
  {
    oscCounter[voix] = 0;
  }
  if(oscCounter[voix] <= n_interruption[voix]/2)
  {
      if(lfo_on)//si le lfo est activé
      {
        return get_LFO_value();//on envoie sa valeur
      }
      else//sinon on envoie 255
      {
        return 255;
      }
  }
  else
  {
    return 0;
  }
}

byte sawtooth(int voix)
{
  if(gate[voix])
  {
    if(oscCounter[voix]  >= n_interruption[voix])
    {
      oscCounter[voix] = 0;
    }
    return oscCounter[voix] * get_LFO_value() / n_interruption[voix];
  }
  else
  {
    return 0;
  }
}

byte sinusoide(int voix)
{
  if(gate[voix])
  {
    if(oscCounter[voix]  >= n_interruption[voix] )
    {
      oscCounter[voix] = 0;
    }
    return sine_table[oscCounter[voix] * N_SINE / n_interruption[voix]];
  }
  else
  {
    return 0;
  }
}


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


void updateWaveShape()
{
  wave_prev_state = wave_current_state;
  wave_current_state = digitalRead(waveShapeButtonPin);
  if(wave_current_state == HIGH && wave_prev_state == LOW)
  {
    if(waveshape_osc >= N_WAVESHAPE)
    {
      waveshape_osc = 1;
    }
    else
    {
      waveshape_osc++;
    }
  }
}

void updateLfoState()
{
  lfo_prev_state = lfo_current_state;
  lfo_current_state = digitalRead(lfoStateButtonPin);
  
  //sur un front montant du boutton
  if(lfo_current_state == HIGH && lfo_prev_state == LOW)
  {
    lfo_on = !lfo_on;
  }
  //Serial.println(lfo_on);
}

void updateLfoFreq()
{
  int knobvalue = analogRead(knobPin);
  lfoFreq = 1 + knobvalue * 20 / 1024;
  lfo_n_interruption = oscInterruptFreq / lfoFreq;
}

void loop()
{
  count++;
  //usbMIDI.read();
  MIDI.read();
  updateWaveShape();
  updateLfoState();
  updateLfoFreq();
  //Serial.println(d);
  //playWithButton();
}
