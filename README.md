# Out-of-band signaling
This is an implementation of **Out-of-band signaling** between client and server.
The goal of the project is to guess a `secret` value that is used as the time between two messages sent from the client to any server.

The project uses the following technologies:
* **multithreading**
* **sockets**
* **shared memory**
* **semaphores & mutex**
* **signals & signal handlers**

## Supervisor
The supervisor is the part of the project where servers are created and terminated.

## Server
The server is the central part of the project as it manages the connections with the various clients and the reception of the various messages through the use of 4 threads.

## Client
The client is the part of the project where messages are sent with a regular time interval defined by their random `secret` value.

## How to run
To run the project you need to run the following commands:
* `make` to compile the project
* `./supervisor [numberOfServers]` to run the supervisor
* `./client [totalServersRunning] [numberOfServerToConnect] [numberOfMessages]` to run the client
> **Note:** `numberOfservers` and `totalServersRunning` must be the same value

You can also run `make tests` to run the supervisor and some clients.