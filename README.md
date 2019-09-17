# Messenger-Application
* Messenger Application is a C++ application which facilitates client to client to communication, with the help of a server.

* The Client and Server can handle concurrent requests using pthread and select.

## Server
* The Server accepts two files user info file and configuration file as command-line arguments.

* The user info file contains the username, password of previously logged in users, along with their friends.

* The configuration file contains the port number in which the server has to be run.

* The server maintains the information of all the users, which includes logged in users, logged out users and previously logged in users.

* The server handles multiple requests from the clients using select by blocking on different file descriptors.

* New users can register with the server using the IP address and the port of the server by sending a register request.

* Users can also send invitations to other clients through the server and if the user chooses to accept it, then they will be marked as friends and they can start communicating with each other.

* The server notifies all the online friends when a client logins or logouts from the server and updates the online user list.

* Server also notifies all the clients if by any unforeseen circumstances it is closed.

## Client

* Similar to the server, client also accepts configuration files, which tells the client, IP address and Port of the server.

* The client is provided with a list all the online friends including their port and IP address by the server when the client logins. This list is maintained by the client for future communication.

* The client opens sockets using pthread for all its online friends, using this opened sockets client to client direct communication is achieved.

* The client updates its online friends including the opened sockets using the user login and logout information received from the server
