//
//  MainUEE.cpp
//  BooleanSSE
//
//  Created by Bernardo Ferreira on 16/11/16.
//  Copyright © 2016 Bernardo Ferreira. All rights reserved.
//

#include "SseServer.hpp"

using namespace std;

const char* SseServer::pipeDir = "/tmp/BooleanSSE/";


SseServer::SseServer() {
    //init pipe directory
    if(mkdir(pipeDir, 0770) == -1)
        if(errno != EEXIST)
            pee("Failed to mkdir");
    
    //start iee-server pipe
    char pipeName[256];
    strcpy(pipeName, pipeDir);
    strcpy(pipeName+strlen(pipeName), "iee_to_server");
    if(mknod(pipeName, S_IFIFO | 0770, 0) == -1)
        if(errno != EEXIST)
            pee("Fail to mknod");
    readIeePipe = open(pipeName, O_ASYNC | O_RDONLY);
    
    //start server-iee pipe
    bzero(pipeName,256);
    strcpy(pipeName, pipeDir);
    strcpy(pipeName+strlen(pipeName), "server_to_iee");
    if(mknod(pipeName, S_IFIFO | 0770, 0) == -1)
        if(errno != EEXIST)
            pee("Fail to mknod");
    writeIeePipe = open(pipeName, O_ASYNC | O_WRONLY);
    
    //launch client-iee tunnel
    pthread_t t;
    if (pthread_create(&t, NULL, bridgeClientIeeThread, NULL) != 0)
        pee("Error: unable to create bridgeClientTeeThread");
    
    //start listening for iee calls
    while (true) {
        char cmd;
        if (receiveAll(readIeePipe, &cmd, sizeof(char)) < 0)
            pee("ERROR reading from pipe");
        
        switch (cmd) {
            //setup
            case '1': {
                I = new map<vector<unsigned char>,vector<unsigned char> >;
                break;
            }
            // add
            case '2': {
                const int l_size = 20;
                unsigned char* l = new unsigned char[l_size];
                receiveAll(readIeePipe, (char*)l, l_size);
                vector<unsigned char> l_vector = fillVector(l, l_size);
                delete[] l;
                
                const int d_size = sizeof(int)+16;
                unsigned char* d = new unsigned char[d_size];
                receiveAll(readIeePipe, (char*)d, d_size);
                vector<unsigned char> d_vector = fillVector(d, d_size);
                delete[] d;
                
                (*I)[l_vector] = d_vector;
                break;
            }
            // search
            case '3': {
                
                break;
            }
                
                
                
                
                
                
                

//            //read from W 
//            case '2': {
//                //read token from pipe
//                int tokenSize;
//                read(readIeePipe,&tokenSize,sizeof(int));
//                vector<unsigned char> token;
//                for (int i = 0; i < tokenSize; i++)
//                    read(readIeePipe,&token[i],1);
//                
//                //access W
//                map<vector<unsigned char>,vector<unsigned char>>::iterator it = W->find(token);
//                if (it == W->end()) {
//                    puts("requested W key not found");
//                    break;
//                }
//                
//                //write value to pipe
//                for (int i = 0; i < it->second.size(); i++)
//                    write(writeIeePipe,&(it->second[i]),1);
//                break;
//            }
//            //write to W
//            case '3': {
//                //read token from pipe
//                int tokenSize;
//                read(readIeePipe,&tokenSize,sizeof(int));
//                vector<unsigned char> token;
//                for (int i = 0; i < tokenSize; i++)
//                    read(readIeePipe,&token[i],1);
//                
//                //read value from pipe
//                read(readIeePipe,&tokenSize,sizeof(int));
//                vector<unsigned char> value;
//                for (int i = 0; i < tokenSize; i++)
//                    read(readIeePipe,&value[i],1);
//                
//                //write to W
//                (*W)[token] = value;
//                break;
//            }
//            case '4': { //read from I
//                //read token from pipe
//                int tokenSize;
//                read(readIeePipe,&tokenSize,sizeof(int));
//                vector<unsigned char> token;
//                for (int i = 0; i < tokenSize; i++)
//                    read(readIeePipe,&token[i],1);
//                
//                //access W
//                map<vector<unsigned char>,vector<unsigned char>>::iterator it = I->find(token);
//                if (it == I->end()) {
//                    puts("requested W key not found");
//                    break;
//                }
//                
//                //write value to pipe
//                for (int i = 0; i < it->second.size(); i++)
//                    write(writeIeePipe,&(it->second[i]),1);
//                break;
//            }
//            case '5': { //write to I
//                //read token from pipe
//                int tokenSize;
//                read(readIeePipe,&tokenSize,sizeof(int));
//                vector<unsigned char> token;
//                for (int i = 0; i < tokenSize; i++)
//                    read(readIeePipe,&token[i],1);
//                
//                //read value from pipe
//                read(readIeePipe,&tokenSize,sizeof(int));
//                vector<unsigned char> value;
//                for (int i = 0; i < tokenSize; i++)
//                    read(readIeePipe,&value[i],1);
//                
//                //write to W
//                (*I)[token] = value;
//                break;
//            }

            default:
                printf("SseServer unkonwn command!\n");
        }
    }
}


void* SseServer::bridgeClientIeeThread(void* threadData) {
    //start listening socket
    struct sockaddr_in serv_addr;
    int clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock < 0)
        pee("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(serverPort);
    if (bind(clientSock, (const struct sockaddr *) &serv_addr,(socklen_t)sizeof(serv_addr)) < 0)
        pee("ERROR on binding");
    listen(clientSock,5);
    
    //start iee pipe
    char pipeName[256];
    strcpy(pipeName, pipeDir);
    strcpy(pipeName+strlen(pipeName), "clientBridge");
    if(mknod(pipeName, S_IFIFO | 0770, 0) == -1)
        if(errno != EEXIST)
            pee("SseServer::bridgeClientIeeThread: Fail to mknod");
    int pipefd = open(pipeName, O_ASYNC | O_WRONLY);
    
    while (true) {
        //start listening for client calls
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(clientSock, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            pee("ERROR on accept");
            continue;
        }
        //receive data
        char* buf = new char[sizeof(int)];
        if (receiveAll(newsockfd, buf, sizeof(int)) < 0) {
            close(newsockfd);
            pee("ERROR reading from socket");
        }
        int pos = 0;
        int len = readIntFromArr(buf, &pos);
        
        char* bufAll = new char[len];
        if (receiveAll(newsockfd, bufAll, len) < 0) {
            close(newsockfd);
            pee("ERROR reading from tee pipe");
        }
        close(newsockfd);
        
        //redirect to tee
        if (sendall(pipefd, buf, sizeof(int)) < 0)
            pee("ERROR writing len to tee pipe");
        if (sendall(pipefd, bufAll, len) < 0)
            pee("ERROR writing data to tee pipe");
        
        delete[] buf;
        delete[] bufAll;
    }
//    pthread_exit(NULL);
}

SseServer::~SseServer() {
    delete[] I;
}

vector<unsigned char> SseServer::fillVector(unsigned char* array, int len){
    vector<unsigned char> v(len);
    for (int i = 0; i < len; i++)
        v[i] = array[i];
    return v;
}
