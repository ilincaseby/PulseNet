# README PCOM SERVER-CLIENT TCP
## Ilinca Sebastian-Ionut 321CA

## Difficulties encountered
* One of the hardest things to debug was the fact that after a client connects to a server, it needs to tell his id. The server will then accept or decline the connection by sending him a structure or 0 bytes (when 0 bytes are sent on a socket in a TCP protocol, the connection is closed). By doing this *ping-pong* of informations before sensing and receiving actual data, the revent on the socket of the client was set on POLLIN and in the for loop of the server, it detects it and wait for a command from the new client before it can do anything(I do not know why this thing happened only when the clients connected and then disconnected).
* Another thing that bugged me was not testing with checker by also giving it the permission to modify the TCP buffer from kernel with *'sudo'*.
* By checking the length of the command from stdin on the server to be *4*, it could not detect the message from the checker to exit, it was *6* from a reason or another.

## Implementation logic for the client
* As it starts, the client checks for any input errors that can come by not respecting the length of the id or by giving it a wrong port/ip. It also disable the buffering by giving the command *setvbuf(stdout, NULL, _IONBF, BUFSIZ)*.
* After disabling the Nagle's algorithm and initiating **struct sockaddr_in**, it connects to the server and sends it's id. If the server send back 0 bytes, the client closes the socket and exit the program. **But** if the id is unique or is it found by the server inside a special struct that retains old clients with their notifications *on hold*, the client will call a function which creates an array of *pollfd* and wait in a while-loop for events coming from the server or the file descriptor representing *stdin*.
* If the event comes from stdin, the client must call a special function that has the abilty to interpret to interpret the command. Based on that, the function will not also put in a struct defined specially for this protocol the command interpreted if it is a subscribe/unsubscribe message, but will also return values to understand better what's the next step. To be more specific, **-1** means that the command is unknown, this will have no effect on the flow of the program, but it will print a message to stderr, **0** means that the command *exit* has been given and the client break through the while-loop, the last command given also being closing the socket and **1** means that the parameter given as an argument to the function is completed and ready to be sent to the server. Printed messages when **0** is the returning value will depend on a field in the struct(struct name: sub_unsub_t, field name: sub_unsub, I know it's ambiguous, but in my head made a lot of sense:)) ).
* If the server wants to communicate, a special function to complete the functionality of the normal *recv* will be called to capture all the data. If it returns 0 this means that server wants to **close the connection**, if not, this means that a message from a topic has been arrived(this message can be live, or it could just have been on *hold on* while the client was unsubscribed from a topic with the *sf* field set on 0) and I need to print it in the desired form.
* If the client unsubscribes from a non-existing topic or a topic that is not in his subscribe list, the server will just ignore.

## Implementation logic for the server
* I tried very hard to modularize the logic of the server to be fully functional for other future implementation.
* After sending the command *setvbuf(stdout, NULL, _IONBF, BUFSIZ)* and initializing the sockets for UDP and TCP connection (this include the part where the Nagle is disabled). After this, we'll call a function that takes care of introducing the file descriptors in a pollfd array and initiating a server while-loop.
* If analyzing the code, it can be observed that I have abused maps :)). So, **client_map** retains the socket associated with the ID (this helps for checking very fast if the ID is unique when a client wants to connect), while the **socket_client** reverse the key and value from client_map, being useful in the next scenario: A client wants to disconnect from the server, I don't want to search through all ID's to find the socket associatied so I'll make use of the map. **topic_map** retain for every topic a structure that has as its components the name of the topic and two maps(again maps:)) ) with actively subscribers (with sf 0 or 1) and unsubscribed clients that have been subscribed with sf 1 last time they subscribed to the specified topic. The *sub_client_t* structure used for subscribers retains the socket, sf option and what messages to be send when they're back on(this is applied in the unsubs map). The map **disc_clients** retains inactive clients. When a client disconnects, the server retains the topics where is subscribed with sf 0/1 and unsubbed with sf 1. **(IMPORTANT!!!)Everytime he is reconnected, the server will put him back on the subscriptions in the active list or unsubbed list (if he is unsubbed from a topic with sf 1). Also, everytime a topic posts something, the sever will search through the entire map and see if the topic is in the list for subscribed topics with sf 1 or unsubbed list with sf 1 and treat the cases separately: if is subscribed with sf 1, the messages will be put in a vector and sent with the first reconnection from the client. If the client is unsubscribed with sf 1, the messages will be continuously be put in the vector, *not* being sent when he reconnects, but when he resubscribe.**
* **(VERY IMPORTANT!!!)** When a client wants to resubscribe or subscribe even he is already, **the sf will be updated accordingly**. **Another important thing to mention is that when a client is unsubbed with sf 1 and then resub with sf 0, the server will send to him the messages while he was unsubbed but from now on it will not do the same if unsubbed again because the sf is 0 (of course if it is not modified later).**

## Errors treated
* Fatal errors produced by misinterpreting the input or by not checking the input are avoided by stopping the program. For example if the ID is more than 10 chars long, the client will stop running, the same if some arguments are missing from the server/client.
* Other errors by giving invalid commands are not considered fatal errors so if the client/server receive invalid commands, they'll just print the fact that the commands is not recognized and move on.

## Files needed
* Header files:
    * common.h
    * defined_structs.h
    * helpers.h
    * server_helper.h
* C++ files:
    * common.cpp
    * server_helper.cpp
    * server.cpp
    * sub_unsub.cpp
    * subscriber.cpp
    * udp_receive.cpp
* make:
    * Makefile (**the run is made manually, make just build the executable files**)
* **All needed files will be in the archive, including the checker and udp_client**

## Coding style checker
* After many failed attempts to be able to use a checker, I found **cpplint**. So, as a consequence, the coding style errors are resolved with *cpplint*.
* Ignored errors:
    * Consider using strtok_r(...)
    * Is this a non-const reference? If so, make const or use a pointer
    * Do not use namespace using-directives
* I ignored errors above because of the ease that code is written with *using namespace std* and smart pointers (**&**). I tried using strtok_r but was a little overhead needing to make a pointer to the static allocated buffer and then parse its address decreasing the lizibility of the code. As about thread-safe for future implementation, making the buffer be a copy of the command, I think it will work just fine if the threads are started before initiating the buffer.

## Feedback
* As good as it can be this project for learning new things and encounter difficulties, I think the checker should test more extreme cases, even if the points are accorded after manually testing because the person according the credits may encounter a bug that no checker and no test manually tested by myself could discover, it happens (hope I won't get less credit if it is a bug there :))) ).
* It's not the hardest, nor the easiest project, it's somewhere in the middle, but with a semester like this, it's pretty good :)) .
* **If something is unclear, although I tried my best with comments in the code and in this README, please contact me at the following gmail address: ilincasebastian1406@gmail.com**