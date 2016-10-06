/*
 * ottmain.h
 *
 *      Author: froike (Roi Inbar) and Aner Ben-Efraim
 * 	Encapsulation of OTExtension code by Thomas Schneider and Michael Zhonner
 * 
 */


#ifndef _MPC_H_
#define _MPC_H_

#include "typedefs.h"
#include "socket.h"
//#include "../ot/naor-pinkas.h"
//#include "../ot/asharov-lindell.h"
//#include "../ot/ot-extension.h"
#include "cbitvector.h"
//#include "../ot/xormasking.h"


//NEW
#include "BMR.h"

#include <vector>
#include <time.h>

#include <limits.h>
#include <iomanip>
#include <string>
#include <emmintrin.h>
#include <random>
#include <stdint.h>
//#include <thread>

using namespace std;

class OTclass{

private:
	USHORT		m_nPort;// = 7766;
	const char* m_nAddr;// = "localhost";

	// Network Communication
//	vector<CSocket> m_vSockets;
	int m_nPID; // thread id
    const char* m_nSeed;
//	int m_nSecParam;
//	bool m_bUseECC;
//	int m_nBitLength;
//	int m_nMod;
//	MaskingFunction* m_fMaskFct;

	// Naor-Pinkas OT
//	BaseOT* bot;
//	OTExtensionSender *sender;
//	OTExtensionReceiver *receiver;
//	CBitVector U;
//	BYTE *vKeySeeds;
//	BYTE *vKeySeedMtx;

//	int m_nNumOTThreads;
//	int numOfSockets;

	// SHA PRG
//	BYTE			m_aSeed[SHA1_BYTES];
//	int				m_nCounter;
//	double			rndgentime;

	/////
	BYTE version;
	CBitVector delta;
	/////


//	BOOL Init();
//	BOOL Cleanup();
//	BOOL Connect(int initialSocketNum, int port);
//	BOOL Listen(int initialSocketNum, int port);

//	OTExtensionSender* InitOTSender(const char* address, int port, int initialSocketNum);
//	OTExtensionReceiver* InitOTReceiver(const char* address, int port, int initialSocketNum);

	//BOOL PrecomputeNaorPinkasSender();
	//BOOL PrecomputeNaorPinkasReceiver();
	//BOOL ObliviouslyReceive(CBitVector& choices, CBitVector& ret, int numOTs, int bitlength, BYTE version);
	//BOOL ObliviouslySend(CBitVector& X1, CBitVector& X2, int numOTs, int bitlength, BYTE version, CBitVector& delta);

public:
	//constructors
	OTclass();
	OTclass(char *address, int port, int mode);
	OTclass(char *address, int port, int mode,const char* seed);
	~OTclass();//destructor
	//initialization of OT
	BOOL Initialize();

	//Oblivious communication
	BOOL OTsend(int bitlength, int numOTs, CBitVector& X1, CBitVector& X2);
	BOOL OTsendC(int bitlength, int numOTs, CBitVector& X1, CBitVector& X2, CBitVector delta);
	BOOL OTreceive(int bitlength, int numOTs, CBitVector& choices, CBitVector& response);//CBitVector
	BOOL OTreceiveC(int bitlength, int numOTs, CBitVector& choices, CBitVector& response);//CBitVector
	
};


#endif //_MPC_H_
