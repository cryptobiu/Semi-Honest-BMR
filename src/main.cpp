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
