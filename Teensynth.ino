#include <TimerOne.h>
#include <MIDI.h>

#define TAB_SIZE 8
#define N_VOIX 2
#define N_SINE 1000 //nombre de sample dans la look up table de sin
#define N_WAVESHAPE 3 //carré, dente de scie et sinusoïde

MIDI_CREATE_DEFAULT_INSTANCE();

const int test_input_pin = 7;//pour faire des tests quand on n'a pas de MIDI

const int ledMidi = 5;
const int built_in_ledPin = 6;
const int audioPin = 8;
const int debugPin = 9;
const int waveShapeButtonPin = 17;
int wave_current_state = 0;
int wave_prev_state = 0;
int waveshape_osc = 1;//est la même pour toutes les voix

int count = 0;

const unsigned int oscInterruptFreq = 10000;//fréquence d'interruption = Fréquence d'echantillonnage
const float masterTune = 440.f;

int tab_note[TAB_SIZE];
int nb_note_on = 0;
bool found = false;
int audio_output = 0;

int sine_table[N_SINE];

//VOICES variables
volatile long oscPeriod = 240;
volatile int oscFreq[N_VOIX];
int n_interruption[N_VOIX];//nombre d'interruption par période de l'oscillateur
volatile unsigned int oscCounter[N_VOIX];
volatile bool phase[N_VOIX];//à refaire
volatile bool gate[N_VOIX];

//LFO
int lfoFreq = 1;
int lfo_periode = 1000;//en milisecondes
int lfo_waveshape = 1;
int lfo_counter = 0;
int lfo_n_interruption = oscInterruptFreq;

void setup()
{
  Serial.begin(9600);
  
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
    phase[i] = false;
    oscFreq[i] = 440;
    oscCounter[i] = 0;
    n_interruption[i] = 90; //La 440 pour Fe=40000
  }
  //LFO
  lfoFreq = 1;//Hz
  //calcul du nombre d'interruption par période de l'oscillateur
  lfo_n_interruption = oscInterruptFreq / lfoFreq;
  Serial.println(lfo_n_interruption);

  //tests
  pinMode(ledMidi, OUTPUT);
  pinMode(test_input_pin, INPUT);
  pinMode(waveShapeButtonPin, INPUT);
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
  
  //On ajoute la note dans le tableau
  tab_note[nb_note_on] = note;
  nb_note_on++;//on incrémente seulement après nb_note_on
  
  //On assigne les voix aux différentes notes
  assignation_voix();
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
    found = false;
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
    oscCounter[i]++;
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
    PORTF = audio_output;
  }
  else
  {
    PORTF = 0;
  }
  audio_output = 0;
}

byte get_LFO_value()
{
  //return lfo_sinusoide();
  return lfo_sawtooth();
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
  Serial.println(lfo_counter);
  //Serial.println(lfo_counter * 255 / lfo_n_interruption);
  return lfo_counter * 255 / lfo_n_interruption;
}


byte squareWave(int voix)
{
  if(oscCounter[voix] >= oscInterruptFreq / (2 * oscFreq[voix]))//on divise par deux pour changer toutes les 1/2 période
  {
    oscCounter[voix] = 0;
    if (phase[voix] && gate[voix])
    {
      phase[voix] = !phase[voix];
      return get_LFO_value();
    }
    else
    {
      phase[voix] = !phase[voix];
      return 0;
    }
  }
  return 0;
}

byte sawtooth(int voix)
{
  if(gate[voix])
  {
    if(oscCounter[voix]  >= n_interruption[voix])
    {
      oscCounter[voix] = 0;
    }
    return oscCounter[voix] * 255 / n_interruption[voix];
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


void loop()
{
  count++;
  //usbMIDI.read();
  MIDI.read();
  updateWaveShape();

  //playWithButton();
}
