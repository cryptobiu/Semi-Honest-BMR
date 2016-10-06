/*
 * 	BMR_BGW.cpp
 * 
 *      Author: Aner Ben-Efraim 
 * 	
 * 	year: 2016
 * 
 */

#include "BMR_BGW.h"

/*
VARIABLES FOR BGW
*/
//Share of R^i, for each i
__m128i* RShare;
//The degree of the secret-sharings before multiplications
int degSmall;
//The degree of the secret-sharing after all multiplications
int degBig;
//The version honest majority we assume
int majType;
//vectors for storing shares
__m128i* shares;
//number of shares
int totalSharesNumber;


//Large vectors for storing secrets and coefficients
//all the secrets to be shared
__m128i* allSecrets;
//all the coefficients of the polynomials
__m128i* allCoefficients;
//number of polynomials with deg==smallDeg
int numOfSmallDegPolynomials;


//Initialize the necessary values for BGW, according to numer of parties and assumption on majority, and precompute the powers and coefficients
void InitializeBGW(int hm)
{
	majType = hm;
	if (hm ==2)// 2/3 honest majority
	{
		degSmall = ((numOfParties+2) / 3) - 1;
		degBig = 3 * degSmall;
	}
	else// regular honest majority
	{
		degSmall = ((numOfParties + 1) / 2) - 1;
		degBig = 2 * degSmall;
	}
	preComputeCoefficients(numOfParties, degBig);
	if (majType == 3)
		preComputeReconstructionCoefficients(numOfParties, degSmall);
}

void generatePolynomials()
{
	
	allSecrets= static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	int totalNumberOfCoef;
	if (majType != 3)
	{
		numOfSmallDegPolynomials = 1 + cyc->numOfANDGates + cyc->numOfInputWires;//R and lambdas are degSmall(lambdas for output of XOR gates are computed using free-XOR)
		totalNumberOfCoef = numOfSmallDegPolynomials*degSmall + (totalSharesNumber - numOfSmallDegPolynomials)*degBig;//The rest (PRGs) are degBig
	}
	else//extra reduction round - all polynomials are of degree degSmall
	{
		numOfSmallDegPolynomials = totalSharesNumber;
		totalNumberOfCoef = totalSharesNumber*degSmall;
	}

	allCoefficients= static_cast<__m128i *>(_aligned_malloc(totalNumberOfCoef*sizeof(__m128i), 16));
	for (int i = 0; i < totalNumberOfCoef; i++)
		allCoefficients[i] = RAND;

	allSecrets[0] = R;
	
	int count = 1;//Start counting from 1 because 0 is shares of R.
	for (int p = 0; p < numOfParties; p++)
		for (int i = 0; i < cyc->playerArray[p].playerBitAmount; i++)
		{
			if (cyc->playerArray[p].playerWires[i]->lambda)
				allSecrets[count] = ONE;
			else
				allSecrets[count] = ZERO;
			count++;
		}
	
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		if (cyc->regularGates[g]->output->lambda)
			allSecrets[count]= ONE;
		else
			allSecrets[count] = ZERO;
		count++;
	}
	
	for (int g = 0; g < cyc->numOfANDGates; g++)
		for (int i = 0; i < 4; i++)
			for (int p = 0; p < numOfParties; p++)
			{
				allSecrets[count] = cyc->regularGates[g]->G[i][p];
				count++;
			}
	
	//shares of ZERO - not used
	if (totalSharesNumber > (4 * numOfParties)*cyc->numOfANDGates + (cyc->numOfANDGates + numOfInputWires) + 1) 
	{
		cout << "extra shares" << endl;
		for (int g = 0; g < cyc->numOfANDGates; g++)
			for (int p = 0; p < numOfParties; p++)
			{
				allSecrets[count] = ZERO;
				count++;
			}
	}	

}
#ifdef MULTTHREADNUM
void computeShareswThreads(int p, __m128i* ans, int tNum)
{
	int start = (tNum*totalSharesNumber) / MULTTHREADNUM;
	int end= ((tNum+1)*totalSharesNumber) / MULTTHREADNUM;
	int count;
	if (start < numOfSmallDegPolynomials)
		count = start*degSmall;
	else
		count = numOfSmallDegPolynomials*degSmall + (start - numOfSmallDegPolynomials)*degBig;
	for (int i = start; i < end; i++)
	{
		ans[i] = allSecrets[i];
		if (i < numOfSmallDegPolynomials)
		{
			for (int deg = 1; deg < degSmall + 1; deg++)
			{
				//notice the i-1, due to POW(X,0) not saved (always 1...)	
				ans[i] = ADD(ans[i], MULHZ(allCoefficients[count], powers[p*degBig + (deg - 1)]));
				count++;																
			}
		}
		else
		{
			for (int deg = 1; deg < degBig + 1; deg++)
			{
				//notice the i-1, due to POW(X,0) not saved (always 1...)
				ans[i] = ADD(ans[i], MULHZ(allCoefficients[count], powers[p*degBig + (deg - 1)]));//POW(SETX(p), i)));//
				count++;
			}
		}
	}
}
#endif



void computeShares(int p, __m128i* ans)
{
	int count = 0;
	for (int i = 0; i < totalSharesNumber; i++)
	{

#ifndef PIPELINED_MULTIPLICATION
		ans[i] = allSecrets[i];
		if (i < numOfSmallDegPolynomials)
		{
			//cout<< i << endl;
			for (int deg = 1; deg < degSmall+1; deg++)
			{
				//notice the i-1, due to POW(X,0) not saved (always 1...)	
				ans[i] = ADD(ans[i], MULHZ(allCoefficients[count], powers[p*degBig +(deg-1)]));
				count++;
			}
		}
		else
		{
			for (int deg = 1; deg < degBig + 1; deg++)
			{
				//notice the i-1, due to POW(X,0) not saved (always 1...)
				ans[i] = ADD(ans[i], MULHZ(allCoefficients[count], powers[p*degBig + (deg - 1)]));
				count++;
			}
		}
#else
		if (i < numOfSmallDegPolynomials)
		{
			gfDotProductPipedHZ(allCoefficients+count, powers+(p*degBig), degSmall, &ans[i]);
			count += degSmall;
		}
		else
		{
			gfDotProductPipedHZ(allCoefficients+count, powers+(p*degBig), degBig, &ans[i]);
			count += degBig;
		}
		ans[i] = ADD(ans[i],allSecrets[i]);
#endif
	}
}

void sendShares(int party)
{
	__m128i* sendShares = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	//compute shares here
#ifndef MULTTHREADNUM
	computeShares(party, sendShares);
#else
	thread *threads = new thread[MULTTHREADNUM];
	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i] = thread(computeShareswThreads,party,sendShares,i);//add (XOR) the local shares


	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i].join();//wait for all threads to finish
#endif

	communicationSenders[party]->sendMsg(sendShares, totalSharesNumber*sizeof(__m128i));//VERIFY THIS

	_aligned_free(sendShares);
}

void receiveShares(int party)
{
	__m128i* receivedShares = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	//receive shares

	communicationReceivers[party]->reciveMsg(receivedShares, totalSharesNumber*sizeof(__m128i));//VERIFY THIS

	//save share of R^i
	RShare[party] = receivedShares[0];

	//add to local shares/shares from other parties
	mtxVal.lock();
		XORvectors(shares, receivedShares, shares, totalSharesNumber);
	mtxVal.unlock();
	_aligned_free(receivedShares);
}

void addLocalShares()
{
	__m128i* localShares= static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	
#ifndef MULTTHREADNUM
	computeShares(partyNum, localShares);
#else
	thread *threads = new thread[MULTTHREADNUM];
	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i] = thread(computeShareswThreads, partyNum, localShares, i);//add (XOR) the local shares


	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i].join();//wait for all threads to finish
#endif
	//save share of R^i
	RShare[partyNum] = localShares[0];

	//add to shares from other parties
	mtxVal.lock();
		XORvectors(shares, localShares, shares,totalSharesNumber);
	mtxVal.unlock();
	_aligned_free(localShares);
}

void secretShare()
{
	
	//initialize vector for storing shares				
	totalSharesNumber = (4 * numOfParties)*cyc->numOfANDGates + (cyc->numOfANDGates + numOfInputWires) + 1;//PRGs + lambda for each wire + R 

	shares = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	memset(shares, 0, totalSharesNumber*sizeof(*shares)); // for heap-allocated arrays, where N is the number of elements
	//alocate memory for storing shares of R
	RShare= static_cast<__m128i *>(_aligned_malloc(numOfParties*sizeof(__m128i), 16));//for saving shares of R^i
	
	generatePolynomials();

	thread *threads = new thread[numOfParties * 2];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(sendShares, i);//generate and send shares to player i.
			threads[i * 2 + 1] = thread(receiveShares, i);//receive shares from player i and sum (XOR) them. Also saves share of R^i
		}
		else
			threads[i * 2] = thread(addLocalShares);//add (XOR) the local shares
	}
	
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != (2 * partyNum + 1))//i != 2 * partyNum &&
		{
			threads[i].join();//wait for all threads to finish
		}
	}

	//locate the shares in the appropriate places
	
	int count = 1;//Start counting from 1 because 0 is shares of R.
	for (int p = 0; p < numOfParties; p++)
		for (int i = 0; i < cyc->playerArray[p].playerBitAmount; i++)
		{
			cyc->playerArray[p].playerWires[i]->lambdaShare = shares[count];
			count++;
		}
	
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		cyc->regularGates[g]->output->lambdaShare = shares[count];
		count++;
	}
	
	for (int g = 0; g < cyc->numOfANDGates; g++)
		for (int i = 0; i < 4; i++)
			for (int p = 0; p < numOfParties; p++)
			{
				cyc->regularGates[g]->G[i][p] = shares[count];
				count++;
			}

	//free memory
	_aligned_free(allSecrets);
	_aligned_free(allCoefficients);
	_aligned_free(shares);
	delete[] threads;

	//free XOR gates - share of lambda of output wire is xor of shares of lambdas of input wires
	for (int g = 0; g < cyc->numOfXORGates; g++)
		cyc->specialGates[g]->output->lambdaShare = _mm_xor_si128(cyc->specialGates[g]->input1->lambdaShare, cyc->specialGates[g]->input2->lambdaShare);
}

#ifdef MULTTHREADNUM
void mulLambdasBGWwThreads(int tNum)
{
	int start = (tNum*cyc->numOfANDGates) / MULTTHREADNUM;
	int end = ((tNum + 1)*cyc->numOfANDGates) / MULTTHREADNUM;
	Gate* gate;
	for (int g = start; g < end; g++)
	{
		gate = cyc->regularGates[g];
		MULT(gate->input1->lambdaShare, gate->input2->lambdaShare, gate->mulLambdaShare);//multiply the shares of lambdas of input wires and saves result in mulLambdaShare.
	}
}
#endif

void mulLambdasBGW()
{
#ifndef MULTTHREADNUM
	Gate* gate;
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		gate = cyc->regularGates[g];
		MULT(gate->input1->lambdaShare, gate->input2->lambdaShare, gate->mulLambdaShare);//multiply the shares of lambdas of input wires and saves result in mulLambdaShare.
	}
#else
	thread *threads = new thread[MULTTHREADNUM];
	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i] = thread(mulLambdasBGWwThreads, i);

	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i].join();//wait for all threads to finish
#endif
}

#ifdef MULTTHREADNUM
void mulRBGWwThreads(int tNum, int totalThreads)
{

	int start = (tNum*cyc->numOfANDGates) / totalThreads;
	int end = ((tNum + 1)*cyc->numOfANDGates) / totalThreads;
	__m128i tmpShare0;//share of (mulLambdas+outLambda)
	__m128i tmpShare1;//share of mulLambdas+inLambda1+outLambda
	__m128i tmpShare2;//share of mulLambdas+inLambda2+outLambda
	__m128i tmpShare3;//share of mulLambdas+inLambda1+inLambda2+outLambda
	Gate* gate;
	int shift;
	for (int g = start; g < end; g++)
	{
		gate = cyc->regularGates[g];
		shift = gate->sh;
		tmpShare0 = ADD(gate->mulLambdaShare, gate->output->lambdaShare);
		tmpShare1 = ADD(tmpShare0, gate->input1->lambdaShare);
		tmpShare2 = ADD(tmpShare0, gate->input2->lambdaShare);
		tmpShare3 = ADD(ADD(tmpShare1, gate->input2->lambdaShare), ONE);

		for (int p = 0; p < numOfParties; p++)//Add share of R*tmpShare to the share of the PRG+S
		{
			gate->G[0 ^ shift][p] = ADD(gate->G[0 ^ shift][p], MUL(tmpShare0, RShare[p]));
			gate->G[1 ^ shift][p] = ADD(gate->G[1 ^ shift][p], MUL(tmpShare1, RShare[p]));
			gate->G[2 ^ shift][p] = ADD(gate->G[2 ^ shift][p], MUL(tmpShare2, RShare[p]));
			gate->G[3 ^ shift][p] = ADD(gate->G[3 ^ shift][p], MUL(tmpShare3, RShare[p]));
		}
	}
}
#endif

void mulRBGW()
{
#ifndef MULTTHREADNUM
	__m128i tmpShare0;//share of (mulLambdas+outLambda)
	__m128i tmpShare1;//share of mulLambdas+inLambda1+outLambda
	__m128i tmpShare2;//share of mulLambdas+inLambda2+outLambda
	__m128i tmpShare3;//share of mulLambdas+inLambda1+inLambda2+outLambda
	Gate* gate;
	int shift;
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		gate = cyc->regularGates[g];
		shift = gate->sh;
		tmpShare0 = ADD(gate->mulLambdaShare, gate->output->lambdaShare);
		tmpShare1 = ADD(tmpShare0, gate->input1->lambdaShare);
		tmpShare2 = ADD(tmpShare0, gate->input2->lambdaShare);
		tmpShare3 = ADD(ADD(tmpShare1, gate->input2->lambdaShare),ONE);

		for (int p = 0; p < numOfParties; p++)//Add share of R*tmpShare to the share of the PRG+S
		{
#ifndef PIPELINED_MULTIPLICATION
			gate->G[0 ^ shift][p] = ADD(gate->G[0 ^ shift][p], MUL(tmpShare0, RShare[p]));
			gate->G[1 ^ shift][p] = ADD(gate->G[1 ^ shift][p], MUL(tmpShare1, RShare[p]));
			gate->G[2 ^ shift][p] = ADD(gate->G[2 ^ shift][p], MUL(tmpShare2, RShare[p]));
			gate->G[3 ^ shift][p] = ADD(gate->G[3 ^ shift][p], MUL(tmpShare3, RShare[p]));
#else
			Add_Pointwise_4_Multiplication(
				&tmpShare0, RShare+p,
				&tmpShare1, RShare+p,
				&tmpShare2, RShare+p,
				&tmpShare3, RShare+p,
				&gate->G[0 ^ shift][p], &gate->G[1 ^ shift][p], &gate->G[2 ^ shift][p], &gate->G[3 ^ shift][p]);
#endif
		}
	}
#else
	int threadNum = min(MULTTHREADNUM *numOfParties,16);
	thread *threads = new thread[threadNum];
	for (int i = 0; i < threadNum; i++)
		threads[i] = thread(mulRBGWwThreads, i, threadNum);//add (XOR) the local shares

	for (int i = 0; i < threadNum; i++)
		threads[i].join();//wait for all threads to finish
	delete[] threads;
#endif
	//free memory
	_aligned_free(RShare);//shares of R not needed anymore
}

//Parallel sending
void newSendGateBGW(__m128i* toSend, int player)
{
	int totalSendSize = ((4 * cyc->numOfANDGates*numOfParties) + numOfOutputWires)*sizeof(__m128i);
	
	communicationSenders[player]->sendMsg(toSend, totalSendSize);
}
//Parallel sending
void newReceiveGateBGW(int player)
{
	int totalReceiveSize = ((4 * cyc->numOfANDGates*numOfParties) + numOfOutputWires)*sizeof(__m128i);
	playerSeeds[player] = static_cast<__m128i *>(_aligned_malloc(totalReceiveSize, 16));
	communicationReceivers[player]->reciveMsg(playerSeeds[player], totalReceiveSize);
}

//Sending with order
void exchangeGateBGW(__m128i* toSend, int player)
{
	int total = ((4 * cyc->numOfANDGates*numOfParties) + numOfOutputWires)*sizeof(__m128i);
	playerSeeds[player] = static_cast<__m128i *>(_aligned_malloc(total, 16));//assign space;

	if (player < partyNum)
	{
		communicationReceivers[player]->reciveMsg(playerSeeds[player], total);
		
		communicationSenders[player]->sendMsg(toSend, total);
	}
	else
	{
		communicationSenders[player]->sendMsg(toSend, total);

		communicationReceivers[player]->reciveMsg(playerSeeds[player], total);
	}
}

#ifdef MULTTHREADNUM
void threadReconstruction(int tNum, int totalThreads, int reconDeg, int exchangingParties,int totalSharesSize, __m128i* toSend)
{
	__m128i* shares0 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));
	__m128i* shares1 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));
	__m128i* shares2 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));
	__m128i* shares3 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));
	int start= (tNum* cyc->numOfANDGates) / totalThreads;
	int end= ((tNum+1)* cyc->numOfANDGates) / totalThreads;
	for (int g = start; g <end; g++)
	{
		for (int p = 0; p < numOfParties; p++)
		{
			for (int party = 0; party < exchangingParties; party++)//numOfParties////reconDeg+1
			{
				if (party != partyNum)
				{
					shares0[party] = playerSeeds[party][4 * g* numOfParties + p];
					shares1[party] = playerSeeds[party][(4 * g + 1)*numOfParties + p];
					shares2[party] = playerSeeds[party][(4 * g + 2)*numOfParties + p];
					shares3[party] = playerSeeds[party][(4 * g + 3)*numOfParties + p];
				}
				else
				{
					shares0[party] = toSend[4 * g* numOfParties + p];
					shares1[party] = toSend[(4 * g + 1)*numOfParties + p];
					shares2[party] = toSend[(4 * g + 2)*numOfParties + p];
					shares3[party] = toSend[(4 * g + 3)*numOfParties + p];
				}
			}
			cyc->regularGates[g]->G[0][p] = reconstruct(shares0, reconDeg);
			cyc->regularGates[g]->G[1][p] = reconstruct(shares1, reconDeg);
			cyc->regularGates[g]->G[2][p] = reconstruct(shares2, reconDeg);
			cyc->regularGates[g]->G[3][p] = reconstruct(shares3, reconDeg);
		}
	}
}
#endif

void exchangeGatesBGW()
{
	
	int reconDeg = degBig; 
	int exchangingParties = reconDeg+1;
	if (majType == 3) reconDeg = degSmall;

	exchangingParties = numOfParties;

	if (partyNum >= exchangingParties) return;
	
	int totalSendSize = ((4 * cyc->numOfANDGates*numOfParties)+numOfOutputWires)*sizeof(__m128i);
	__m128i* toSend;

	toSend = bigMemoryAllocation;

	for (int o = 0; o < numOfOutputWires; o++)
	{
		toSend[4 * cyc->numOfANDGates *numOfParties + o] = cyc->outputWires.playerWires[o]->lambdaShare;
	}

	//Now send the data and receive from the other players
	
	playerSeeds = new __m128i*[exchangingParties];
	thread *threads = new thread[2 * (exchangingParties)];
	for (int i = 0; i < exchangingParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(newSendGateBGW, toSend, i);//send inputs to player i
			threads[i * 2 + 1] = thread(newReceiveGateBGW, i);//receive inputs from player i
		}
	}
	for (int i = 0; i < 2*(exchangingParties); i++)
	{
		if (i != 2 * partyNum && i != (2 * partyNum + 1))
		{
			threads[i].join();//wait for all threads to finish
		}
	}

	int totalSharesSize = (exchangingParties)*sizeof(__m128i);
#ifndef MULTTHREADNUM
	__m128i* shares0 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));
	__m128i* shares1 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));
	__m128i* shares2 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));
	__m128i* shares3 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));

	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		for (int p = 0; p < numOfParties; p++)
		{
			for (int party = 0; party < exchangingParties; party++)
			{
				if (party != partyNum)
				{
					shares0[party] = playerSeeds[party][4 * g* numOfParties + p];
					shares1[party] = playerSeeds[party][(4 * g + 1)*numOfParties + p];
					shares2[party] = playerSeeds[party][(4 * g + 2)*numOfParties + p];
					shares3[party] = playerSeeds[party][(4 * g + 3)*numOfParties + p];
				}
				else
				{
					shares0[party] = toSend[4 * g* numOfParties + p];
					shares1[party] = toSend[(4 * g + 1)*numOfParties + p];
					shares2[party] = toSend[(4 * g + 2)*numOfParties + p];
					shares3[party] = toSend[(4 * g + 3)*numOfParties + p];
				}
			}
#ifndef PIPELINED_MULTIPLICATION
			cyc->regularGates[g]->G[0][p] = reconstruct(shares0, reconDeg);
			cyc->regularGates[g]->G[1][p] = reconstruct(shares1, reconDeg);
			cyc->regularGates[g]->G[2][p] = reconstruct(shares2, reconDeg);
			cyc->regularGates[g]->G[3][p] = reconstruct(shares3, reconDeg);
#else
			gfDotProductPiped(shares0, baseRecon, reconDeg+1, &cyc->regularGates[g]->G[0][p]);
			gfDotProductPiped(shares1, baseRecon, reconDeg+1, &cyc->regularGates[g]->G[1][p]);
			gfDotProductPiped(shares2, baseRecon, reconDeg+1, &cyc->regularGates[g]->G[2][p]);
			gfDotProductPiped(shares3, baseRecon, reconDeg+1, &cyc->regularGates[g]->G[3][p]);
#endif
		}
	}

	//Free memory
	_aligned_free(shares1);
	_aligned_free(shares2);
	_aligned_free(shares3);
#else
	delete[] threads;
	int threadNum = min(MULTTHREADNUM *numOfParties,16);
	threads = new thread[threadNum];
	for (int i = 0; i < threadNum; i++)
		threads[i] = thread(threadReconstruction, i, threadNum, reconDeg, exchangingParties, totalSharesSize, toSend);
	
	for (int i = 0; i < threadNum; i++)
		threads[i].join();//wait for all threads to finish
	__m128i* shares0 = static_cast<__m128i *>(_aligned_malloc(totalSharesSize, 16));
#endif
	
	int startIndex = 4 * cyc->numOfANDGates *numOfParties;
	for (int o = 0; o < numOfOutputWires; o++)
	{
		for (int party = 0; party < exchangingParties; party++)
		{
			if (party != partyNum)
			{
				shares0[party] = playerSeeds[party][ startIndex+ o];
			}
			else
			{
				shares0[party] = toSend[startIndex + o];
			}
		}
#ifndef PIPELINED_MULTIPLICATION
		__m128i recon=reconstruct(shares0, reconDeg);
#else
		__m128i recon;
		gfDotProductPiped(shares0, baseRecon, reconDeg+1, &recon);
#endif
		if (EQ(recon, ONE))//check value of reconstruction
		{
			cyc->outputWires.playerWires[o]->lambda = true;
		}
		else
		{
			if (EQ(recon, ZERO))
			{
				cyc->outputWires.playerWires[o]->lambda = false;
			}
			else
				cout << "ERROR! (are you using BGW2 with 3 parties?)" << endl;
		}
	}

	//Free memory
	_aligned_free(shares0);
	delete[] threads;
	for (int p = 0; p < exchangingParties; p++)
		if (p != partyNum)
			_aligned_free(playerSeeds[p]);
	delete[] playerSeeds;
}

void generateReductionPolynomials()
{
	allSecrets = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	int totalNumberOfCoef = totalSharesNumber*degSmall;//here all polynomials are of small degree
	allCoefficients = static_cast<__m128i *>(_aligned_malloc(totalNumberOfCoef*sizeof(__m128i), 16));
	
	for (int i = 0; i < totalNumberOfCoef; i++)
		allCoefficients[i] = RAND;


	//large vectors for polynomials
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		if (partyNum < degBig + 1)
			MULT(cyc->regularGates[g]->mulLambdaShare, baseReduc[partyNum], allSecrets[g]);
		else
		{
			allSecrets[g] = ZERO;
		}
	}
}



//DUPLICATE CODE... (but clearer this way)
void degReduceSend(int party)
{
	__m128i* sendShares = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	//compute shares here

#ifndef MULTTHREADNUM
	computeShares(party, sendShares);
#else
	thread *threads = new thread[MULTTHREADNUM];
	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i] = thread(computeShareswThreads, party, sendShares, i);//add (XOR) the local shares


	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i].join();//wait for all threads to finish
#endif

	//send shares
	communicationSenders[party]->sendMsg(sendShares, totalSharesNumber*sizeof(__m128i));
	_aligned_free(sendShares);
}

void degReduceReceive(int party)
{
	__m128i* receivedShares = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	//receive shares
	communicationReceivers[party]->reciveMsg(receivedShares, totalSharesNumber*sizeof(__m128i));
	//add to local shares/shares from other parties
	mtxVal.lock();
		XORvectors(shares, receivedShares, shares, totalSharesNumber);
	mtxVal.unlock();
	_aligned_free(receivedShares);
}

void degReduceLocal()
{
	__m128i* localShares = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	//compute local shares here
#ifndef MULTTHREADNUM
	computeShares(partyNum, localShares);
#else
	thread *threads = new thread[MULTTHREADNUM];
	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i] = thread(computeShareswThreads, partyNum, localShares, i);//add (XOR) the local shares


	for (int i = 0; i < MULTTHREADNUM; i++)
		threads[i].join();//wait for all threads to finish
#endif
	//add to shares from other parties	
	mtxVal.lock();
		XORvectors(shares, localShares, shares, totalSharesNumber);
	mtxVal.unlock();
	_aligned_free(localShares);
}

//Reduce the degree of the shares of multiplication of lambdas, using a BGW round - not need if more than 2/3 majority
void reduceDegBGW()
{
	totalSharesNumber = cyc->numOfANDGates ;//number of mulLambda shares

	numOfSmallDegPolynomials=totalSharesNumber;

	shares = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	memset(shares, 0, totalSharesNumber*sizeof(*shares)); // for heap-allocated arrays, where N is the number of elements

	generateReductionPolynomials();

	//Now send the data and receive from the other players
	thread *threads = new thread[numOfParties * 2];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(degReduceSend, i);//send inputs to player i
			threads[i * 2 + 1] = thread(degReduceReceive, i);//receive inputs from player i
		}
		else
			threads[i * 2] = thread(degReduceLocal);
	}
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != (2 * partyNum + 1))
		{
			threads[i].join();//wait for all threads to finish
		}
	}
	//store shares in correct location
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		cyc->regularGates[g]->mulLambdaShare = shares[g];
	}
	
	//free memory

	_aligned_free(allSecrets);
	_aligned_free(allCoefficients);
	_aligned_free(shares);
	delete[] threads;

}
#ifdef MULTTHREADNUM
void mulRBGWAssistwThreads(int tNum, int totalThreads, __m128i* tmpReducShare)
{
	int startIndex;

	__m128i tmpShare0;//share of (mulLambdas+outLambda)
	__m128i tmpShare1;//share of mulLambdas+inLambda1+outLambda
	__m128i tmpShare2;//share of mulLambdas+inLambda2+outLambda
	__m128i tmpShare3;//share of mulLambdas+inLambda1+inLambda2+outLambda
	Gate* gate;

	int start = (tNum* cyc->numOfANDGates) / totalThreads;
	int end = ((tNum + 1)* cyc->numOfANDGates) / totalThreads;
	//first step - multiply shares of R with corresponding lambda shares
	for (int g = start; g < end; g++)
	{
		gate = cyc->regularGates[g];
		tmpShare0 = ADD(gate->mulLambdaShare, gate->output->lambdaShare);
		tmpShare1 = ADD(tmpShare0, gate->input1->lambdaShare);
		tmpShare2 = ADD(tmpShare0, gate->input2->lambdaShare);
		//CHECK THIS
		tmpShare3 = ADD(ADD(tmpShare1, gate->input2->lambdaShare), ONE);

		for (int p = 0; p < numOfParties; p++)//Compute share of R*tmpShare
		{
			startIndex = 4 * (g*numOfParties + p);
			MULT(tmpShare0, tmpReducShare[p], allSecrets[startIndex]);
			MULT(tmpShare1, tmpReducShare[p], allSecrets[startIndex + 1]);
			MULT(tmpShare2, tmpReducShare[p], allSecrets[startIndex + 2]);
			MULT(tmpShare3, tmpReducShare[p], allSecrets[startIndex + 3]);
		}
	}
}
#endif

void mulRBGWwithReduction()
{
	int startIndex;

	totalSharesNumber = 4 * numOfParties * cyc->numOfANDGates;//number of mulLambda shares
	shares = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	memset(shares, 0, totalSharesNumber*sizeof(*shares)); // for heap-allocated arrays, where N is the number of elements
	

	numOfSmallDegPolynomials = totalSharesNumber;//here all polynomials are of small degree
	allSecrets = static_cast<__m128i *>(_aligned_malloc(totalSharesNumber*sizeof(__m128i), 16));
	int totalNumberOfCoef = totalSharesNumber*degSmall;//here all polynomials are of small degree
	allCoefficients = static_cast<__m128i *>(_aligned_malloc(totalNumberOfCoef*sizeof(__m128i), 16));
	//Here supposed to choose random coefficients for polynomials using AES-NI
	for (int i = 0; i < totalNumberOfCoef; i++)
		allCoefficients[i] = RAND;


	__m128i* tmpReducShare= static_cast<__m128i *>(_aligned_malloc(numOfParties*sizeof(__m128i), 16));//Used to reduce multiplications by lagrange coefficient for reduction step.
	for (int p = 0; p < numOfParties; p++)//Compute share of R*tmpShare
		MULT(RShare[p], baseReduc[partyNum], tmpReducShare[p]);

	Gate* gate;
#ifndef MULTTHREADNUM

	__m128i tmpShare0;//share of (mulLambdas+outLambda)
	__m128i tmpShare1;//share of mulLambdas+inLambda1+outLambda
	__m128i tmpShare2;//share of mulLambdas+inLambda2+outLambda
	__m128i tmpShare3;//share of mulLambdas+inLambda1+inLambda2+outLambda
	
	//first step - multiply shares of R with corresponding "lambdas"
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		gate = cyc->regularGates[g];
		tmpShare0 = ADD(gate->mulLambdaShare, gate->output->lambdaShare);
		tmpShare1 = ADD(tmpShare0, gate->input1->lambdaShare);
		tmpShare2 = ADD(tmpShare0, gate->input2->lambdaShare);
		//CHECK THIS
		tmpShare3 = ADD(ADD(tmpShare1, gate->input2->lambdaShare), ONE);

		for (int p = 0; p < numOfParties; p++)//Compute share of R*tmpShare
		{
			startIndex = 4 * (g*numOfParties + p);
			MULT(tmpShare0, tmpReducShare[p], allSecrets[startIndex]);
			MULT(tmpShare1, tmpReducShare[p], allSecrets[startIndex + 1]);
			MULT(tmpShare2, tmpReducShare[p], allSecrets[startIndex + 2]);
			MULT(tmpShare3, tmpReducShare[p], allSecrets[startIndex + 3]);
		}
	}
#else
	int threadNum = min(MULTTHREADNUM *numOfParties,16);
	thread *threadss = new thread[threadNum];
	for (int i = 0; i < threadNum; i++)
		threadss[i] = thread(mulRBGWAssistwThreads, i, threadNum, tmpReducShare);//add (XOR) the local shares

	for (int i = 0; i < threadNum; i++)
		threadss[i].join();//wait for all threads to finish
	delete[] threadss;
#endif

	//second step - reduce degree
	thread *threads = new thread[numOfParties * 2];
	for (int i = 0; i < numOfParties; i++)
	{
		if (i != partyNum)
		{
			threads[i * 2] = thread(degReduceSend, i);//send inputs to player i
			threads[i * 2 + 1] = thread(degReduceReceive, i);//receive inputs from player i
		}
		else
			threads[i * 2] = thread(degReduceLocal);
	}
	for (int i = 0; i < 2 * numOfParties; i++)
	{
		if (i != (2 * partyNum + 1))
		{
			threads[i].join();//wait for all threads to finish
		}
	}

	//third step - add shares to the share of PRG+S
	int shift;
	for (int g = 0; g < cyc->numOfANDGates; g++)
	{
		gate = cyc->regularGates[g];
		shift = gate->sh;
		for (int p = 0; p < numOfParties; p++)//Add share of R*tmpShare to the share of the PRG+S
		{
			startIndex = 4 * (g*numOfParties + p);
			gate->G[0 ^ shift][p] = ADD(gate->G[0 ^ shift][p], shares[startIndex]);
			gate->G[1 ^ shift][p] = ADD(gate->G[1 ^ shift][p], shares[startIndex +1]);
			gate->G[2 ^ shift][p] = ADD(gate->G[2 ^ shift][p], shares[startIndex +2]);
			gate->G[3 ^ shift][p] = ADD(gate->G[3 ^ shift][p], shares[startIndex +3]);
		}
	}

	//free memory 
	_aligned_free(allSecrets);
	_aligned_free(allCoefficients);
	_aligned_free(shares);
	delete[] threads;
	_aligned_free(RShare);
}



void deletePartialBGW()
{

}

void deleteAllBGW()
{
	_aligned_free(baseReduc);
	_aligned_free(powers);
}