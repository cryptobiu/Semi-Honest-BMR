//
// Created by moriya on 17/10/17.
//

#ifndef SCAPI_BMRPARTY_H
#define SCAPI_BMRPARTY_H

#include <libscapi/include/cryptoInfra/Protocol.hpp>
#include <libscapi/include/infra/Measurement.hpp>
#include "secCompMultiParty.h"
#include "BMR.h"
#include "BMR_BGW.h"

class BMRParty : public Protocol, public SemiHonest, public MultiParty{

private:
    //honest majority type, player number
    int p, hm;
    Circuit* c;
    bool *out;
    int times;
    Measurement *timer;


public:

    BMRParty(int argc, char* argv[]);
    void run() override;
    bool hasOffline() override { return true; }
    void runOffline() override;
    bool hasOnline() override { return true; }
    void runOnline() override;
    ~BMRParty(){delete timer;}

};


#endif //SCAPI_BMRPARTY_H
