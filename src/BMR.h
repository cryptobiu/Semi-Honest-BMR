/*
 * 	BMR.h
 * 
 *      Author: Aner Ben-Efraim 
 * 	
 * 	year: 2016
 * 
 */

#ifndef _BMR_H_
#define _BMR_H_


#include "ottmain.h"
#include "secCompMultiParty.h"
#include "basicSockets.h"
#include "BMR_BGW.h"
#include "TedKrovetzAesNiWrapperC.h"



using namespace std;

//#define func(a,b)           a+(8-a%8)//a//
#define PAD(a,padTo)		a+(padTo-a%padTo)//a//


Circuit* loadCircuit(char* filename);
void loadInputs(char* filename);
//
void initCommunication(string addr, int port, int player, int mode);
void initializeCommunication(int* ports);
void initializeCommunication(char* filename, Circuit* c, int p);
//
void initializeOT(string addr, int port, int player, int mode);
void initializeOTs(int* ports);
void initializeOTs();
//
void newLoadRandom();


//
void freeXorCompute();
__m128i* XORsuperseeds(__m128i *superseed1, __m128i *superseed2);
void XORsuperseedsNew(__m128i *superseed1, __m128i *superseed2, __m128i *out);
//__m128i* pseudoRandomFunc(__m128i seed1, __m128i seed2, int g);

void computeGates();

//obsolete
void sendLambda(int player);
void receiveLambda(int player, CBitVector choices);
void mulLambdas();
//replaced by
void sendLambdaC(int player, CBitVector Delta);
void receiveLambdaC(int player, CBitVector choices);
void mulLambdasC();

//obsolete
void sendR(int player);
void receiveR(int player, CBitVector choices);
void mulRAssist(int player, CBitVector choices);
void mulR();
//replaced by
void sendRC(int player, CBitVector Delta);
void receiveRC(int player, CBitVector choices);
void mulRC();



//last communication round of offline phase
void exchangeGates(); //each player sends and receives the Ag,Bg,Cg, and Dg of each other player and sums them
void sendGates(string addr, int port, __m128i* inputs, int player);
void receiveGates(string addr, int port, int player);
void newSendGate(__m128i* toSend, int player);
void newReceiveGate(int player);


void exchangeInputs(); //each player sends and receives his input bits (XORed with lambda).

void sendInput(bool* inputs, int player);//send inputs to specific player (specified by addr and port)
void receiveInput(int player);//receive inputs from a specific player

void exchangeSeeds(); //each player sends and receives the seeds corresponding to the input bits (XORed with lambda).

void sendSeed(__m128i* seeds, int player);//send inputs to specific player (specified by addr and port)
void receiveSeed(int player);//receive inputs from a specific player

bool* computeOutputs();

//
void synchronize();
void sendBit(int player, bool* toSend);
void receiveBit(int player);
//
void deletePartial();
void deleteAll();

#endif _BMR_H_
