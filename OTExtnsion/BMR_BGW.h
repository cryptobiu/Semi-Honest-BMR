/*
 * 	BMR_BGW.h
 * 
 *      Author: Aner Ben-Efraim 
 * 	
 * 	year: 2016
 * 
 */

#ifndef _BMR_BGW__H_
#define _BMR_BGW_H_
#pragma once

#include "BMR_BGW_aux.h"
#include "BMR.h"
#include "basicSockets.h"
#include <thread>
#include <mutex> 

/*
GLOBAL VARIABLES USED IN BOTH BMR AND BMR_BGW
*/
//the circuit
extern Circuit* cyc;

//for communication
extern string * addrs;
extern BmrNet ** communicationSenders;
extern BmrNet ** communicationReceivers;
//storage room for exchanging data
extern __m128i** playerSeeds;

//number of Wires
//extern int numOfWires;
extern int numOfInputWires;
extern int numOfOutputWires;

//number of players
extern int numOfParties;
//this player number
extern int partyNum;
//the random offset
extern __m128i R;

extern mutex mtxVal;
extern mutex mtxPrint;


extern __m128i* bigMemoryAllocation;

void InitializeBGW(int hm);

void secretShare();
//get shares of multiplication of lambdas, for and gate calculation
void mulLambdasBGW();
//Reduce the degree of the shares of multiplication of lambdas, using a BGW round - not need if more than 2/3 majority
void reduceDegBGW();
//compute shares of multiplications of R with the correct lambdas, and add to shares of PRG+S
void mulRBGW();
//computes shares of multiplications of R with the correct lambdas, does an extra degree reduction step, and adds to shares of PRG+S
void mulRBGWwithReduction();

void sendShares(int party);

void receiveShares(int party);

void addLocalShares();

void exchangeGatesBGW();

void deletePartialBGW();

void deleteAllBGW();


#endif
