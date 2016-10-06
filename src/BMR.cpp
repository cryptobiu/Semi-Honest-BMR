/*
 * 	BMR.cpp
 * 
 *      Author: Aner Ben-Efraim 
 * 	
 * 	year: 2016
 * 
 */

#include "BMR.h"
#include <thread>
#include <mutex> 
#include "secCompMultiParty.h"
#include "ottmain.h"


//Threads
mutex mtxVal;
mutex mtxPrint;

//the circuit
Circuit* cyc;

//number of Wires
//int numOfWires;
int numOfInputWires;
int numOfOutputWires;

//number of players
int numOfParties;
//this player number
int partyNum;


//communication
string * addrs;
BmrNet ** communicationSenders;
BmrNet ** communicationReceivers;
//for OTExtension wrappers
OTclass **receivers;
OTclass **senders;

//A random 128 bit number, representing the difference between the seed for value 0 and value 1 of the wires ("free XOR")
__m128i R;

//Response vectors - stores the response from the OT.
CBitVector *OTs;//for lambda multiplication
CBitVector *OTLongs;//for R multiplication

//Send vectors for lambdas
CBitVector *X1;
CBitVector *X2;

//Send vectors for R
CBitVector *X1Long;
CBitVector *X2Long;

//for exchanging data
__m128i** playerSeeds;

//Preallocate memory
__m128i* bigMemoryAllocation;


//load the circuit
Circuit* loadCircuit(char* filename)
{
	cyc = readCircuitFromFile(filename);
	numOfParties = cyc->amountOfPlayers;
	cyc->numOfInputWires = 0;
	for (int p = 0; p < numOfParties; p++)
	{
		cyc->numOfInputWires += cyc->playerArray[p].playerBitAmount;
	}
	numOfInputWires = cyc->numOfInputWires;
	numOfOutputWires = cyc->outputWires.playerBitAmount;
	return cyc;
}

//setting up communication
void initCommunication(string addr, int port, int player, int mode)
{
	char temp[25];
	strcpy(temp, addr.c_str());
	if (mode == 0)
	{
		communicationSenders[player] = new BmrNet(temp, port);
		communicationSenders[player]->connectNow();
	}
	else
	{
		communicationReceivers[player] = new BmrNet(port);
		communicationReceivers[player]->listenNow();
	}
}

void initializeCommunication(int* ports)
{
	int i;
	communicationSenders = new BmrNet*[numOfParties];
	communicationReceivers = new BmrNet*[numOfParties];
	thread *threads = new thread[numOfParties * 2];
	for (i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2 + 1] = thread(initCommunication, addrs[i], ports[i * 2 + 1], i, 0);
			threads[i * 2] = thread(initCommunication, "127.0.0.1", ports[i * 2], i, 1);
		}
	}
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != 2 * partyNum && i != (2 * partyNum + 1))
			threads[i].join();//wait for all threads to finish
	}

	delete[] threads;
}

void initializeCommunicationSerial(int* ports)//Use this for many parties
{
	int i;
	communicationSenders = new BmrNet*[numOfParties];
	communicationReceivers = new BmrNet*[numOfParties];
	for (i = 0; i < numOfParties; i++)
	{
		if (i<partyNum)
		{
		  initCommunication( addrs[i], ports[i * 2 + 1], i,0);
		  initCommunication("127.0.0.1", ports[i * 2], i, 1);
		}
		else if (i>partyNum)
		{
		  initCommunication("127.0.0.1", ports[i * 2], i, 1);
		  initCommunication( addrs[i], ports[i * 2 + 1], i,0);
		}
	}
}


void initializeCommunication(char* filename, Circuit* c, int p)
{
	cyc = c;
	FILE * f = fopen(filename, "r");
	partyNum = p;
	char buff[STRING_BUFFER_SIZE];
	char ip[STRING_BUFFER_SIZE];
	
	addrs = new string[numOfParties];
	int * ports = new int[numOfParties * 2];


	for (int i = 0; i < numOfParties; i++)
	{
		fgets(buff, STRING_BUFFER_SIZE, f);
		sscanf(buff, "%s\n", ip);
		addrs[i] = string(ip);
		
		ports[2 * i] = 8000 + i*numOfParties + partyNum;
		ports[2 * i + 1] = 8000 + partyNum*numOfParties + i;
	}

	fclose(f);
	//if (numOfParties>20) 
	  initializeCommunicationSerial(ports);

	delete[] ports;
}

//Functions for initializing OT
void initializeOT(string addr, int port, int player, int mode)
{
	char temp[25];
	strcpy(temp, addr.c_str());
	__m128i* key=static_cast<__m128i *>(_aligned_malloc(4 * sizeof(__m128i), 16));
	key[0]=RAND;
	key[1]=RAND;
	key[2]=RAND;
	key[3]=RAND;
	if (mode == 0)
	{
		senders[player] = new OTclass(temp, port, mode,(const char*)key);
		senders[player]->Initialize();
	}
	else
	{
		receivers[player] = new OTclass(temp, port,mode,(const char*)key);
		receivers[player]->Initialize();
	}
}

void initializeOTs(int* ports)//Parallel - not usable on unix, due to no thread safety of Miracl
{
	int i;
	senders = new OTclass*[numOfParties];
	receivers = new OTclass*[numOfParties];
	thread *threads = new thread[numOfParties * 2];
	for (i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(initializeOT, "127.0.0.1", ports[i * 2], i, 0);
			threads[i * 2 + 1] = thread(initializeOT, addrs[i], ports[i * 2 + 1], i, 1);
		}
	}
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != 2 * partyNum && i != (2 * partyNum+1))
			threads[i].join();
	}
	delete[] threads;
}

void newInitializeOTs(int* ports)//Serial - use this method on unix
{
	int i;
	senders = new OTclass*[numOfParties];
	receivers = new OTclass*[numOfParties];
	for (i = 0; i < numOfParties; i++)
	{
		if (i<partyNum)
		{
		  initializeOT( "127.0.0.1", ports[i * 2], i, 0);
		  initializeOT(addrs[i], ports[i * 2 + 1], i, 1);
		}
		else if (i>partyNum)
		{
		  initializeOT(addrs[i], ports[i * 2 + 1], i, 1);
		  initializeOT( "127.0.0.1", ports[i * 2], i, 0);
		}
	}
}

void initializeOTs()
{	
	int * ports = new int[numOfParties * 2];
	
	for (int i = 0; i < numOfParties; i++)
	{
		ports[2 * i] = 22500+i*numOfParties+partyNum;
		ports[2 * i + 1] = 22500 + partyNum*numOfParties + i;
	}		
		
	//initializeOTs(ports);//Windows
	newInitializeOTs(ports);//Unix
	delete[] ports;
}

void newLoadRandom()
{
	
	R = RAND;
	
	for (int p = 0; p < numOfParties; p++)
	{
		for (int i = 0; i < cyc->playerArray[p].playerBitAmount; i++)
		{
			cyc->playerArray[p].playerWires[i]->seed = RAND;
			if (p == partyNum)
			{
				cyc->playerArray[partyNum].playerWires[i]->lambda = RAND_BIT;
				cyc->playerArray[partyNum].playerWires[i]->realLambda = cyc->playerArray[partyNum].playerWires[i]->lambda;
			}
			else//letting only Party i choose the lambdas for his input wires.
			{
				cyc->playerArray[p].playerWires[i]->lambda = 0;
			}
		}
	}
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		cyc->regularGates[g]->output->seed = RAND;
		cyc->regularGates[g]->output->lambda = RAND_BIT;
	}
	//compute seeds and lambdas on wires of outputs of XOR gates by XORing the values on the input wires.
	freeXorCompute();
}

//XOR gates - calculate seeds and lambdas
void freeXorCompute()
{
	for (int g = 0; g < cyc->numOfXORGates; g++)
	{
		//XOR lambdas
		cyc->specialGates[g]->output->lambda = cyc->specialGates[g]->input1->lambda ^ cyc->specialGates[g]->input2->lambda;//out=in1^in2

		//XOR seeds
		cyc->specialGates[g]->output->seed = _mm_xor_si128(cyc->specialGates[g]->input1->seed, cyc->specialGates[g]->input2->seed);//out=in1^in2
	}
}


//functions for multiplying the lambdas

//Using correlated OT
void sendLambdaC(int player, CBitVector Delta)
{
	senders[player]->OTsendC(1, PAD(cyc->numOfANDGates, 8), X1[player], X2[player], Delta);
}
//Using correlated OT
void receiveLambdaC(int player, CBitVector choices)
{
	OTs[player].Create(PAD(cyc->numOfANDGates, 8), 1);
	receivers[player]->OTreceiveC(1, PAD(cyc->numOfANDGates, 8), choices, OTs[player]);
}
//not used (using correlated OT function instead)
void sendLambda(int player)
{
	senders[player]->OTsend(1, PAD(cyc->numOfANDGates,8), X1[player], X2[player]);
}
//not used (using correlated OT function instead)
void receiveLambda(int player, CBitVector choices)
{
	OTs[player].Create(PAD(cyc->numOfANDGates, 8),1);
	receivers[player]->OTreceive(1, PAD(cyc->numOfANDGates, 8), choices, OTs[player]);
}

//not used (using correlated OT function instead)
void mulLambdas()
{
	thread* threads = new thread[2*numOfParties];
	//initialize Choices vector and X2 vector according to lambdasIn2[g] and lambdasIn1[g] respectively (lambdas vector for easy XORing)
	CBitVector choicesLambda;//The choices are lambdaIn2
	choicesLambda.Create(PAD(cyc->numOfANDGates, 8), 1);

	choicesLambda.Reset();
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		choicesLambda.Set(g, cyc->regularGates[g]->input2->lambda);//lambdasIn2[g]
	}

	X1 = new CBitVector[numOfParties];//A random vector
	X2 = new CBitVector[numOfParties];//X1^lambdaIn1
	bool r, l;
	for (int p = 0; p < numOfParties; p++)
	{
		if (p != partyNum)
		{
			X1[p].Create(PAD(cyc->numOfANDGates, 8), 1);
			X1[p].Reset();
			//Create X2 and set the values
			X2[p].Create(PAD(cyc->numOfANDGates, 8), 1);
			X2[p].Reset();
			for (int g = 0; g < cyc->numOfANDGates; g++)
			{
				r = RAND_BIT;

				X1[p].Set(g, r);//X1 is random
				//if player chooses 1 (i.e., lambdaIn2[g] of otherplayer=1) then give lambdaIn1[g] + the random element
				l = cyc->regularGates[g]->input1->lambda;
				X2[p].Set(g, r^l);
			}
		}
	}

	OTs = new CBitVector[numOfParties];//make room for incoming OTs
	//Do the OT, using threads
	for (int p = 0; p < numOfParties; p ++)
	{
		if (p != partyNum)
		{
			threads[2*p] = std::thread(sendLambda, p);
			threads[2*p + 1] = std::thread(receiveLambda, p, choicesLambda);
		}
	}

	//wait for all OT to finish
	for (int i = 0; i < 2 * numOfParties; i++)
		if (i != 2 * partyNum && i != 2 * partyNum + 1)
				threads[i].join();

	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		cyc->regularGates[g]->mulLambdas = cyc->regularGates[g]->input1->lambda & cyc->regularGates[g]->input2->lambda;
	}
	
	//sum the random bits (which were used for masking)
	//and
	//sum the received lambdas (which are either random or lambda2+random)
	//so that
	//multLambda[g]=lambdaIn1[g]*lambdaIn2[g]
	for (int p = 0; p < numOfParties; p++)
	{
		
		if (p != partyNum)
		{
			for (int g = 0; g < cyc->numOfANDGates; g++)
			{
				cyc->regularGates[g]->mulLambdas ^= (X1[p].GetInt(g, 1) ^ OTs[p].GetInt(g, 1));
			}
		}
	}
	delete[] threads;
}

//Using correlated OT
void mulLambdasC()
{
	thread* threads = new thread[2 * numOfParties];

	CBitVector choicesLambda;//The choices are lambdaIn2
	choicesLambda.Create(PAD(cyc->numOfANDGates, 8), 1);

	choicesLambda.Reset();
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		choicesLambda.Set(g, cyc->regularGates[g]->input2->lambda);//lambdasIn2[g]
	}

	//X1^X2=lambdaIn1
	X1 = new CBitVector[numOfParties];
	X2 = new CBitVector[numOfParties];
	//Delta for correlated OT - lambdaIn1
	CBitVector Delta;
	Delta.Create(PAD(cyc->numOfANDGates, 8), 1);

	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		Delta.Set(g, cyc->regularGates[g]->input1->lambda);
	}

	for (int p = 0; p < numOfParties; p++)
	{
		if (p != partyNum)
		{
			X1[p].Create(PAD(cyc->numOfANDGates, 8), 1);
			X2[p].Create(PAD(cyc->numOfANDGates, 8), 1);
		}
	}

	OTs = new CBitVector[numOfParties];//make room for incoming OTs
	//Do the OT, using threads
	for (int p = 0; p < numOfParties; p++)
	{
		if (p != partyNum)
		{
			threads[2 * p] = std::thread(sendLambdaC, p, Delta);
			threads[2 * p + 1] = std::thread(receiveLambdaC, p, choicesLambda);
		}
	}

	//wait for all OT to finish
	for (int i = 0; i < 2 * numOfParties; i++)
		if (i != 2 * partyNum && i != 2 * partyNum + 1)
			threads[i].join();

	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		cyc->regularGates[g]->mulLambdas = cyc->regularGates[g]->input1->lambda & cyc->regularGates[g]->input2->lambda;
	}

	//sum the random bits (which were used for masking)
	//and
	//sum the received lambdas (which are either random or lambda2+random)
	//so that
	//multLambda[g]=lambdaIn1[g]*lambdaIn2[g] 
	for (int p = 0; p < numOfParties; p++)
	{

		if (p != partyNum)
		{
			for (int g = 0; g < cyc->numOfANDGates; g++)
			{
				cyc->regularGates[g]->mulLambdas ^= (X1[p].GetInt(g, 1) ^ OTs[p].GetInt(g, 1));//****unification of the 2 commands below
			}
		}
	}

	Delta.delCBitVector();
	choicesLambda.delCBitVector();
	
	delete[] threads;
}




//Functions for multiplying R with (lambdaIn1*lambdaIn2+lambdaOut)

//Using correlated OT
void sendRC(int player,CBitVector Delta)
{
	senders[player]->OTsendC(128, 3 * cyc->numOfANDGates, X1Long[player], X2Long[player], Delta);
}

//Using correlated OT
void receiveRC(int player, CBitVector choices)
{
	OTLongs[player].Create(3 * cyc->numOfANDGates, 128);
	receivers[player]->OTreceiveC(128, 3 * cyc->numOfANDGates, choices, OTLongs[player]);
}

//Using correlated OT
void mulRC()
{
	
	CBitVector choices;//merging of choicesAg, choicesBg, choicesCg, and choicesDg
	
	//allocation of space for OT sending
	X1Long = new CBitVector[numOfParties];
	X2Long = new CBitVector[numOfParties];
	
	//Delta is just copies of R^i
	CBitVector Delta;
	Delta.Create(3 * cyc->numOfANDGates,128);
	for (int g = 0; g < 3*cyc->numOfANDGates; g++)
		Delta.Set(g, R);

	OTLongs = new CBitVector[numOfParties];//make room for incoming OTs

	choices.Create(3 * cyc->numOfANDGates);
	bool tmp;
	for (int g = 0; g < cyc->numOfANDGates; g++)//initialize the vector(s)
	{

		tmp = cyc->regularGates[g]->mulLambdas^cyc->regularGates[g]->output->lambda;//tmp= share of lambdaIn1*lambdaIn2^lambdaOut
																							//Use shift to adjust to gate.
		int sh = cyc->regularGates[g]->sh;


		choices.Set(g * 3, tmp);//multLambda[g] ^ lambda[out[g]] for Ag
		if (tmp)//local multiplication of R*(lambdaIn1*lambdaIn2+lambdaOut)
		{
			cyc->regularGates[g]->G[0 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[0 ^ sh][partyNum], R);
			cyc->regularGates[g]->G[3 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][partyNum], R);
		}

		choices.Set(g * 3 + 1, tmp^cyc->regularGates[g]->input1->lambda);//multLambda[g] ^ lambdas[in1[g]] ^ lambda[out[g]] for Bg [multLambda ^ lambdaIn1=lambdaIn1*(!lambdaIn2) ]
		if (tmp^cyc->regularGates[g]->input1->lambda)
		{
			cyc->regularGates[g]->G[1 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[1 ^ sh][partyNum], R);
			cyc->regularGates[g]->G[3 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][partyNum], R);
		}

		choices.Set(g * 3 + 2, tmp^cyc->regularGates[g]->input2->lambda);//multLambda[g] ^ lambdas[in2[g]] ^ lambda[out[g]] for Cg
		if (tmp^cyc->regularGates[g]->input2->lambda)
		{
			cyc->regularGates[g]->G[2 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[2 ^ sh][partyNum], R);
			cyc->regularGates[g]->G[3 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][partyNum], R);
		}

		cyc->regularGates[g]->G[3 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][partyNum], R);//XOR previous results and Xor R for Dg
			
	}

	for (int p = 0; p < numOfParties; p++)
	{
		if (p != partyNum)
		{
			X1Long[p].Create(3 * cyc->numOfANDGates, 128);//need a different random vector for each player (because they can collude)
			X2Long[p].Create(3 * cyc->numOfANDGates, 128);
		}
	}
	//Do the OT, using threads

	thread	*threads = new thread[2 * numOfParties];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(sendRC, i, Delta);//sending using Correlated OT
			threads[i * 2 + 1] = thread(receiveRC, i, choices);
		}
	}
	//wait for all OT threads to finish
	for (int i = 0; i < 2 * numOfParties; i++)
		if (i != 2 * partyNum && i != 2 * partyNum + 1)
			threads[i].join();
	
	////build Ag,Bg,Cg,Dg for all gates
	__m128i recR, recR2, recR3;
	__m128i recX1, recX12, recX13;
	for (int p = 0; p < numOfParties; p++)
	{
		__m128i * otResult = (__m128i*) OTLongs[p].GetArr();
		__m128i * otx1 = (__m128i*) X1Long[p].GetArr();
		if (p != partyNum)	for (int g = 0; g < cyc->numOfANDGates; g++)
		{
			int sh = cyc->regularGates[g]->sh;
			recR = otResult[3 * g];

			cyc->regularGates[g]->G[0 ^ sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[0 ^ sh][p], recR);
			recR2 = otResult[3 * g + 1];

			cyc->regularGates[g]->G[1 ^ sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[1 ^ sh][p], recR2);
			recR3 = otResult[3 * g + 2];

			cyc->regularGates[g]->G[2 ^ sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[2 ^ sh][p], recR3);

			cyc->regularGates[g]->G[3 ^ sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][p], recR);
			cyc->regularGates[g]->G[3 ^ sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][p], recR2);
			cyc->regularGates[g]->G[3 ^ sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][p], recR3);

			recX1 = otx1[3 * g];
			recX12 = otx1[3 * g + 1];
			recX13 = otx1[3 * g + 2];

			cyc->regularGates[g]->G[0 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[0 ^ sh][partyNum], recX1);
			cyc->regularGates[g]->G[1 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[1 ^ sh][partyNum], recX12);
			cyc->regularGates[g]->G[2 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[2 ^ sh][partyNum], recX13);
			//*************
			cyc->regularGates[g]->G[3 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][partyNum], recX1);
			cyc->regularGates[g]->G[3 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][partyNum], recX12);
			cyc->regularGates[g]->G[3 ^ sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3 ^ sh][partyNum], recX13);
			//*************
		}
	}

	Delta.delCBitVector();
	choices.delCBitVector();
	
	delete[] threads;
}

//not used (using correlated OT function instead)
void receiveR(int player, CBitVector choices)
{
	OTLongs[player].Create(3*cyc->numOfANDGates,128);
	receivers[player]->OTreceive(128, 3 * cyc->numOfANDGates, choices, OTLongs[player]);
}

//not used (using correlated OT function instead)
void sendR(int player)
{
	senders[player]->OTsend(128, 3 * cyc->numOfANDGates, X1Long[player], X2Long[player]);	
}

//not used (using correlated OT function instead)
void mulR()
{
	CBitVector choices;

	X1Long = new CBitVector[numOfParties];
	X2Long = new CBitVector[numOfParties];
	OTLongs = new CBitVector[numOfParties];//make room for incoming OTs

	choices.Create(3*cyc->numOfANDGates);
	bool tmp;
	for (int g = 0; g < cyc->numOfANDGates; g++)//initialize the vector(s)
	{

		tmp = cyc->regularGates[g]->mulLambdas^cyc->regularGates[g]->output->lambda;
		//Use shift to adjust to gate.
		int sh = cyc->regularGates[g]->sh;
		
		
		choices.Set(g * 3, tmp);//multLambda[g] ^ lambda[out[g]] for Ag
		if (tmp)//local multiplication of R*(lambdaIn1*lambdaIn2+lambdaOut)
		{
			cyc->regularGates[g]->G[0^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[0^sh][partyNum], R);
			cyc->regularGates[g]->G[3^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][partyNum], R);
		}
		//cout << "Ag:"; print(cyc->regularGates[g]->G[0],numOfParties);
		
		choices.Set(g * 3 + 1, tmp^cyc->regularGates[g]->input1->lambda);//multLambda[g] ^ lambdas[in1[g]] ^ lambda[out[g]] for Bg [multLambda ^ lambdaIn1=lambdaIn1*(!lambdaIn2) ]
		if (tmp^cyc->regularGates[g]->input1->lambda)
		{
			cyc->regularGates[g]->G[1^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[1^sh][partyNum], R);
			cyc->regularGates[g]->G[3^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][partyNum], R);
		}
		
		choices.Set(g * 3 + 2, tmp^cyc->regularGates[g]->input2->lambda);//multLambda[g] ^ lambdas[in2[g]] ^ lambda[out[g]] for Cg
		if (tmp^cyc->regularGates[g]->input2->lambda)
		{
			cyc->regularGates[g]->G[2^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[2^sh][partyNum], R);
			cyc->regularGates[g]->G[3^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][partyNum], R);
		}
		
		cyc->regularGates[g]->G[3^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][partyNum], R);//XOR previous results and Xor R for Dg
		
	}
		
	for (int p = 0; p < numOfParties; p++)
	{
		if (p != partyNum)
		{
			X1Long[p].Create(3 * cyc->numOfANDGates, 128);//need a different random vector for each player (because they can collude)
	
			__m128i rand1, rand2, rand3;// rand4
			X2Long[p].Create(cyc->numOfANDGates * 3, 128);
			for (int g = 0; g < cyc->numOfANDGates; g++)
			{
				int sh = cyc->regularGates[g]->sh;
				
				rand1 = RAND; rand2 = RAND; rand3 = RAND;
				X1Long[p].Set(3 * g, rand1); 
				
				cyc->regularGates[g]->G[0^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[0^sh][partyNum], rand1);
				
				X1Long[p].Set(3 * g + 1, rand2); 
				cyc->regularGates[g]->G[1^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[1^sh][partyNum], rand2);
				
				X1Long[p].Set(3 * g + 2, rand3); 
				cyc->regularGates[g]->G[2^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[2^sh][partyNum], rand3);

				//Logic: R(-lambdaIn1)(-lambdaIn2)=R(lambdaIn1+1)(lambdaIn2+1)=R*lambdaIn1*lambdaIn2+R*lambdaIn1+R*lambdaIn2 (+R, which is done locally)
				
				cyc->regularGates[g]->G[3^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][partyNum], rand1);
				cyc->regularGates[g]->G[3^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][partyNum], rand2);
				cyc->regularGates[g]->G[3^sh][partyNum] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][partyNum], rand3);
				
				//every 128-bit element of X2Long is R XORed with the (random) element of X1Long
				X2Long[p].Set(g * 3, _mm_xor_si128(R, rand1)); // X1^R
				X2Long[p].Set(g * 3 + 1, _mm_xor_si128(R, rand2));
				X2Long[p].Set(g * 3 + 2, _mm_xor_si128(R, rand3));
				
			}
		}
	}
	
	//Do the OT, using threads
	thread	*threads = new thread[2 * numOfParties];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(sendR, i);
			threads[i * 2 + 1] = thread(receiveR, i, choices);
		}
	}
	//wait for all OT to finish
	for (int i = 0; i < 2 * numOfParties; i++)
		if (i != 2 * partyNum && i != 2 * partyNum + 1)
			threads[i].join();

		
		
	////build Ag,Bg,Cg,Dg for all gates
	__m128i recR, recR2, recR3;
	for (int p = 0; p < numOfParties; p++)
	{
		if (p!=partyNum)	for (int g = 0; g < cyc->numOfANDGates; g++)
		{
			int sh = cyc->regularGates[g]->sh;
			recR = OTLongs[p].GetBlock(3 * g);
			
			cyc->regularGates[g]->G[0^sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[0^sh][p], recR);
			recR2 = OTLongs[p].GetBlock(3 * g+1);
			
			cyc->regularGates[g]->G[1^sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[1^sh][p], recR2);
			recR3 = OTLongs[p].GetBlock(3 * g+2);
			
			cyc->regularGates[g]->G[2^sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[2^sh][p], recR3);
			
			cyc->regularGates[g]->G[3^sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][p], recR);
			cyc->regularGates[g]->G[3^sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][p], recR2);
			cyc->regularGates[g]->G[3^sh][p] = _mm_xor_si128(cyc->regularGates[g]->G[3^sh][p], recR3);
		}
	}
	delete[] threads;
}






__m128i* XORsuperseeds(__m128i *superseed1, __m128i *superseed2)
{
	
	void* test=_aligned_malloc(numOfParties*sizeof(__m128i), 16);
	
	__m128i* ans = static_cast<__m128i *> (test); 
	for (int p = 0; p < numOfParties; p++)
	{
		ans[p] = _mm_xor_si128(superseed1[p], superseed2[p]);
	}
	return ans;
}

void XORsuperseedsNew(__m128i *superseed1, __m128i *superseed2, __m128i *out)
{
	for (int p = 0; p < numOfParties; p++)
	{
		out[p] = _mm_xor_si128(superseed1[p], superseed2[p]);
	}
}



void computeGates()
{
	if (majType==0)
		bigMemoryAllocation = static_cast<__m128i *>(_aligned_malloc(4*cyc->numOfANDGates*numOfParties * sizeof(__m128i)+numOfOutputWires, 16));
	else
		bigMemoryAllocation = static_cast<__m128i *>(_aligned_malloc((4 * cyc->numOfANDGates*numOfParties+numOfOutputWires) * sizeof(__m128i), 16));
	
	__m128i seed1, seed2;
	Gate* gate;
	for (int g = 0; g < cyc->numOfANDGates; g++)//add the seeds to Ag, Bg, Cg, and Dg
	{
		gate = cyc->regularGates[g];

		seed1 = gate->input1->seed;
		seed2 = gate->input2->seed;

		//Compute the pseudorandom functions for Ag,Bg,Cg,Dg
		for (int i = 0; i < 4; i++)
			gate->G[i] = &bigMemoryAllocation[(4*g+i)*numOfParties];

		PSEUDO_RANDOM_FUNCTION(seed1, seed2, gate->gateNumber, numOfParties, gate->G[0]);//in1,in2
		PSEUDO_RANDOM_FUNCTION(seed1, _mm_xor_si128(seed2, R), gate->gateNumber, numOfParties, gate->G[1]);//in1,in2^R
		PSEUDO_RANDOM_FUNCTION(_mm_xor_si128(seed1, R), seed2, gate->gateNumber, numOfParties, gate->G[2]);//in1^R,in2
		PSEUDO_RANDOM_FUNCTION(_mm_xor_si128(seed1, R), _mm_xor_si128(seed2, R), gate->gateNumber, numOfParties, gate->G[3]);//in1^r,in2^R
		
		
		gate->G[0][partyNum] = _mm_xor_si128(gate->G[0][partyNum], gate->output->seed);
		gate->G[1][partyNum] = _mm_xor_si128(gate->G[1][partyNum], gate->output->seed);
		gate->G[2][partyNum] = _mm_xor_si128(gate->G[2][partyNum], gate->output->seed);
		gate->G[3][partyNum] = _mm_xor_si128(gate->G[3][partyNum], gate->output->seed);
	}
}


void newSendGate(__m128i* toSend, int player)
{
	int outputSize = numOfOutputWires;//I think this should be padded to a multiple of 128
	int total = sizeof(__m128) * 4 * cyc->numOfANDGates *numOfParties+outputSize;
	communicationSenders[player]->sendMsg(toSend, total);
}

void newReceiveGate(int player)
{
	int outputSize = numOfOutputWires;//I think this should be padded to a multiple of 128
	int total = sizeof(__m128) * 4 * cyc->numOfANDGates *numOfParties+outputSize;
	playerSeeds[player] = static_cast<__m128i *>(_aligned_malloc(total, 16));//assign space;
	communicationReceivers[player]->reciveMsg(playerSeeds[player], total);
}


void exchangeGates()
{
	__m128i *toSend;

	for (int o = 0; o < numOfOutputWires; o++)
		((bool*)bigMemoryAllocation)[4 * cyc->numOfANDGates *numOfParties*sizeof(__m128i) + o] = cyc->outputWires.playerWires[o]->lambda;
	toSend = bigMemoryAllocation;

	playerSeeds = new __m128i*[numOfParties];
	thread *threads = new thread[numOfParties * 2];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(newSendGate, toSend,i);//send inputs to player i
			threads[i * 2 + 1] = thread(newReceiveGate, i);//receive inputs from player i
		}
	}
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != 2 * partyNum && i != (2 * partyNum + 1))
		{
			threads[i].join();//wait for all threads to finish
		}
	}

	for (int party = 0; party < numOfParties; party++)
	{
		if (party != partyNum)
		{
			for (int g = 0; g < cyc->numOfANDGates; g++)
			{
				for (int p = 0; p < numOfParties; p++)
				{
					cyc->regularGates[g]->G[0][p] = _mm_xor_si128(playerSeeds[party][4 * g*numOfParties + p], cyc->regularGates[g]->G[0][p]);
					cyc->regularGates[g]->G[1][p] = _mm_xor_si128(playerSeeds[party][(4 * g + 1)*numOfParties + p], cyc->regularGates[g]->G[1][p]);
					cyc->regularGates[g]->G[2][p] = _mm_xor_si128(playerSeeds[party][(4 * g + 2)*numOfParties + p], cyc->regularGates[g]->G[2][p]);
					cyc->regularGates[g]->G[3][p] = _mm_xor_si128(playerSeeds[party][(4 * g + 3)*numOfParties + p], cyc->regularGates[g]->G[3][p]);
				}
			}
			
			for (int o = 0; o < numOfOutputWires;o++)
				cyc->outputWires.playerWires[o]->lambda ^= ((bool*) playerSeeds[party])[(sizeof(__m128i)/sizeof(bool))* 4 * cyc->numOfANDGates *numOfParties + o];
		}
	}
	
	//free memory
	delete[] threads;
	for (int p = 0; p < numOfParties; p++)
		if (p!=partyNum)
			_aligned_free(playerSeeds[p]);
	delete[] playerSeeds;
}

//Online phase communication functions

void sendInput(bool* inputs, int player)
{
	communicationSenders[player]->sendMsg(inputs, cyc->playerArray[partyNum].playerBitAmount);// +numOfOutputWires);
}

void receiveInputs(int player)
{
	bool* playerInputs = new bool[cyc->playerArray[player].playerBitAmount];
	
	communicationReceivers[player]->reciveMsg(playerInputs, cyc->playerArray[player].playerBitAmount);
	
	for (int i = 0; i < cyc->playerArray[player].playerBitAmount; i++)//store value in correct place
	{
		cyc->playerArray[player].playerWires[i]->value = playerInputs[i];
	}
	
	delete[] playerInputs;//free memory
}


void exchangeInputs()//send and receive inputs (XORed with lambda)
{
	bool *toSend = new bool[cyc->playerArray[partyNum].playerBitAmount];// +numOfOutputWires];
	for (int i = 0; i < cyc->playerArray[partyNum].playerBitAmount; i++)
	{
		//the value to send/use for calculations is the real value XORed with lambda
		toSend[i] = cyc->playerArray[partyNum].playerWires[i]->value;
	}

	thread *threads = new thread[numOfParties * 2];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(sendInput, toSend, i);//send inputs to player i
			threads[i * 2 + 1] = thread(receiveInputs, i);//receive inputs from player i
		}
	}
	
	
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != 2 * partyNum && i != (2 * partyNum + 1))
			threads[i].join();//wait for all threads to finish
	}

	delete[] threads;

	delete[] toSend;
}


void sendSeed(__m128i* inputs, int player)
{
	communicationSenders[player]->sendMsg(inputs, sizeof(__m128i)*numOfInputWires);
}

void receiveSeed(int player)
{
	__m128i* playerSeeds = static_cast<__m128i *>(_aligned_malloc(numOfInputWires*sizeof(__m128i), 16));//allocate temporary memory to store response
	communicationReceivers[player]->reciveMsg(playerSeeds, sizeof(__m128i)*numOfInputWires);//receive values from player
	
	int count = 0;
	for (int p = 0; p < numOfParties; p++)
	{
		for (int i = 0; i < cyc->playerArray[p].playerBitAmount; i++)
		{
			cyc->playerArray[p].playerWires[i]->superseed[player] = playerSeeds[count];
			count++;
		}
	}
	_aligned_free(playerSeeds);//free memory
}


void exchangeSeeds()
{
	__m128i *toSend = static_cast<__m128i *>(_aligned_malloc(numOfInputWires*sizeof(__m128i), 16));//assign space;
	int count = 0;
	for (int p = 0; p < numOfParties; p++)
	{
		for (int i = 0; i < cyc->playerArray[p].playerBitAmount; i++)
		{
			toSend[count] = cyc->playerArray[p].playerWires[i]->seed;
			if (cyc->playerArray[p].playerWires[i]->value)
				toSend[count] = _mm_xor_si128(toSend[count], R);
			cyc->playerArray[p].playerWires[i]->superseed[partyNum] = toSend[count];//update local superseed
			count++;
		}
	}

	thread *threads = new thread[numOfParties * 2];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(sendSeed,toSend,i);//send inputs to player i
			threads[i * 2 + 1] = thread(receiveSeed, i);//receive inputs from player i
		}
	}
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != 2 * partyNum && i != (2 * partyNum + 1))
		{
			threads[i].join();//wait for all threads to finish
		}
	}
	

	delete[] threads;
	_aligned_free(toSend);//free memory
}

//online phase - compute output
bool* computeOutputs()
{

	__m128i* arr = static_cast<__m128i *>(_aligned_malloc(numOfParties * sizeof(__m128i), 16));
	bool* outputs = new bool[cyc->outputWires.playerBitAmount];
	bool valueIn1;
	bool valueIn2;
	for (int g = 0; g < cyc->amountOfGates; g++)
	{

		if (cyc->gateArray[g].flagNOMUL)//free XOR
		{
			XORsuperseedsNew(cyc->gateArray[g].input1->superseed, cyc->gateArray[g].input2->superseed, cyc->gateArray[g].output->superseed);
		}
		else//Regular gate computation
		{
		  
			valueIn1 = getValue(cyc->gateArray[g].input1, partyNum);
			valueIn2 = getValue(cyc->gateArray[g].input2, partyNum);

			if (!valueIn1&&!valueIn2)
				{cyc->gateArray[g].output->superseed = cyc->gateArray[g].G[0];}
			else if (!valueIn1&&valueIn2)
				{cyc->gateArray[g].output->superseed = cyc->gateArray[g].G[1];}
			else if (valueIn1&&!valueIn2)
				{cyc->gateArray[g].output->superseed = cyc->gateArray[g].G[2];}
			else if (valueIn1&&valueIn2)
				{cyc->gateArray[g].output->superseed = cyc->gateArray[g].G[3];}

			for (int p = 0; p < numOfParties; p++)
			{
				PSEUDO_RANDOM_FUNCTION(cyc->gateArray[g].input1->superseed[p], cyc->gateArray[g].input2->superseed[p], cyc->gateArray[g].gateNumber, numOfParties, arr);

				XORsuperseedsNew(cyc->gateArray[g].output->superseed, arr, cyc->gateArray[g].output->superseed);
			}
		}
	}
	for (int w = 0; w < cyc->outputWires.playerBitAmount; w++)
	{
		outputs[w] = getTrueValue(cyc->outputWires.playerWires[w], partyNum);
	}
	_aligned_free(arr);
	return outputs;
}

//memory release functions

void deletePartial()
{
	_aligned_free(bigMemoryAllocation);

	freeCircuitPartial(cyc);
	if (majType > 0)
		deletePartialBGW();
	else
	{
		for (int p = 0; p < numOfParties; p++)
		{
		X1[p].delCBitVector();
		X2[p].delCBitVector();
		OTs[p].delCBitVector();
		X1Long[p].delCBitVector();
		X2Long[p].delCBitVector();
		OTLongs[p].delCBitVector();
		}
		delete[] X1;
		delete[] X2;
		delete[] OTs;
		delete[] X1Long;
		delete[] X2Long;
		delete[] OTLongs;
	}
}

void deleteAll()
{
	if (majType == 0)//no honest majority - close OT connections
	{
		for (int i = 0; i < numOfParties; i++)
		{
			if (i != partyNum)
			{
				//There seems to be a problem with the closing of the connections here
				delete receivers[i];
				delete senders[i];
			}
		}
		delete[] receivers;
		delete[] senders;
	}
	else
		deleteAllBGW();
	
	//close connection
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			delete communicationReceivers[i];
			delete communicationSenders[i];
		}
	}
	delete[] communicationReceivers;
	delete[] communicationSenders;

	delete[] addrs;

	//delete Circuit
	freeCircle(cyc);

	AES_free();
}

//synchronization functions
void sendBit(int player, bool* toSend)
{
	communicationSenders[player]->sendMsg(toSend, 1);
}

void receiveBit(int player)
{
	bool *sync = new bool[1];
	communicationReceivers[player]->reciveMsg(sync,1);
}

void synchronize()
{
	bool* toSend=new bool[1];
	toSend[0] = 0;
	thread *threads = new thread[numOfParties * 2];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(sendBit, i, toSend);//send inputs to player i
			threads[i * 2 + 1] = thread(receiveBit, i);//receive inputs from player i
		}
	}
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != 2 * partyNum && i != (2 * partyNum + 1))
		{
			threads[i].join();//wait for all threads to finish
		}
	}
	delete[] threads;
}
