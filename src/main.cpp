//#include "ottmain.h"
//#include <thread>
//#include "secCompMultiParty.h"
//#include "BMR.h"
//#include "BMR_BGW.h"
//#include <math.h>
//
////LOCK
//#include <sys/types.h>
//#include <sys/stat.h>
#include "BMRParty.h"

using namespace std;

/*main program. Should receive as arguments:
1. A file specifying the circuit, number of players, input wires, output wires
2. A file specifying the connection details
3. A file specifying the current player, inputs
4. aPlayer number
5. Random seed
*/
int main(int argc, char** argv)
{
	BMRParty party(argc, argv);
	party.run();

	return 0;
}
