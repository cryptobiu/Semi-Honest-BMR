/*
 * ottmain.h
 *
 *      Author: froike (Roi Inbar) and Aner Ben-Efraim
 * 	Encapsulation of OTExtension code by Thomas Schneider and Michael Zhonner
 * 
 */


#include "ottmain.h"

using namespace std;


//Default seed. Initialize the seed in the constructor
//static const char* m_nSeed = "437398417012387813714564100";

//MaskingFunction* m_fMaskFct;

OTclass::OTclass()
{
}

OTclass::OTclass(char* address, int port, int mode)
{
/*	cout << "Playing as role: " << mode << endl;
	//Choose OT extension version: G_OT, C_OT or R_OT
	version = G_OT;//This will be the default, unless stated specifically otherwise in the function

	//Use elliptic curve cryptography in the base-OTs
	m_bUseECC = true;
	//The security parameter (163,233,283 for ECC or 1024, 2048, 3072 for FFC)
	m_nSecParam = 283;
	
	m_nNumOTThreads = OTTHREADNUM;
	m_vSockets.resize(m_nNumOTThreads);//m_nNumOTThreads
	
	m_nAddr = address;
	m_nPort = port;
	m_nPID = mode;*/
}
OTclass::OTclass(char * address, int port, int mode, const char * seed)
{ /*
	cout << "Playing as role: " << mode << endl;
	//Choose OT extension version: G_OT, C_OT or R_OT
	version = G_OT;//This will be the default, unless stated specifically otherwise in the function

	//Use elliptic curve cryptography in the base-OTs
	m_bUseECC = true;
	//The security parameter (163,233,283 for ECC or 1024, 2048, 3072 for FFC)
	m_nSecParam = 283;
	
	m_nNumOTThreads = OTTHREADNUM;
	m_vSockets.resize(m_nNumOTThreads);//m_nNumOTThreads
	*/
	m_nAddr = address;
	m_nPort = port;
	m_nPID = mode;
	m_nSeed = seed;
}
OTclass::~OTclass()
{
	/*OTclass::Cleanup();*/
}
/*
BOOL OTclass::Init()
{
	SHA_CTX sha;
	OTEXT_HASH_INIT(&sha);
	OTEXT_HASH_UPDATE(&sha, (BYTE*)&m_nPID, sizeof(m_nPID));
	OTEXT_HASH_UPDATE(&sha, (BYTE*)m_nSeed, sizeof(m_nSeed));
	OTEXT_HASH_FINAL(&sha, m_aSeed);

	m_nCounter = 0;

	bot = new NaorPinkas(m_nSecParam, m_aSeed, m_bUseECC);

	return TRUE;
}*/

//This doesn't seem to close the sockets properly for some reason.
/*BOOL OTclass::Cleanup()
{
    for (int i = 0; i < m_nNumOTThreads; i++)
	{
		m_vSockets[i].Close();
	}
	return true;
}

BOOL OTclass::Connect(int initialSocketNum, int port)
{
	BOOL bFail = FALSE;
	LONG lTO = CONNECT_TIMEO_MILISEC;

#ifdef PRINT_STEPS
	cout << "Connecting to party " << !m_nPID << ": " << m_nAddr << ", " << m_nPort << endl;
#endif
	for (int k = m_nNumOTThreads - 1; k >= 0; k--)
	{
		int socketNum = initialSocketNum + k;
		for (int i = 0; i<RETRY_CONNECT; i++)
		{
			if (!m_vSockets[socketNum].Socket())
			{
				printf("Socket failure: ");
				goto connect_failure;
			}

			if (m_vSockets[socketNum].Connect(m_nAddr, port, lTO))
			{
				// send pid when connected
				m_vSockets[socketNum].Send(&socketNum, sizeof(int));
#ifdef PRINT_STEPS
				cout << " (" << !m_nPID << ") (" << socketNum << ") connected" << endl;
#endif
				if (k == 0)
				{
					//cout << "connected" << endl;
					return TRUE;
				}
				else
				{
					break;
				}
				SleepMiliSec(10);
				m_vSockets[socketNum].Close();/////
			}
			SleepMiliSec(20);
			if (i + 1 == RETRY_CONNECT)
				goto server_not_available;
		}
	}
server_not_available:
	printf("Server not available: ");
connect_failure:
	cout << " (" << !m_nPID << ") connection failed" << endl;
	return FALSE;
}


BOOL OTclass::Listen(int initialSocketNum, int port)
{
#ifdef PRINT_STEPS
	cout << "Listening: " << m_nAddr << ":" << m_nPort << ", with size: " << m_nNumOTThreads << endl;
#endif
	if (!m_vSockets[initialSocketNum].Socket())
	{
		goto listen_failure;
	}
	if (!m_vSockets[initialSocketNum].Bind(port))/////m_nPort,m_nAddr
		goto listen_failure;
	if (!m_vSockets[initialSocketNum].Listen())
		goto listen_failure;

	for (int i = 0; i<m_nNumOTThreads; i++)//twice the actual number, due to double sockets for OT
	{
		CSocket sock;
		
		if (!m_vSockets[initialSocketNum].Accept(sock))
		{
			cerr << "Error in accept" << endl;
			goto listen_failure;
		}

		UINT threadID;
		sock.Receive(&threadID, sizeof(int));

		if (threadID >= m_nNumOTThreads)
		{
			cout << "thread id:" << threadID << endl;
		}

#ifndef BATCH
		cout << " (" << m_nPID << ") (" << threadID << ") connection accepted" << endl;
#endif
		// locate the socket appropriately
		m_vSockets[threadID].AttachFrom(sock);
		sock.Detach();
	}

#ifdef PRINT_STEPS
	cout << "Listening finished" << endl;
#endif
	return TRUE;

listen_failure:
	cout << "Listen failed" << endl;
	return FALSE;
}

*/
void OTclass::InitOTSender()
{
    m_spMe = SocketPartyData(IpAdress::from_string("127.0.0.1"), COMPARE_FUNCTIONALITY_SERVER_PORT);
    m_spOther = SocketPartyData(IpAdress::from_string(m_nAddr), COMPARE_FUNCTIONALITY_CLIENT_PORT);
    m_cpChannel = make_shared<CommPartyTCPSynced>(m_ioService, m_spMe, m_spOther);

    m_cf_channel->join(500, 5000);
    m_otSender = new OTExtensionBristolSender(OT_PORT, true, m_cpChannel);
}

void OTclass::InitOTReceiver()
{/*
    m_spMe = SocketPartyData(IpAdress::from_string("127.0.0.1"), COMPARE_FUNCTIONALITY_SERVER_PORT);
    m_spOther = SocketPartyData(IpAdress::from_string(m_nAddr), COMPARE_FUNCTIONALITY_CLIENT_PORT);
    m_cpChannel = make_shared<CommPartyTCPSynced>(m_ioService, m_spMe, m_spOther);

    m_cf_channel->join(500, 5000);
    m_otReceiver = new OTExtensionBristolReciever(Globals::m_serverIP, OT_PORT, true, m_cf_channel);*/
}
/*
BOOL OTclass::PrecomputeNaorPinkasSender()
{

	int nSndVals = 2;
	BYTE* pBuf = new BYTE[NUM_EXECS_NAOR_PINKAS * SHA1_BYTES];
	int log_nVals = (int)ceil(log((double)nSndVals) / log(2.0)), cnt = 0;
	
	U.Create(NUM_EXECS_NAOR_PINKAS*log_nVals, m_aSeed, cnt);

	bot->Receiver(nSndVals, NUM_EXECS_NAOR_PINKAS, U, m_vSockets[0], pBuf);

	BYTE* pBufIdx = pBuf;
	for (int i = 0; i<NUM_EXECS_NAOR_PINKAS; i++) //80 HF calls for the Naor Pinkas protocol
	{
		memcpy(vKeySeeds + i * AES_KEY_BYTES, pBufIdx, AES_KEY_BYTES);
		pBufIdx += SHA1_BYTES;
	}
	delete[] pBuf;

	return true;
}

BOOL OTclass::PrecomputeNaorPinkasReceiver()
{
	int nSndVals = 2;
	
	// Execute NP receiver routine and obtain the key 
	BYTE* pBuf = new BYTE[SHA1_BYTES * NUM_EXECS_NAOR_PINKAS * nSndVals];

	// N-P sender: send: C0 (=g^r), C1, C2, C3 
	bot->Sender(nSndVals, NUM_EXECS_NAOR_PINKAS, m_vSockets[0], pBuf);
	
	BYTE* pBufIdx = pBuf;
	for (int i = 0; i<NUM_EXECS_NAOR_PINKAS * nSndVals; i++)
	{
		memcpy(vKeySeedMtx + i * AES_KEY_BYTES, pBufIdx, AES_KEY_BYTES);
		pBufIdx += SHA1_BYTES;
	}

	delete[] pBuf;

	return true;
}

//Encapsulating Oblivious send
BOOL OTclass::ObliviouslySend(CBitVector& X1, CBitVector& X2, int numOTs, int bitlength, BYTE version, CBitVector& delta)
{
	bool success = FALSE;
	int nSndVals = 2; //Perform 1-out-of-2 OT


	m_fMaskFct = new XORMasking(bitlength);

	success = sender->send(numOTs, bitlength, X1, X2, delta, version, m_nNumOTThreads, m_fMaskFct);
	delete(m_fMaskFct);

	return success;
}


//Encapsulating Oblivious receive
BOOL OTclass::ObliviouslyReceive(CBitVector& choices, CBitVector& ret, int numOTs, int bitlength, BYTE version)
{
	bool success = FALSE;


	m_fMaskFct = new XORMasking(bitlength);

	success = receiver->receive(numOTs, bitlength, choices, ret, version, m_nNumOTThreads, m_fMaskFct);
	delete(m_fMaskFct);

	return success;
}
*/

BOOL OTclass::Initialize()
{

/*
	if (m_nPID == 0) //Play as OT sender
	{
		InitOTSender(m_nAddr, m_nPort, 0);
	}
	else
	{
		InitOTReceiver(m_nAddr, m_nPort, 0);
	}
*/
	return true;
}

BOOL OTclass::OTsend(int bitlength, int numOTs, CBitVector& X1, CBitVector& X2)
{
/*	delta.Create(numOTs, bitlength, m_aSeed, m_nCounter);*/
	return true;//OTclass::ObliviouslySend(X1, X2, numOTs, bitlength, version, delta);
}

BOOL OTclass::OTsendC(int bitlength, int numOTs, CBitVector& X1, CBitVector& X2,CBitVector delta)
{
	return true;//OTclass::ObliviouslySend(X1, X2, numOTs, bitlength, C_OT, delta);
}

BOOL OTclass::OTreceive(int bitlength, int numOTs, CBitVector& choices, CBitVector& response)
{
/*	response.Create(numOTs, bitlength);
	OTclass::ObliviouslyReceive(choices, response, numOTs, bitlength, version);*/
	return TRUE;
}
//Receiving for correlated OT
BOOL OTclass::OTreceiveC(int bitlength, int numOTs, CBitVector& choices, CBitVector& response)
{
/*	response.Create(numOTs, bitlength);
	OTclass::ObliviouslyReceive(choices, response, numOTs, bitlength, C_OT);*/
	return TRUE;
}
