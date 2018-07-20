# PROACTOR PATTERN IMPLEMENTED IN SOCKET SERVER

This code is an implementation of the Proactor pattern with TCP socket server with multiple clients.

To compile run :- 

g++ astcp.cpp -o astcp -std=c++11 -lpthread

./astcp		  

Run the client using telnet from the terminal :-

telnet ip_address 8888

There are two events that can be triggered for testing :-
one     (runs a for loop of 5 with 1sec delay each)
two     (runs a for loop of 10 with 1sec delay each)

Other then the above two commands there is no handler present for and other command and so the server will not respond to any other command

After connecting to the server type "one" or "two" to see the event run.

Multiple clients can connect to the server simultaneously and send multiple request at the same time and they will run in async.
