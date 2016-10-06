#include "ottmain.h"
#include <thread>
#include "secCompMultiParty.h"
#include "BMR.h"
#include "BMR_BGW.h"
#include <math.h>

//LOCK
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

/*main program. Should receive as arguments:
1. A file specifying the circuit, number of players, input wires, output wires
2. A file specifying the connection details
3. A file specifying the current player, inputs
4. Player number
5. Random seed
*/
int main(int argc, char** argv)
{
//honest majority type, player number
int hm,p;
//circuit
Circuit* c;
//the output
bool *out;


{
	//Start initialization phase
	
	p = atoi(argv[1]);//Use this to get player number from arguments (use for local run).
	
	if (p < 0)
	{
		p = getPartyNum(argv[4]);//Use this to get player number by matching IP address
		//argv[3][strlen(argv[3]) - 1] = ('0'+p);//change chosen input file to match
	}

	cout << "This is player:" << p << endl;


	
#ifdef PRINT_STEPS
	cout << "Reading circuit from file" << endl;
#endif

	//1 - Read the circuit from the file
	c = loadCircuit(argv[2]);

	cout << "Number parties: " << c->amountOfPlayers << endl;
	cout << "Number of AND gates: " << c->numOfANDGates << endl;
	cout << "Number of XOR gates: " << c->numOfXORGates << endl;
	//cout << "Number of iterations: " << TEST_ROUNDS << endl;


#ifdef PRINT_STEPS
	cout << "Circuit read"<<endl;
	cout << "Initializing Communication" << endl;
#endif

	//2 - initialize the communication
	initializeCommunication(argv[4], c, p);

#ifdef PRINT_STEPS
	cout << "Communication Initialized" << endl;
	//cout << endl << endl << double(clock() - startTime) / (double)CLOCKS_PER_SEC << " seconds." << endl << endl;
#endif

#ifdef PRINT_STEPS
	cout << endl<<"User given key "<< argv[5] << endl;
#endif

	initializeRandomness(argv[5], c->amountOfPlayers); 
	
	//check adversary model
	hm = 0;
	if (argc > 6) hm = atoi(argv[6]);
	
	VERSION(hm);



	if (hm == 0) //No honest majority - initialize OT-Extension based algorithm.
	{
#ifdef PRINT_STEPS
		cout << "Initializing OTs" << endl;
#endif

		//3.2 - initialize the OTExtension (Base OTs, etc.)
		initializeOTs();

#ifdef PRINT_STEPS
		cout << "OTs Initialized" << endl;
#endif
#ifdef BMR_DEBUG
		system("PAUSE");
#endif
	}
	else//Honest majoritym - initialize BGW based algorithm
	{
#ifdef PRINT_STEPS
		cout << "Initializing BGW coef" << endl;
#endif
		//3.2 - precompute powers and degree reduction coefficients used in BGW protocol.
		InitializeBGW(hm);//Honest majority - we will use BGW
#ifdef PRINT_STEPS
		cout << "BGW coef initialized" << endl;
#endif
	}
	
}
//End setup phase


//BEGIN TESTING FOR LOOP
//for (int repetition = 0; repetition < 11; repetition++)
{

	synchronize();
	//Start offline phase


#ifdef PRINT_STEPS
	cout << "Choosing Seeds and lambdas" << endl;
#endif

	//4 - load random R, random seeds, random lambdas and compute lambdas/seed of XOR gates
	newLoadRandom();

#ifdef PRINT_STEPS
	cout << "Seeds and lambdas chosen" << endl;
	
	//cout << "Ready to Roll" << endl;

	cout << "Computing AES for gates" << endl;
#endif

	//here the regular gates go
	//5 - compute AES of Ag,Bg,Cg,Dg for multiplication gates
	computeGates();//Me & Roi - Requires AES...


#ifdef PRINT_STEPS
	cout << "AES for gates computed" << endl;
#endif

		//////NEW
	if (hm == 0)//Run OT based protocol
	{
#ifdef PRINT_STEPS
	cout << "Multiplying lambdas (OT)" << endl;
#endif

	//6 - multiply the lambdas for multiplication gates - uses OT
	mulLambdasC();

#ifdef PRINT_STEPS
	cout << "Multiplied lambdas" << endl;

	cout << "Multiplying R (OT)" << endl;
#endif

	mulRC();


#ifdef PRINT_STEPS
	cout << "Multiplied R" << endl;

	cout << "Exchanging gates" << endl;
#endif
	//8 - send and receive gates
	exchangeGates();

#ifdef PRINT_STEPS
	cout << "Exchanged gates" << endl;
#endif
	  
	}
	else//Run BGW based protocol
	{
#ifdef PRINT_STEPS
	cout << "Computing and distributing shares" << endl;
#endif
	//5.2 Create secret-sharings of correct degrees and send/receive
	secretShare();//secret-share lambdas, R, and PRGs - first communication round

#ifdef PRINT_STEPS
	cout << "Shares computed and distributed" << endl;

	cout << "Multiplying lambdas" << endl;
#endif

	//6 - multiply the lambdas for multiplication gates - local computation
	mulLambdasBGW();//local computation

#ifdef PRINT_STEPS
	cout << "Multiplied lambdas" << endl;
#endif
	if (hm == 1 || hm==3)//BGW 3 or BGW 4 - need to do degree reduction
	{
#ifdef PRINT_STEPS
	cout << "Reducing Degree" << endl;
#endif

	//6.2 - reduce degree of share polynomials (not needed if >2/3 honest)
	reduceDegBGW();
				
#ifdef PRINT_STEPS
	cout << "Reduced Degree" << endl;
#endif
	}

#ifdef PRINT_STEPS
	cout << "Multiplying R" << endl;
#endif
	//7 - according to lambdas multiplication, multiply R for multiplication gates. Add results to Ag,Bg,Cg,Dg respectively.
	if (hm != 3)
		mulRBGW();//local computation
	else
		mulRBGWwithReduction();//run BGW4 with an extra reduction step

#ifdef PRINT_STEPS
	cout << "Multiplied R" << endl;

	cout << "Exchanging gates" << endl;
#endif

	//8 - send and receive gates
	exchangeGatesBGW();//communication round, also exchanges the output lambdas, and reconstructs the gates
	}
	
	//end offline phase

	{//Start of online phase
	synchronize();

	//2 - load inputs from file
	loadInputs(argv[3], cyc, p);

#ifdef PRINT_STEPS
	cout << "Revealing masked inputs" << endl;
#endif
	//9 - reveal Inputs (XORed with lambda)
	exchangeInputs();

#ifdef PRINT_STEPS
	cout << "Revealed masked inputs" << endl;

	cout << "Revealing corresponding seeds" << endl;
#endif
	//10 - reveal corresponding seeds
	exchangeSeeds();
#ifdef PRINT_STEPS
	cout << "Revealed corresponding seeds" << endl;
#endif

#ifdef PRINT_STEPS
	cout << "Computing values of garbled circuit" << endl;
#endif

	//11 - compute output
	out = computeOutputs();

#ifdef PRINT_STEPS
		cout << "Outputs computed" << endl;
#endif
	

	cout << "Outputs:" << endl;

	for (int i = 0; i < cyc->outputWires.playerBitAmount; i++)
	{
		cout << out[i];
	}

	
	
	//if (repetition < TEST_ROUNDS - 1)
	  //delete[] out;
	}
	
	deletePartial();
}//end for loop
	//deleteAll();
	return 0;	
}