//
// Created by moriya on 17/10/17.
//

#ifndef SCAPI_BMRPARTY_H
#define SCAPI_BMRPARTY_H

#include <libscapi/include/cryptoInfra/Protocol.hpp>
#include "secCompMultiParty.h"
#include "BMR.h"
#include "BMR_BGW.h"

class BMRParty : public Protocol, public SemiHonest, public MultiParty{

private:
    //honest majority type, player number
    int p, hm;
//circuit
    Circuit* c;
//the output

    bool *out;


public:

    BMRParty(int argc, char* argv[]);

    void run() override;

    bool hasOffline() override { return true; }

    void runOffline() override;

    bool hasOnline() override { return true; }

    void runOnline() override;

};


#endif //SCAPI_BMRPARTY_H
