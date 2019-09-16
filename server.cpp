#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <netinet/in.h>
#include <string>
#include <cstring>



using namespace std;

void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child %d terminated.\n", pid);
    return ;
}

int main(int argc, char * argv[]) {

    if(argc !=3){
        cout<<"Please pass two parameters of user info and configuration file"<<endl;
        return 1;
    }

    struct sockaddr_in addr, recaddr, clientAddr;
    char buf[1024];

    memset(&addr, 0, sizeof(struct sockaddr_in));



    fstream configurationFile;
    configurationFile.open(argv[2]);
    string configuration;
    size_t pos = 0;


    getline(configurationFile, configuration);

    vector<string> args;
    string intermediate;
    stringstream configurationStream(configuration);

    while (getline(configurationStream, intermediate, ':')) {
        args.push_back(intermediate);
    }

    if (args[0] == "port") {
        addr.sin_port = htons((short) atoi(args[1].c_str()));
    } else {
        cout << "Error in the configuration file Assigning available port number" << endl;
        addr.sin_port = htons((short) atoi("0"));
    }

    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    //printf("ip = %s, port = %d\n", inet_ntoa(addr.sin_addr), htons(addr.sin_port));



    fstream userInfoFile;
    userInfoFile.open(argv[1]);
    string userInfo;
    args.clear();

    vector<vector<string> > userInfoVector;


    while (getline(userInfoFile, userInfo)) {
        vector<string> args;
        stringstream userInfoStream(userInfo);
        while (getline(userInfoStream, intermediate, '|')) {
            args.push_back(intermediate);
        }

        if (args.size() > 2) {
            stringstream friendListStream(args.back());
            args.pop_back();
            while (getline(friendListStream, intermediate, ';')) {
                args.push_back(intermediate);
            }
        }
        userInfoVector.push_back(args);
    }


    /*for(int i=0;i<userInfoVector.size();i++){
        for(int j=0;j<userInfoVector[i].size();j++){
            cout<<userInfoVector[i][j]<<" ";
        }
        cout<<endl;
    }*/



    map<string, vector<string> > userNameToIPDict;
    map<int, string> sockToUserName;
    vector<int> sock_vector;

    int sockfd, rec_sock;
    socklen_t len;
    fd_set allset, rset;
    int maxfd;


    struct sigaction abc;
    abc.sa_handler = sig_chld;
    sigemptyset(&abc.sa_mask);
    abc.sa_flags = 0;
    sigaction(SIGCHLD, &abc, NULL);


    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror(": Can't get socket");
        return 1;
    }

    if (::bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror(": bind");
        return 1;
    }

    len = sizeof(addr);

    if (getsockname(sockfd, (struct sockaddr *) &addr, &len) < 0) {
        perror(": can't get name");
        return 1;
    }

    cout << "Listening: ip = " << inet_ntoa(addr.sin_addr) << " port = " << htons(addr.sin_port)<<endl;


    if (listen(sockfd, 5) < 0) {
        perror(": bind");
        return 1;
    }


    FD_ZERO(&allset);
    FD_SET(sockfd, &allset);
    maxfd = sockfd;

    cout<<"Server Creation Successful"<<endl<<endl;
    cout<<"Waiting for connections..."<<endl<<endl;


    while (true) {
        //cout<<"Starting the loop"<<endl;
        rset = allset;
        select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset)) {

            memset(&recaddr, 0, sizeof(struct sockaddr_in));

            if ((rec_sock = accept(sockfd, (struct sockaddr *) (&recaddr), &len)) < 0) {
                if (errno == EINTR)
                    continue;
                else {
                    perror(":accept error");
                    exit(1);
                }
            }

            //printf("Remote machine connection accepted = %s, port = %d.\n", inet_ntoa(recaddr.sin_addr),ntohs(recaddr.sin_port));

            sock_vector.push_back(rec_sock);
            FD_SET(rec_sock, &allset);
            if (rec_sock > maxfd) {
                maxfd = rec_sock;
            }
        }

        auto itr = sock_vector.begin();
        while (itr != sock_vector.end()) {
            int num, fd;
            fd = *itr;
            if (FD_ISSET(fd, &rset)) {

                memset(&buf[0], 0, sizeof(buf));

                num = read(fd, buf, 1024);

                if (num == 0) {
                    /* client exits */
                    close(fd);
                    FD_CLR(fd, &allset);
                    itr = sock_vector.erase(itr);

                    string username;
                    if (sockToUserName.find(fd) != sockToUserName.end()) {
                        username=sockToUserName.at(fd);
                        for (int i = 0; i < userInfoVector.size(); i++) {
                            if (userInfoVector[i][0].compare(username) == 0) {
                                for (int j = 2; j < userInfoVector[i].size(); j++) {
                                        if (userNameToIPDict.find(userInfoVector[i][j]) != userNameToIPDict.end()) {
                                            string messageToSend = "sxu|200|" + username;
                                            int des = stoi(userNameToIPDict.at(userInfoVector[i][j])[2]);
                                            write(des, messageToSend.c_str(), strlen(messageToSend.c_str()));
                                        }
                                    }
                                }
                            }
                        }

                    if (sockToUserName.find(fd) != sockToUserName.end()) {
                        if (userNameToIPDict.find(sockToUserName.at(fd)) != userNameToIPDict.end()) {
                            userNameToIPDict.erase(sockToUserName.at(fd));
                        }
                        sockToUserName.erase(fd);
                    }
                    if(strlen(username.c_str())==0){
                        //cout<<"One of the client with fd "<<fd<<" exited abruptly"<<endl;
                    }
                    else{
                        cout<<"The client "<<username<<" exited"<<endl;
                        cout << "The total number of user online is: " << userNameToIPDict.size() << endl;
                    }




                    continue;
                } else if (strlen(buf) !=0) {

                    args.clear();
                    stringstream readMessageStream(buf);
                    while (getline(readMessageStream, intermediate, '|')) {
                        args.push_back(intermediate);
                    }

                    if (args[0].compare("r") == 0) {

                        //cout<<"Registering a user"<<endl;
                        //Format r|userName|Password|IP address|Port
                        string messageTosend;
                        int isUserNamePresent = false;
                        string userName = args[1];
                        string password = args[2];



                        //Find if the username exists
                        for (auto row = userInfoVector.begin(); row != userInfoVector.end(); row++) {
                            if (userName.compare(row->front()) == 0) {
                                isUserNamePresent = true;
                                break;
                            }
                        }

                        //Send message back to the client
                        if (isUserNamePresent) {

                            string messageTosend = "sr|500|Username already exists";
                            write(fd, messageTosend.c_str(), strlen(messageTosend.c_str()));
                        } else {
                            string messageTosend = "sr|200|User created";

                            vector<string> temp;
                            temp.push_back(userName);
                            temp.push_back(password);
                            userInfoVector.push_back(temp);

                            temp.clear();


                            memset(&clientAddr, 0, sizeof(struct sockaddr_in));

                            if(getpeername(fd, (struct sockaddr *) (&clientAddr), &len)==0){
                                temp.push_back(inet_ntoa(clientAddr.sin_addr));
                                temp.push_back(to_string(ntohs(clientAddr.sin_port)));
                            }

                            else{
                                perror(":get IP and Port of client error");
                                exit(1);
                            }

                            temp.push_back(to_string(fd));

                            userNameToIPDict.insert(pair<string, vector<string> >(userName, temp));

                            sockToUserName.insert(pair<int, string>(fd, userName));

                            write(fd, messageTosend.c_str(), strlen(messageTosend.c_str()));

                            cout << "The total number of user online is: " << userNameToIPDict.size() << endl;
                        }

                    }



                    else if (args[0].compare("l") == 0) {

                        //cout<<"Logging in the user"<<endl;

                        //Format l|userName|password|IP|Port
                        int isMatch = false;
                        string userName = args[1];
                        string password = args[2];
                        for (auto row = userInfoVector.begin(); row != userInfoVector.end(); row++) {
                            auto col = row->begin();
                            if (userName.compare(*col) == 0) {
                                col++;
                                if (password.compare(*col) == 0) {
                                    isMatch = true;
                                    break;
                                }
                            }
                        }
                        if (isMatch) {
                            vector<string> temp;

                            memset(&clientAddr, 0, sizeof(struct sockaddr_in));

                            if(getpeername(fd, (struct sockaddr *) (&clientAddr), &len)==0){
                                temp.push_back(inet_ntoa(clientAddr.sin_addr));
                                temp.push_back(to_string(ntohs(clientAddr.sin_port)));
                            }

                            else{
                                perror(":get IP and Port of client error");
                                exit(1);
                            }

                            temp.push_back(to_string(fd));
                            userNameToIPDict.insert(pair<string, vector<string> >(userName, temp));
                            sockToUserName.insert(pair<int, string>(fd, userName));

                            temp.clear();

                            //Find all the online users

                            for (auto row = userInfoVector.begin(); row != userInfoVector.end(); row++) {
                                if (userName.compare(row->front()) == 0) {
                                    auto col = row->begin();
                                    col++;
                                    col++;
                                    while (col != row->end()) {
                                        temp.push_back(*col);
                                        col++;
                                    }
                                }
                            }


                            //Send the message to the logged in client
                            string messageToSend = "sl|200|User Logged In";

                            for (auto row = temp.begin(); row != temp.end(); row++) {
                                if (userNameToIPDict.find(*row) != userNameToIPDict.end()) {
                                    messageToSend += "|" + *row + "|" + userNameToIPDict.at(*row)[0] + "|" +
                                                     userNameToIPDict.at(*row)[1];
                                }
                            }

                            write(fd, messageToSend.c_str(), strlen(messageToSend.c_str()));


                            //Send the message to all online friends of the logged in user

                            for (auto row = temp.begin(); row != temp.end(); row++) {
                                if (userNameToIPDict.find(*row) != userNameToIPDict.end()) {
                                    if (userNameToIPDict.find(userName) != userNameToIPDict.end()) {
                                        string messageToSend = "so|200|" + userName + "|" +
                                                        userNameToIPDict.at(userName)[0] + "|" +
                                                        userNameToIPDict.at(userName)[1];
                                        int des = stoi(userNameToIPDict.at(*row)[2]);
                                        write(des, messageToSend.c_str(), strlen(messageToSend.c_str()));
                                    }
                                }
                            }

                            cout << "The total number of user online is: " << userNameToIPDict.size() << endl;
                        } else {
                            string messageToSend = "sl|500|Wrong Username or Password";
                            write(fd, messageToSend.c_str(), strlen(messageToSend.c_str()));
                        }

                    }



                    else if (args[0].compare("i") == 0) {

                        //cout<<"Invitation for a client recieved"<<endl;


                        //Format i|userName|userNameInviting|Message

                        //Send message to the inviting user
                        if (userNameToIPDict.find(args[2]) != userNameToIPDict.end()) {
                            string messageToSend = "si|200|" + args[1] + "|" + userNameToIPDict.at(args[1])[0]
                                                   + "|" + userNameToIPDict.at(args[1])[1];
                            if (args.size() == 4) {
                                messageToSend += "|" + args[3];
                            }
                            int des = stoi(userNameToIPDict.at(args[2])[2]);
                            write(des, messageToSend.c_str(), strlen(messageToSend.c_str()));
                        }

                        //The user is absent send a error message
                        else {
                            string messageToSend = "sie|500|" + args[2] + "|" + "Missing";
                            write(fd, messageToSend.c_str(), strlen(messageToSend.c_str()));
                        }
                    }



                    else if (args[0].compare("ia") == 0) {

                        //cout<<"Invitation acceptence request recieved"<<endl;

                        //Format ia|userName|userNameInvitingAccept|Message

                        //Add friend to the inviting and invitation accepted user
                        if (userNameToIPDict.find(args[2]) != userNameToIPDict.end()) {
                            string messageToSend = "sia|200|";
                            for (int i = 0; i < userInfoVector.size(); i++) {
                                if (args[1].compare(userInfoVector[i][0]) == 0) {
                                    userInfoVector[i].push_back(args[2]);
                                }
                                if (args[2].compare(userInfoVector[i][0])==0) {
                                    userInfoVector[i].push_back(args[1]);
                                }
                            }


                            //Send the accepted message to the inviting user
                            messageToSend=messageToSend + args[1] + "|" + userNameToIPDict.at(args[1])[0] + "|" +
                            userNameToIPDict.at(args[1])[1];

                            if (args.size() == 4) {
                                messageToSend += "|" + args[3];
                            }

                            int des = stoi(userNameToIPDict.at(args[2])[2]);
                            write(des, messageToSend.c_str(), strlen(messageToSend.c_str()));


                            //Send the message approve message to accepting user
                            messageToSend="ssi|200|"+args[2]+"|"+userNameToIPDict.at(args[2])[0] + "|" + userNameToIPDict.at(args[2])[1];
                            write(fd,messageToSend.c_str(),strlen(messageToSend.c_str()));

                        }

                        //Error the inviting user after invitation is missing
                        else {
                            string messageToSend = "sia|500|" + args[2] + "|" + "Missing";
                            write(fd, messageToSend.c_str(), strlen(messageToSend.c_str()));
                        }
                    }


                    //logout the user
                    else if (args[0].compare("x") == 0) {
                        //cout<<"Logging out the user"<<endl;
                        //Format x|userName|IP|Port

                        //Clear the information of the user
                        close(fd);
                        FD_CLR(fd, &allset);

                        if (sockToUserName.find(fd) != sockToUserName.end()) {
                            sockToUserName.erase(fd);
                        }
                        if (userNameToIPDict.find(args[1]) != userNameToIPDict.end()) {
                            userNameToIPDict.erase(args[1]);
                        }

                        itr = sock_vector.erase(itr);

                        //cout<<"Sending notification to the friends"<<endl;


                        //Send notification to all the online friends
                        for (int i = 0; i < userInfoVector.size(); i++) {
                            if (userInfoVector[i][0].compare(args[1]) == 0) {
                                for (int j = 2; j < userInfoVector[i].size(); j++) {
                                    if (userNameToIPDict.find(userInfoVector[i][j]) != userNameToIPDict.end()) {
                                        string messageToSend = "sxu|200|" + args[1];
                                        int des = stoi(userNameToIPDict.at(userInfoVector[i][j])[2]);
                                        write(des, messageToSend.c_str(), strlen(messageToSend.c_str()));
                                    }
                                }
                            }
                        }
                        cout<<"The client "<<args[1]<<" exited"<<endl;
                        cout << "The total number of user online is: " << userNameToIPDict.size() << endl;
                    }
                }

            }
            if(itr !=sock_vector.end())
                ++itr;

        }

        maxfd = sockfd;
        if (!sock_vector.empty()) {
            maxfd = max(maxfd, *max_element(sock_vector.begin(),
                                            sock_vector.end()));
        }
    }
}


