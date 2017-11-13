//
// Created by moriya on 17/10/17.
//

#include "BMRParty.h"

BMRParty::BMRParty(int argc, char* argv[]) : Protocol("Semi Honest BMR", argc, argv){
    p = stoi(arguments["partyID"]);

    cout << "This is player:" << p << endl;

    //1 - Read the circuit from the file
    c = loadCircuit(arguments["circuitFile"].c_str());

    cout << "Number parties: " << c->amountOfPlayers << endl;
    cout << "Number of AND gates: " << c->numOfANDGates << endl;
    cout << "Number of XOR gates: " << c->numOfXORGates << endl;

    //2 - initialize the communication
    initializeCommunication(arguments["partiesFile"].c_str(), c, p);

    initializeRandomness(arguments["key"].c_str(), c->amountOfPlayers);

    //check adversary model
    hm = 0;
    if (argc > 6) hm = stoi(arguments["version"]);

    VERSION(hm);

    vector<string> subTaskNames{"Offline", "Online"};
    timer = new Measurement("SemiHonestBMR", p, c->amountOfPlayers, 1, subTaskNames);

    if (hm == 0) //No honest majority - initialize OT-Extension based algorithm.
    {
        //3.2 - initialize the OTExtension (Base OTs, etc.)
        initializeOTs();
    }
    else { //Honest majoritym - initialize BGW based algorithm

        //3.2 - precompute powers and degree reduction coefficients used in BGW protocol.
        InitializeBGW(hm);//Honest majority - we will use BGW
    }


}


void BMRParty::run() {
    timer->startSubTask();
    runOffline();
    timer->endSubTask(0, 0);

    timer->startSubTask();
    runOnline();
    timer->endSubTask(1, 0);
}

void BMRParty::runOffline() {

    synchronize();

    //Start offline phase
    auto start = scapi_now();

    //4 - load random R, random seeds, random lambdas and compute lambdas/seed of XOR gates
    newLoadRandom();


    //here the regular gates go
    //5 - compute AES of Ag,Bg,Cg,Dg for multiplication gates
    computeGates();//Me & Roi - Requires AES...


    //NEW
    if (hm == 0)//Run OT based protocol
    {

        //6 - multiply the lambdas for multiplication gates - uses OT
        mulLambdasC();

        mulRC();

        //8 - send and receive gates
        exchangeGates();

    } else {//Run BGW based protocol

        //5.2 Create secret-sharings of correct degrees and send/receive
        secretShare();//secret-share lambdas, R, and PRGs - first communication round

        //6 - multiply the lambdas for multiplication gates - local computation
        mulLambdasBGW();//local computation

        if (hm == 1 || hm==3) {//BGW 3 or BGW 4 - need to do degree reduction

            //6.2 - reduce degree of share polynomials (not needed if >2/3 honest)
            reduceDegBGW();

        }

        //7 - according to lambdas multiplication, multiply R for multiplication gates. Add results to Ag,Bg,Cg,Dg respectively.
        if (hm != 3)
            mulRBGW();//local computation
        else
            mulRBGWwithReduction();//run BGW4 with an extra reduction step

        //8 - send and receive gates
        exchangeGatesBGW();//communication round, also exchanges the output lambdas, and reconstructs the gates
    }

    print_elapsed_ms(start, "Offline time: ");

//end offline phase
}

void BMRParty::runOnline() {

    //2 - load inputs from file
    loadInputs(arguments["inputsFile"].c_str(), cyc, p);

    //Start of online phase
    auto start = scapi_now();
    // synchronize();

    //9 - reveal Inputs (XORed with lambda)
    exchangeInputs();

    //10 - reveal corresponding seeds
    exchangeSeeds();

    //11 - compute output
    out = computeOutputs();

    print_elapsed_ms(start, "Online time: ");

    cout << "Outputs:" << endl;

    for (int i = 0; i < cyc->outputWires.playerBitAmount; i++) {
        cout << out[i];
    }

    deletePartial();
}