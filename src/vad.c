#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "pav_analysis.h"
#include "vad.h"

const float FRAME_TIME = 10.0F; /* in ms. */
const int ALFA1=3;
const int ALFA2=5;
/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MS", "MV"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  //feat.zcr = feat.p = feat.am = (float) rand()/RAND_MAX;
  feat.zcr=compute_zcr(x,N,16000);
  feat.p=compute_power(x,N);
  feat.am=compute_am(x,N);
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate,float alfa0,int Ninit) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->alfa0=alfa0;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->Ninit=Ninit;
  vad_data->aux=0;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state;
  if (vad_data->state==ST_MAYBEVOICE)
    state=ST_VOICE;
  else if(vad_data->state==ST_MAYBESILENCE)
    state=ST_SILENCE;
  else 
    state=vad_data->state;
  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x) { 

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  vad_data->last_feature = f.p; /* save feature, in case you want to show */

  switch (vad_data->state) {
    
  case ST_INIT:
   // vad_data->k0 = f.p+vad_data->alfa0; //definimos k0 como alfa0+potencia de la trama
    vad_data->k0= vad_data->k0+ pow(10,f.p/10);
    vad_data->aux=vad_data->aux+1;
    if(vad_data->aux==vad_data->Ninit)
      vad_data->k0= 10*log10 ((1/vad_data->Ninit)*vad_data->k0); //hemos calculado el valor de k0 con la fórmula del enunciado de la práctica
    vad_data->k1=vad_data->k0+ALFA1;
    vad_data->k2=vad_data->k1+ALFA2; 
    vad_data->state = ST_SILENCE;
    break;

  case ST_SILENCE: //fp es la potencia de la trama
    if (f.p > vad_data->k1)
      vad_data->state = ST_MAYBEVOICE;
      vad_data->silence=1; //cuento que tengo una trama en silencio 
    break;

  case ST_VOICE:
    if (f.p < vad_data->k1)
      vad_data->state = ST_MAYBESILENCE;
      vad_data->voice=1; //cuento que tengo una trama en voz
    break;

  case ST_MAYBESILENCE:
   if(f.p>vad_data->k2)
      vad_data->state=ST_VOICE;
    if(f.p<vad_data->k1)
      vad_data->state=ST_MAYBESILENCE;
      vad_data->silence=vad_data->silence+1;
    if(vad_data->silence>9) //para probar
      vad_data->state=ST_SILENCE;
   
  case ST_MAYBEVOICE:
    if(f.p< vad_data->k1)
      vad_data->state=ST_SILENCE;
    if(f.p>vad_data->k2)
      vad_data->state=ST_MAYBEVOICE;
      vad_data->voice=vad_data->voice+1;
    if(vad_data->voice>9)//probar
      vad_data->state=ST_VOICE;

  case ST_UNDEF:
    break;
  }

  if (vad_data->state == ST_SILENCE ||
      vad_data->state == ST_VOICE ||
      vad_data->state == ST_MAYBESILENCE || 
      vad_data->state == ST_MAYBEVOICE)
    return vad_data->state;
  else if(vad_data->state==ST_INIT)
    return ST_SILENCE;
  else
    return ST_UNDEF;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}
