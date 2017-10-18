DESCRIPTION

Implementation of semi-honest secure BMR (Beaver Micali Rogaway) protocol from "Optimizing Semi-Honest Secure Multiparty Computation for the Internet" by Aner Ben-Efraim, Yehuda Lindell, and Eran Omri.

COMPILATION

In the master branch, compilation is done by the command "bash build"

In the experimentation branch, compilation is done using the cmake file ("cmake ." and then "make").

RUNNING

Running is done by executing

"BMRPassive -partyID <Partynum> -circuitFile <CircuitFile> -inputsFile <InputsFile> -partiesFile <IPFile> -key <Key> -version <Version Number>"

Partynum is the party number. This number should also match the location of the party's IP in the IP file. For non-local execution, it is possible to use "-1" and the party number is taken as the location in the IP file.

CircuitFile is the file of the circuit. All parties must use the exact same file.

InputsFile contains the inputs for the input wires of the party.

IPFile is the IPs (IPv4) of the parties. The file contains the IPs written row after row, without spaces. The IP of party p should appear at row p+1 (party 0 written on the first row, etc.). All parties must use the exact same file.

Key should be a random secret key.

Version Number is the version we use. Use "0" for the OT version, "1" for the BGW3 version (requires an honest majority), "2" for the BGW2 version (requires 2/3 honest majority, and works only for number of parties >3), "3" for the BGW4. All parties must use the same version number.