
```
Code -> polling.c
Input format -> Number of booths
For each booth in new line: Number of voters,Number of EVMs and Maximum capacity for each EVM in this booth.
Compiler -> gcc -lpthread
```
Implementation
-> For Each Booth(Including EVM + Voter) only one mutex is used as described in moodle
-> Two conditional variable for each Booth. One to send signal from EVM to Voter that EVM is ready and other to send signal from voter to EVM that he has finished voting.
-> Each booth thread is created first.
-> Then in *create_booth* function all the thread for voters and EVMs are created.
-> Then in *create_voter* function voter waits till an EVM in same booth is empty.
-> In *create_EVM* function all EVMs threads run till all voters in that booth get finished. EVM is sent to *polling_ready_evm*
-> In *polling_ready_evm* he gives a signal to voter in that booth.
-> After that signal a single voter comes from that booth and sent to *voter_in_slot* function.
-> **Voter votes and sends a return signal** that he has finished voting.
-> Voter thread is closed after that.
-> After that EVM declares himself free and process continue till all voters have finished voting in that booth.
-> After all the voters have finished the voting in a booth all EVM threads are closed in that booth and Booth thread also closes itself.
-> After voting is done in all booths. Program is terminated.
