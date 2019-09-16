#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>
#include <pthread.h>
#include <cstring>

using namespace std;

#define PORT "52123"

map<string, vector<string> > userNameToIPDict;
map<string,string> IPTouserNameDict;
string myUsername;
bool logout=true;
socklen_t len;


//Clear all the errors in socket before close
int getSO_ERROR(int fd) {
    int err = 1;
    socklen_t len = sizeof err;
    if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len))
        perror("getSO_ERROR");
    if (err)
        errno = err;
    return err;
}

//Close all the socket
void closeSocket(int fd) {
    if (fd >= 0) {
        getSO_ERROR(fd);
        if (shutdown(fd, SHUT_RDWR) < 0)
            if (errno != ENOTCONN && errno != EINVAL)
                perror("shutdown");
        if (close(fd) < 0)
            perror("close");
    }
}


//Display the menu
void menu(){
    cout<<endl<<endl;
    cout<<"***********************************"<<endl;
    cout<<"Please select one of the options:"<<endl<<endl;
    cout<<"1.Register with the server using command \"r\""<<endl;
    cout<<"2.Log into the server using command \"l\""<<endl;
    cout<<"3.Exit the client program using command \"exit\""<<endl;
    cout<<"***********************************"<<endl;
    cout<<endl<<endl;
}



//Tokenize the string
vector<string> tokenizeTheString(char buf[]){
    vector<string> args;
    string intermediate;
    stringstream bufStream(buf);
    while (getline(bufStream, intermediate, '|')) {
        args.push_back(intermediate);
    }
    return args;
}


//Listening to the all the connected clients
void *listenToClients(void *args){

    int clientfd;
    ssize_t n;
    char buf[1024];

    clientfd = *((int *)args);
    free(args);

    pthread_detach(pthread_self());

    while(logout){
        memset(&buf[0], 0, sizeof(buf));

        n = read(clientfd, buf, sizeof(buf));

        if(n==0){
            //cout<<"The client with fd "<<clientfd<<" disconnected. Stopping the thread"<<endl;
            break;
        }

        else if(strlen(buf)!=0){
            vector<string> args=tokenizeTheString(buf);

            if(args[0].compare("cm")==0){
                cout<<endl<<"You have received a new message from "<<args[1]<<": "<<endl;
                cout<<"\""<<args[2]<<"\""<<endl;
            }
            else{
                cout<<"An invalid message received from the client with fd "<<clientfd<<endl;
            }
        }
    }

    close(clientfd);

    //cout<<"The client loop broke"<<endl;

    return(NULL);

}


//Listen to server for any communication
void *listenToSever(void *args){

    int serverfd;
    ssize_t n;
    char buf[1024];

    serverfd = *((int *)args);
    free(args);

    pthread_detach(pthread_self());

    while(logout){

        memset(&buf[0], 0, sizeof(buf));


        n = read(serverfd, buf, sizeof(buf));

        if(n==0 && logout){
            cout<<"The server has closed. Exiting the client!!!"<<endl;
            exit(1);
        }

        else if(strlen(buf) !=0){

            vector<string> args=tokenizeTheString(buf);


            //Received an invite from a client
            if(args[0].compare("si")==0){

                if(args[1].compare("200")==0){
                    cout<<"The user "<<args[2]<<" has requested to add you as friend. ";

                    if(args.size()==6) {
                        cout<<"With a message. ";
                        cout << args[5] << endl;
                    }
                    cout<<"You can accept by pressing 'ia|username|message'"<<endl;
                }

                else{
                    cout<<"Server sent a wrong invitation. Error!!! "<<buf<<endl;
                }
            }


                //Inviting user Missing
            else if(args[0].compare("sie")==0){

                if(args[1].compare("500")==0){
                    cout << "Could not find the user " << args[2]
                         << "!!! Please check if user is online and the spelling  is correct" << endl;
                }
                else{
                    cout<<"Server replied with unrecognized status.Error!!! "<<buf<<endl;
                }

            }

                //Client has accepted the invitation
            else if(args[0].compare("sia")==0){

                if(args[1].compare("200")==0){
                    cout<<"The user "<<args[2]<<" has accepted your friend request. ";

                    if(args.size()==6) {
                        cout<<"With a message ";
                        cout << args[5] << endl;
                    }
                    cout<<"You can now send messages to each other"<<endl;

                    vector<string> temp;
                    temp.push_back(args[3]);
                    temp.push_back(args[4]);
                    userNameToIPDict.insert(pair<string, vector<string> >(args[2],temp));
                    IPTouserNameDict.insert(pair<string,string>(args[3],args[2]));
                }

                    //The accepted client name is missing
                else if(args[1].compare("500")==0) {

                    cout << "Could not find the user " << args[2]
                         << "!!! Please check the spelling" << endl;
                }

                else{
                    cout<<"The server replied with a wrong code. Error!!! "<<buf<<endl;
                }
            }

                //The invitation acceptance has been approved by server
            else if(args[0].compare("ssi")==0){

                if(args[1].compare("200")==0){
                    cout<<"The invitation acceptance from "<<args[2]<<" has been approved by the server. "
                                                                      <<endl<<"You can start sending messages"<<endl;
                    vector<string> temp;
                    temp.push_back(args[3]);
                    temp.push_back(args[4]);
                    userNameToIPDict.insert(pair<string, vector<string> >(args[2], temp));
                    IPTouserNameDict.insert(pair<string, string>(args[3], args[2]));

                }

                else{
                    cout<<"The server reply could not be understood error!!! "<<buf<<endl;
                }
            }

                //Friend became online message
            else if(args[0].compare("so")==0){

                if(args[1].compare("200")==0) {
                    cout << "Your friend " << args[2] << " has just come online, you can send messages" << endl;
                    //cout<<"All the friends available to chat :"<<endl;
                    vector<string> temp;
                    temp.push_back(args[3]);
                    temp.push_back(args[4]);
                    userNameToIPDict.insert(pair<string, vector<string> >(args[2], temp));
                    IPTouserNameDict.insert(pair<string, string>(args[3], args[2]));
                    if(userNameToIPDict.size()>0) {
                        cout << "Your friends who are online:" << endl;
                        for (auto it = userNameToIPDict.begin(); it != userNameToIPDict.end(); ++it) {
                            cout << it->first << endl;
                        }
                    }
                }
                else{
                    cout<<"The server could not be understood error!!! "<<buf<<endl;
                }

            }


                //A client has exited from the server
            else if(args[0].compare("sxu")==0){

                if(args[1].compare("200")==0){

                    cout<<"The user "<<args[2]<<" has exited from the server"<<endl;

                    if(userNameToIPDict.find(args[2]) != userNameToIPDict.end()){
                        string ip=userNameToIPDict.at(args[2])[0];
                        if(IPTouserNameDict.find(ip) != IPTouserNameDict.end()){
                            IPTouserNameDict.erase(ip);
                        }

                        /*if(userNameToIPDict.at(args[2]).size()==3){
                            int des=stoi(userNameToIPDict.at(args[2])[2]);
                            close(des);
                        }*/
                        userNameToIPDict.erase(args[2]);


                    }
                }
                else{
                    cout<<"The server replied with a wrong code for client exiting from the server "<<buf<<endl;
                }

            }
            else{
                cout<<"Server reply could not be recognized. Error!!! "<<buf<<endl;
            }
        }

    }

    //cout<<"The server loop broke"<<endl;

    return(NULL);
}


//Accepts incomeing connections from other clients
void *acceptTheConnections(void *args) {

    int clientfd, acceptedClientfd;
    struct sockaddr_in acceptedClientAddr;
    pthread_t tid;

    clientfd = *((int *) args);
    free(args);

    pthread_detach(pthread_self());

    while (logout) {

        memset(&acceptedClientAddr, 0, sizeof(struct sockaddr_in));

        if ((acceptedClientfd = accept(clientfd, (struct sockaddr *) (&acceptedClientAddr), &len)) < 0) {
            if (errno == EINTR) {
                continue;
            }

            else if(logout==false){
                break;
            }

            else{
                perror(":accept error");
                exit(1);
            }
        }

        string Ip = inet_ntoa(acceptedClientAddr.sin_addr);
        string port = to_string(ntohs(acceptedClientAddr.sin_port));


        int *acceptedClientfdPtr;
        acceptedClientfdPtr = (int *) malloc(1 * sizeof(int));
        *acceptedClientfdPtr = acceptedClientfd;

        if (IPTouserNameDict.find(Ip) != IPTouserNameDict.end()) {
            userNameToIPDict.at(IPTouserNameDict.at(Ip)).push_back(to_string(acceptedClientfd));
            pthread_create(&tid, NULL, &listenToClients, (void *) acceptedClientfdPtr);
            //cout << "Thread created for the client IP: " << Ip << ", Port: " << port << endl;
        } else {
            cout << "The username of the accepted client " << Ip << " is missing. Closing the connection" << endl;
            close(acceptedClientfd);
        }
    }

    //cout << "Breaking the accept connection loop" << endl;

    return (NULL);
}




int main(int argc, char * argv[])
{

    struct sockaddr_in serverAddr, clientAddr;
    char buf[1024];
    int serverfd,clientfd;
    pthread_t tid, acceptTid;



    if(argc !=2){
        cout<<"Please pass two parameters of user info and configuration file"<<endl;
        return 1;
    }

    fstream configurationFile;
    configurationFile.open(argv[1]);
    string configuration;
    size_t pos = 0;

    vector<string> args;
    string intermediate;

    while(getline(configurationFile, configuration)) {
        stringstream configurationStream(configuration);
        while (getline(configurationStream, intermediate, ':')) {
            args.push_back(intermediate);
        }
    }


    string serverIp=string(args[1]);
    string serverPort=string(args[3]);

    //servhost:1.2.3.4
    //servport:100


    while(true) {


        memset(&serverAddr, 0, sizeof(struct sockaddr_in));

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons((short)atoi(serverPort.c_str()));
        serverAddr.sin_addr.s_addr = inet_addr(serverIp.c_str());

        if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror(": Can't get socket");
            exit(1);
        }

        if (connect(serverfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            perror(": connect");
            exit(1);
        }

        menu();

        string selection;
        cin >> selection;

        //Register with the server
        if (selection.compare("r") == 0) {
            bool success=false;
            string username;
            string password;


            //Loop until registration is successful
            while(success==false){

                //Send the message to the server
                cout<<"Enter Username: ";
                cin>>username;
                cout<<"Enter Password: ";
                cin>>password;

                string messageToSend="r|"+username+"|"+password;

                write(serverfd, messageToSend.c_str(),strlen(messageToSend.c_str()));



                //Read from the server
                memset(&buf[0], 0, sizeof(buf));
                if (read(serverfd, buf, 1024)==0) {
                    printf("Server closed the connection.\n");
                    exit(0);
                }

                else if(strlen(buf)!=0){

                    args.clear();
                    args =tokenizeTheString(buf);

                    if(args[0].compare("sr")==0){

                        //Registration successful successful
                        if(args[1].compare("200")==0){
                            cout<<"Registration Successful"<<endl;

                            success=true;
                            myUsername=username;
                        }
                        else if(args[1].compare("500")==0){
                            cout<<"The username is already taken Please try again!!!"<<endl;
                        }
                        else{
                            cout<<"Something went wrong, returned other than 500 or 200. Please try again!!! "<<buf<<endl;
                        }
                    }
                    else{
                        cout<<"Something went wrong returned a different message. Please try again!!! "<<buf<<endl;
                    }
                }
            }

        }


            //Login to the server
        else if (selection.compare("l") == 0) {

            bool success=false;
            string username;
            string password;

            //Send the message to the server
            while(success==false) {
                cout << "Enter Username: ";
                cin >> username;
                cout << "Enter Password: ";
                cin >> password;
                string messageToSend = "l|" + username + "|" + password;

                write(serverfd, messageToSend.c_str(), strlen(messageToSend.c_str()));

                //Read from the server
                memset(&buf[0], 0, sizeof(buf));
                if (read(serverfd, buf, 1024) == 0) {
                    printf("server closed the connection.\n");
                    exit(0);
                } else if (strlen(buf) != 0) {
                    args.clear();
                    args = tokenizeTheString(buf);

                    if (args[0].compare("sl") == 0) {

                        //Login successful successful
                        if (args[1].compare("200") == 0) {
                            success=true;
                            myUsername=username;
                            cout<<"Login Successful"<<endl;


                            //Read and print all the online friends
                            for(int i=3;i<args.size();i=i+3){
                                vector<string> temp;
                                temp.push_back(args[i+1]);
                                temp.push_back(args[i+2]);
                                userNameToIPDict.insert(pair<string,vector<string> >(args[i],temp));
                                IPTouserNameDict.insert(pair<string,string>(args[i+1],args[i]));
                            }

                            if(userNameToIPDict.size()>0) {
                                cout << "Your friends who are online:" << endl;
                                for (auto it = userNameToIPDict.begin(); it != userNameToIPDict.end(); ++it) {
                                    cout << it->first << endl;
                                }
                            }
                            else{
                                cout<<"None of your friends are online"<<endl;
                            }
                        }

                            //Login Failed
                        else if (args[1].compare("500") == 0) {
                            cout<<"Username or Password is incorrect. Please try again!!!"<<endl;
                        }

                        else {
                            cout<<"Something went wrong, returned other than 500 or 200. Please try again!!! "<<buf<<endl;
                        }
                    }

                    else {
                        cout<<"Something went wrong returned a different message. Please try again!!! "<<buf<<endl;
                    }
                }
            }

        }

        else if (selection.compare("exit") == 0) {
            cout<<"Closing the connection";
            exit(0);
        }

        else {
            cout << "Invalid option Please try again" << endl;
            continue;
        }

        memset(&clientAddr, 0, sizeof(struct sockaddr_in));

        clientAddr.sin_addr.s_addr = INADDR_ANY;
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port=htons((short) atoi(PORT));

        if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror(": Can't get socket");
            exit(1);
        }

        if (::bind(clientfd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
            perror(": bind");
            exit(1);
        }

        len = sizeof(clientAddr);

        if (getsockname(clientfd, (struct sockaddr *)&clientAddr, &len) < 0) {
            perror(": can't get name");
            _exit(1);
        }

        //printf("The Client is listening ip = %s, port = %d\n", inet_ntoa(clientAddr.sin_addr), htons(clientAddr.sin_port));

        if (listen(clientfd, 5) < 0) {
            perror(": bind");
            exit(1);
        }

        logout=true;


        //Creating thread to listen to the server
        int *serverfdPtr;
        serverfdPtr = (int *)malloc(1*sizeof(int));
        *serverfdPtr = serverfd;


        pthread_create(&tid, NULL, &listenToSever,(void *)serverfdPtr);



        //Creating thread to accept connection from clients
        int *clientfdPtr;
        clientfdPtr = (int *)malloc(1* sizeof(int));
        *clientfdPtr = clientfd;


        pthread_create(&acceptTid, NULL, &acceptTheConnections, (void *)clientfdPtr);
        cin.ignore();




        //Listensing to all the client connection requests
        while(logout) {

            cout<<endl<<endl;
            cout<<"******************************************************"<<endl;
            cout<<"Please select one of the following:"<<endl<<endl;
            cout<<"1. Message a friend by typing 'm|username|message to send'"<<endl;
            cout<<"2. Invite a friend by typing 'i|username|invitation message'"<<endl;
            cout<<"3. Accept the invitation of a friend by typing 'ia|username|invitation accept message'"<<endl;
            cout<<"4. Logout from the server by typing 'logout'"<<endl;
            cout<<"******************************************************"<<endl;
            cout<<endl;


            string input;

            getline(cin,input,'\n');

            //cin>>input;

            char buf[input.length()];
            strcpy(buf,input.c_str());

            //cin.ignore();

            vector<string> args=tokenizeTheString(buf);

            if(args[0].compare("m")==0){
                if(args.size()<3){
                    cout<<"Error the message format is wrong. Please try again!!!"<<endl<<endl;
                    continue;
                }

                if(userNameToIPDict.find(args[1]) != userNameToIPDict.end()){

                    //cout<<"Sending the message..."<<endl;

                    if(userNameToIPDict.at(args[1]).size()==3){
                        //cout<<"No socket creation"<<endl;

                        int des=stoi(userNameToIPDict.at(args[1])[2]);
                        string messageToSend="cm|"+myUsername+"|"+args[2];

                        write(des,messageToSend.c_str(),strlen(messageToSend.c_str()));

                    }

                    else{


                        //Creating the connection to the client
                        //cout<<"Connecting to the client..."<<endl;

                        //cout<<"socket created"<<endl;

                        struct sockaddr_in createdClientAddr;
                        int createdClientfd;
                        pthread_t tid;

                        memset(&createdClientAddr, 0, sizeof(struct sockaddr_in));

                        string ip=userNameToIPDict.at(args[1])[0];


                        createdClientAddr.sin_family = AF_INET;
                        createdClientAddr.sin_port = htons((short)atoi(PORT));
                        createdClientAddr.sin_addr.s_addr = inet_addr(ip.c_str());

                        if ((createdClientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                            perror(": Can't get socket");
                            exit(1);
                        }
                        //cout<<"Got the socket"<<endl;

                        if (connect(createdClientfd, (struct sockaddr *)&createdClientAddr, sizeof(createdClientAddr)) < 0) {
                            perror(": connect");
                            exit(1);
                        }

                        userNameToIPDict.at(args[1]).push_back(to_string(createdClientfd));


                        string messageToSend="cm|"+myUsername+"|" + args[2];


                        write(createdClientfd,messageToSend.c_str(),strlen(messageToSend.c_str()));


                        //Create a thread to listen to the client
                        int *createdClientfdPtr;
                        createdClientfdPtr = (int *)malloc(sizeof(int));
                        *createdClientfdPtr = createdClientfd;

                        pthread_create(&tid, NULL, &listenToClients, (void *)createdClientfdPtr);

                        //cout<<"Thread created for the client IP: "<<ip<< ", Port: "<<PORT<<endl;
                    }

                    cout<<"Message sent"<<endl;
                }
                else{
                    cout<<"The username "<<args[1]<<" is missing from your friend list. Please try again!!!"<<endl;
                }

            }


                //inviting a fiend
            else if(args[0].compare("i")==0){
                if(args.size()<2){
                    cout<<"Error the invitation format is wrong. Please try again!!!"<<endl<<endl;
                    continue;
                }

                string messageToSend="i|"+myUsername+"|"+args[1];
                if(args.size()==3){
                    messageToSend+="|"+args[2];
                }
                write(serverfd,messageToSend.c_str(),strlen(messageToSend.c_str()));

                cout<<"Invite sent"<<endl;

            }


                //Accept the invitation
            else if(args[0].compare("ia")==0){

                if(args.size()<2){
                    cout<<"Error the invitation accept format is wrong. Please try again!!!"<<endl<<endl;
                    continue;
                }

                string messageToSend="ia|"+myUsername+"|"+args[1];
                if(args.size()==3){
                    messageToSend+="|"+args[2];
                }

                write(serverfd,messageToSend.c_str(),strlen(messageToSend.c_str()));

                cout<<"Invitation accepted"<<endl;


            }

                //Logout from the server
            else if(args[0].compare("logout")==0){

                string messageToSend="x|"+myUsername;
                write(serverfd,messageToSend.c_str(),strlen(messageToSend.c_str()));
                logout=false;

                closeSocket(clientfd);

                for ( auto it = userNameToIPDict.begin(); it != userNameToIPDict.end(); it++ )
                {
                    if(it->second.size()==3){
                        int des=stoi(it->second[2]);
                        closeSocket(des);
                    }
                }
                userNameToIPDict.clear();
                IPTouserNameDict.clear();

                cout<<"Please wait closing all the connections..."<<endl;
                sleep(5);
                cout<<"Successfully logged out"<<endl;
                break;
            }

            else{
                cout<<"Please enter a valid option. Try again !!!"<<endl;
            }

        }

    }

}
