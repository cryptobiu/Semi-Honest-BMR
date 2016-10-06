/*
 * secCompMultiParty.h
 *
 *  Created on: Mar 31, 2015
 *      Author: froike (Roi Inbar) 
 * 	Modified: Aner Ben-Efraim
 * 
 */



#ifndef SECCOMPMULTIPARTY_H_
#define SECCOMPMULTIPARTY_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <emmintrin.h>
#include <iomanip>  //cout
#include <iostream>
#include "aes.h"
#include "TedKrovetzAesNiWrapperC.h"
#include "myDefs.h"

extern Wire *zeroWire, *oneWire;


//returns a (pseudo)random 128bit number using AES-NI
__m128i LoadSeedNew();
//initialize randomness from user key using AES-NI
void initializeRandomness(char* key, int numOfParties);
//returns a (pseudo)random bit using AES-NI, by sampling a bit;
bool LoadBool();

//XORs the superseeds
void XORvectors(__m128i *vec1, __m128i *vec2, __m128i *out, int length);

/*
*creates a pseudoranodm number of 128*length bits
*uses 256bit-AES with seed1||seed2 as the key and g*length,g*length+1,...,g*length+length-1 as the messages.
*/
__m128i* pseudoRandomFunc(__m128i seed1, __m128i seed2, int g, int length);
void pseudoRandomFunc(__m128i seed1, __m128i seed2, int g, int length,__m128i* result);

////returns the value of the wire
bool getValue(Wire wire, int player);

bool getValue(Wire *wire, int player);

bool getTrueValue(Wire wire, int player, Wire father);

bool getTrueValue(Wire *wire, int player);



//load the inputs
void loadInputs(char* filename,Circuit * cyc, int partyNum);

/*
 * This function gets a path to a file that represents a circuit (format instructions below).
 * 
 * Returns a Circuit struct.
 *
 */
Circuit * readCircuitFromFile(char path[]);

/*
 * private function that creates gate structs, used in readCircuitFromFile function.
 */
Gate GateCreator(const unsigned int inputBit1, const unsigned int inputBit2, const unsigned int outputBit, const unsigned int TTable, const unsigned int flags, unsigned int number);

/*
 * This function prints the Circuit.
 */
void printCircuit(const Circuit * C);

void removeSpacesAndTabs(char* source);



Gate ** specialGatesCollector(Gate * GatesArray, const unsigned int arraySize, const unsigned int specialAmount);
Gate ** regularGatesCollector(Gate * GatesArray, const unsigned int arraySize, const unsigned int regularGatesAmount);


/*
*prints an arry of _m128i
*/
void print(__m128i* arr, int size);
/*prints a __m128i number*/
void print128_num(__m128i var);

inline void hex_dump(unsigned char* str, int length)
{
	for (int i = 0; i< length; i++)
	{
		printf("%02x", str[i]);
	}
	printf("\n");
}

/*
* This functions as a destroyer for the circuit struct. 
*/
void freeCircle(Circuit * c);
void freeCircuitPartial(Circuit * c);
void freeWire(Wire w);
void freeGate(Gate gate);
void freePlayer(aPlayer p);

void printVersion(int hm);


#endif /* SECCOMPMULTIPARTY_H_ */