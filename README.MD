##Semi Honest BMR

Implementation of semi-honest secure BMR (Beaver Micali Rogaway) protocol according to the article "Optimizing Semi-Honest Secure Multiparty Computation for the Internet" by Aner Ben-Efraim, Yehuda Lindell, and Eran Omri.

###Compilation

In the master branch, compilation is done by the command "bash build"

In the experimentation branch, compilation is done using the cmake file ("cmake ." and then "make").


Running is done by executing

./SemiHonestBMR -partyID \<Partynum\> -circuitFile \<CircuitFile\> -inputsFile \<InputsFile\> -partiesFile \<IPFile\> 
-key \<Key\> -version \<Version Number\> -internalIterationsNumber \<Internal Iterations Number\>

Partynum is the party number. This number should also match the location of the party's IP in the IP file. 
For non-local execution, it is possible to use "-1" and the party number is taken as the location in the IP file.

CircuitFile is the file of the circuit. All parties must use the exact same file.

InputsFile contains the inputs for the input wires of the party.

IPFile is the IPs (IPv4) of the parties. The file contains the IPs written row after row, without spaces.
The IP of party p should appear at row p+1 (party 0 written on the first row, etc.).
All parties must use the exact same file.

Key should be a random secret key.

The protocol supports 4 different versions:
1. "0" for the OT version, 
2. "1" for the BGW3 version (requires an honest majority), 
3. "2" for the BGW2 version (requires 2/3 honest majority, works only when number of parties >3), 
4. "3" for the BGW4.

All parties must use the same version number.