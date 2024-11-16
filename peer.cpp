#include <sys/socket.h> //library for socket-related functions
#include <arpa/inet.h>  //library for internet operations
#include <pthread.h>    //library for threading
#include <unistd.h>     //library for close and other system calls
#include <stdio.h>      //library for input-output operations
#include <stdlib.h>     //library for memory allocation and process control
#include <netinet/in.h> //library for internet protocol family
#include <string.h>     //library for string manipulation
#include <iostream>     //library for standard input and output

using namespace std; //use the standard namespace

string name; //store the name of the user
int PORT;    //store the user's port number

//function for sending messages
void sending() {
    char buffer[2000] = {0}; //buffer to store the formatted message

    int PORT_server; //variable to store the recipient's port
    cout << "Enter the port to send a message: "; //prompt for recipient port
    cin >> PORT_server; //read the port

    int sock = 0; //socket file descriptor
    struct sockaddr_in serv_addr; //structure to store server address
    char message[1024] = {0}; //buffer for the message to be sent

    //create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error"); //print error if socket creation fails
        return; //exit the function
    }

    //configure server address
    serv_addr.sin_family = AF_INET; //set address family to IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY; //accept connections from any address
    serv_addr.sin_port = htons(PORT_server); //convert port to network byte order

    //connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed"); //print error if connection fails
        close(sock); //close the socket
        return; //exit the function
    }

    cin.ignore(); //clear input buffer
    cout << "Enter your message: "; //prompt for message
    cin.getline(message, 1024); //read the message

    //format the message with sender information
    snprintf(buffer, sizeof(buffer), "%s [PORT:%d] says: %s", name.c_str(), PORT, message);
    send(sock, buffer, strlen(buffer), 0); //send the message to the server
    cout << "Message sent!" << endl; //confirm the message was sent

    close(sock); //close the socket
}

//function for receiving messages
void receiving(int SFD) {
    struct sockaddr_in addr; //structure to store client address
    int addrlen = sizeof(addr); //store the size of the client address
    char buffer[10000] = {0};  //buffer to store received messages
    fd_set currentSockets, readySockets; //sets for monitoring file descriptors

    FD_ZERO(&currentSockets); //initialize the set
    FD_SET(SFD, &currentSockets); //add server socket to the set

    //infinite loop to handle incoming connections
    for (;;) {
        readySockets = currentSockets; //copy current set to ready set

        //monitor sockets for activity
        if (select(FD_SETSIZE, &readySockets, NULL, NULL, NULL) < 0) {
            perror("Select error"); //print error if select fails
            exit(EXIT_FAILURE); //terminate the program
        }

        //iterate over file descriptors
        for (int i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET(i, &readySockets)) { //check if a descriptor is ready
                if (i == SFD) { //if server socket is ready
                    //accept new client connection
                    int clientSocket = accept(SFD, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
                    if (clientSocket < 0) { //check for errors
                        perror("Accept error"); //print error if accept fails
                        continue; //skip to the next iteration
                    }
                    FD_SET(clientSocket, &currentSockets); //add client socket to the set
                } else { //if client socket is ready
                    int value = recv(i, buffer, sizeof(buffer), 0); //receive data
                    if (value <= 0) { //check if connection closed or error occurred
                        close(i); //close the socket
                        FD_CLR(i, &currentSockets); //remove socket from the set
                    } else { //if data is received
                        cout << "Received: " << buffer << endl; //print the message
                        memset(buffer, 0, sizeof(buffer)); //clear the buffer
                    }
                }
            }
        }
    }
}

//thread function to handle receiving
void *receiveThread(void *serverFileDes) {
    int SFD = *((int *)serverFileDes); //extract server file descriptor
    receiving(SFD); //call the receiving function
    return nullptr; //return null
}

//main function
int main() {
    int serverFileDes; //server file descriptor
    struct sockaddr_in addr; //structure to store server address

    cout << "Enter your name: "; //prompt for user name
    cin >> name; //read user name

    cout << "Enter your port number: "; //prompt for user port
    cin >> PORT; //read user port

    //create a socket
    if ((serverFileDes = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed"); //print error if socket creation fails
        exit(EXIT_FAILURE); //terminate the program
    }

    //configure server address
    addr.sin_family = AF_INET; //set address family to IPv4
    addr.sin_addr.s_addr = INADDR_ANY; //accept connections from any address
    addr.sin_port = htons(PORT); //convert port to network byte order

    //bind the socket to the address
    if (bind(serverFileDes, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed"); //print error if bind fails
        close(serverFileDes); //close the socket
        exit(EXIT_FAILURE); //terminate the program
    }

    //listen for incoming connections
    if (listen(serverFileDes, 5) < 0) {
        perror("Listen failed"); //print error if listen fails
        close(serverFileDes); //close the socket
        exit(EXIT_FAILURE); //terminate the program
    }

    pthread_t threadID; //variable to store thread id
    pthread_create(&threadID, NULL, receiveThread, &serverFileDes); //create a thread for receiving

    //infinite loop for user commands
    for (;;) {
        string input; //variable to store user input
        cout << "Enter a command (send/s, quit/q): "; //prompt for command
        cin >> input; //read user input

        //convert input to lowercase
        for (auto &ch : input) {
            ch = tolower(ch);
        }

        if (input == "quit" || input == "q") { //check for quit command
            cout << "Quitting..." << endl; //print quitting message
            close(serverFileDes); //close the server socket
            break; //exit the loop
        } else if (input == "send" || input == "s") { //check for send command
            sending(); //call the sending function
        } else { //if input is invalid
            cout << "Invalid command!" << endl; //print error message
        }
    }

    pthread_join(threadID, NULL); //wait for the receiving thread to finish
    cout << "Peer-to-peer communication ended." << endl; //print exit message
    return 0; //exit the program
}
