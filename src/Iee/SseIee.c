//
//  SseIee.cpp
//  BISEN
//
//  Created by Bernardo Ferreira on 15/11/16.
//  Copyright © 2016 Bernardo Ferreira. All rights reserved.
//

/*
 Required changes are tagged with [BP]

 Modifications:
 -> There is no connection between client and IEE. Client requests will be given as input messages to SseIee.
 -> SseIee runs with a message already on plaintext. Secure channel encryption/decryption is taken care of outside SseIee.
 -> Thus, KCom no longer exists here.
 -> Pipe connections will thus only be required for Iee->Server and Server->Iee. 
*/

#include "SseIee.h"

const char* pipeDir = "/tmp/BooleanSSE/";

void print_buffer(const char* name, const unsigned char * buf, const unsigned long long len) {
    ocall_printf("%s size: %llu\n", name, len);
    for(unsigned i = 0; i < len; i++)
        ocall_printf("%02x", buf[i]);
    ocall_printf("\n");
}

void init_pipes() {
    //init pipe directory
    if(mkdir(pipeDir, 0770) == -1)
        if(errno != EEXIST)
            iee_pee("Failed to mkdir");

    char pipeName[256];

    //start server-iee pipe
    strcpy(pipeName, pipeDir);
    strcpy(pipeName+strlen(pipeName), "server_to_iee");
	//not necessary, server creates file.
    //if(mknod(pipeName, S_IFIFO | 0770, 0) == -1)
        //if(errno != EEXIST)
            //iee_pee("Fail to mknod");
    
    ocall_printf("opening read pipe!\n");
    readServerPipe = ocall_open(pipeName, O_ASYNC | O_RDONLY);

    //start iee-server pipe
    bzero(pipeName,256);
    strcpy(pipeName, pipeDir);
    strcpy(pipeName+strlen(pipeName), "iee_to_server");
	//not necessary, server creates file.
    //if(mknod(pipeName, S_IFIFO | 0770, 0) == -1)
    //    if(errno != EEXIST)
    //       iee_pee("Fail to mknod");

    ocall_printf("opening write pipe!\n");
    writeServerPipe = ocall_open(pipeName, O_ASYNC | O_WRONLY);
    ocall_printf("donarino!\n");
    ocall_printf("Finished IEE init! Gonna start listening for client requests through bridge!\n");
}

void destroy_pipes() {
    close(readServerPipe);
    close(writeServerPipe);
}

// IEE entry point
void f(unsigned char **out, unsigned long long *out_len, const unsigned long long pid, const unsigned char * in, const unsigned long long in_len) {
    #ifdef VERBOSE
    ocall_printf("\n***** Entering IEE *****\n");
    #endif

    // set out variables
    *out = NULL;
    *out_len = 0;

    //setup operation
    if(in[0] == 'i')
        setup(out, out_len, in, in_len);
    //add / update operation
    else if (in[0] == 'a')
        add(out, out_len, in, in_len);
    //search operation
    else if (in[0] == 's')
        search(out, out_len, in, in_len);

    #ifdef VERBOSE
    ocall_printf("\n***** Leaving IEE *****\n\n");
    #endif
}

void setup(unsigned char **out, unsigned long long *out_len, const unsigned char* in, const unsigned long long in_len) {
    int pos = 1;

    #ifdef VERBOSE
    ocall_printf("IEE: Starting Setup!\n");
    #endif

    // read kEnc
    const int kEnc_size = iee_readIntFromArr(in, &pos);

    unsigned char* kEnc = (unsigned char*)malloc(sizeof(unsigned char) * kEnc_size);
    iee_readFromArr(kEnc, kEnc_size, in, &pos);

    // read kF
    const int kF_size = iee_readIntFromArr(in, &pos);
    unsigned char* kF = (unsigned char*)malloc(sizeof(unsigned char) * kF_size);
    iee_readFromArr(kF, kF_size, in, &pos);

    // Keys are  given by the client (as input message)
    setKeys(kEnc, kF);

    // tell server to init index I
    unsigned char op = '1';
    iee_socketSend(writeServerPipe, &op, sizeof(char));

    // output message
    *out_len = 1;
    *out = (unsigned char*)malloc(sizeof(unsigned char));
    (*out)[0] = 0x90;

    #ifdef VERBOSE
    ocall_printf("IEE: Finished Setup!\n");
    #endif
}

void add(unsigned char **out, unsigned long long *out_len, const unsigned char* in, const unsigned long long in_len) {
    #ifdef VERBOSE
    ocall_printf("IEE: Started add!\n");
    #endif

    // read buffer
    int pos = 1;
    while(pos < in_len) {
        //get d,c,w from array
        const int d = iee_readIntFromArr(in, &pos);
        const int c = iee_readIntFromArr(in, &pos);

        // read word
        char* word = (char*)malloc(sizeof(char) * MAX_WORD_SIZE);
        char* tmp = (char*)malloc(sizeof(char));
        int counter = 0;

        do {
            iee_readFromArr(tmp, 1, in, &pos);
            word[counter++] = tmp[0];
        } while(tmp[0] != '\0' && counter < MAX_WORD_SIZE);
        free(tmp);

        // guarantee string is terminated
        word[MAX_WORD_SIZE - 1] = '\0';
        ocall_printf("%s\n", word);

        //calculate key kW (with hmac sha256)
        unsigned char* kW = (unsigned char*)malloc(sizeof(unsigned char) * H_BYTES);
        c_hmac(kW, (unsigned char*)word, strlen(word), get_kF());
        free(word);

        //calculate index position label
        unsigned char* label = (unsigned char*)malloc(sizeof(unsigned char) * H_BYTES);
        c_hmac(label, (unsigned char*)&c, sizeof(int), kW);
        //free(kW);

        //calculate index value enc_data
        int enc_data_size = sizeof(int) + C_EXPBYTES;

        unsigned char* nonce = (unsigned char*)malloc(sizeof(unsigned char) * C_NONCESIZE);
        for(int i= 0; i < C_NONCESIZE; i++) nonce[i] = 0x00;

        unsigned char* enc_data = (unsigned char*)malloc(sizeof(unsigned char) * enc_data_size);
        c_encrypt(enc_data, (unsigned char*)&d, sizeof(int), nonce, get_kEnc());

        //send label and enc_data to server
        unsigned char op = '2';
        //ocall_printf("add %d %d\n", H_BYTES, enc_data_size);
        iee_socketSend(writeServerPipe, &op, sizeof(unsigned char));
        iee_socketSend(writeServerPipe, (unsigned char*)label, H_BYTES);
        iee_socketSend(writeServerPipe, (unsigned char*)enc_data, enc_data_size);

        print_buffer("add label", label, H_BYTES);
        print_buffer("add enc", enc_data, enc_data_size);
        ocall_printf("\n\n");
        free(label);
        free(enc_data);
    }

    #ifdef VERBOSE
    ocall_printf("Finished add in IEE!\n");
    #endif

    // output message
    *out_len = 1;
    *out = (unsigned char*)malloc(sizeof(unsigned char));
    (*out)[0] = 0x90;
}

void get_docs_from_server(vec_token *query, unsigned count_words) {
    #ifdef VERBOSE
    ocall_printf("Requesting docs from server!\n");
    #endif

    // initialise array to hold all tokens in random order
    iee_token *rand[count_words];
    for(unsigned i = 0; i < count_words; i++)
        rand[i] = NULL;

    // randomly fill the array with the tokens we need
    for(unsigned i = 0; i < vt_size(*query); i++) {
        // ignore non-word tokens
        if(query->array[i].type != WORD_TOKEN)
            continue;

        /*for(unsigned ii = 0; ii < count_words; ii++) {
            if(rand[ii])
                ocall_printf("%c %s\n", rand[ii]->type, rand[ii]->word);
            else
                ocall_printf("%d\n", rand[ii]);
        }
        ocall_printf("\n");*/

        // choose a random unoccupied position from the rand array
        int pos;
        do {
            pos = c_random_uint_range(0, count_words);
        } while(rand[pos] != NULL);

        rand[pos] = &(*query).array[i];
    }

    #ifdef VERBOSE
    ocall_printf("Randomized positions!\n");
    #endif

    // request the documents from the server
    for(unsigned i = 0; i < count_words; i++) {
        iee_token *tkn = rand[i];

        //cout << "counter for " << tkn->word << " is " << tkn->counter << endl;
        if(tkn->counter == 0) {
            vec_int dummy;
            tkn->docs = dummy;
        }

        ocall_printf("word is %s\n", tkn->word);
        //calculate key kW
        unsigned char* kW = (unsigned char*)malloc(sizeof(unsigned char) * H_BYTES);
        c_hmac(kW, (unsigned char*)tkn->word, strlen(tkn->word), get_kF());

        //calculate relevant index positions
        unsigned char** labels = (unsigned char**)malloc(sizeof(unsigned char*) * tkn->counter);
        unsigned char* l = (unsigned char*)malloc(sizeof(unsigned char) * H_BYTES);
        for (int c = 0; c < tkn->counter; c++) {
            c_hmac(l, (unsigned char*)&c, sizeof(int), kW);
            unsigned char* label = (unsigned char*)malloc(sizeof(unsigned char) * H_BYTES);
            for (int i = 0; i < H_BYTES; i++)
                label[i] = l[i];

            labels[c] = label;
            print_buffer("label", label,H_BYTES);
            bzero(l, H_BYTES);
        }

        free(l);
        free(kW);

        //request index positions from server
        int len = sizeof(char) + sizeof(int) + tkn->counter * H_BYTES;
        unsigned char* buff = (unsigned char*)malloc(sizeof(unsigned char)* len);
        char op = '3';
        int pos = 0;
        iee_addToArr(&op, sizeof(unsigned char), buff, &pos);
        iee_addIntToArr(tkn->counter, buff, &pos);
        for (int i = 0; i < tkn->counter; i++)
            for (int j = 0; j < H_BYTES; j++)
                iee_addToArr(&(labels[i][j]), sizeof(unsigned char), buff, &pos);

        iee_socketSend(writeServerPipe, buff, len);
        free(buff);

        for (int i = 0; i < tkn->counter; i++)
            free(labels[i]);
        free(labels);

        //decrypt query results
        len = tkn->counter * sizeof(int);
        buff = (unsigned char*)malloc(sizeof(unsigned char)* len);

        unsigned char* nonce = (unsigned char*)malloc(sizeof(char)*C_NONCESIZE);
        for(int i= 0; i < C_NONCESIZE; i++) nonce[i] = 0x00;

        unsigned char* enc_data = (unsigned char*)malloc(sizeof(unsigned char) * 44);
        unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char) * 44);
        pos = 0;
        for (int i = 0; i < tkn->counter; i++) {
            iee_socketReceive(readServerPipe, enc_data, 44);

            c_decrypt(data, enc_data, 44, nonce, get_kEnc());
            iee_addToArr((unsigned char*)data, sizeof(int), buff, &pos);

            //cout << "recv ." << buff << "." << endl;
            /** Another way of doing it
                int d = -1;
                memcpy(&d, data, sizeof(int));
                iee_addIntToArr(d, buff, &pos);
            **/

            bzero(enc_data, symBlocksize);
            bzero(data, symBlocksize);
        }

        // generate int vector
        const int nr_docs = len / sizeof(int);
        vec_int docs; // TODO check if this is always sorted
                      // else has to be sorted in evaluator; may not be needed for vec_int
        vi_init(&docs, nr_docs);
        pos = 0;
        for (int i = 0; i < nr_docs; i++) {
            int tmp = -1;
            iee_memcpy(&tmp, buff + pos, sizeof(int));
            pos += sizeof(int);
            ocall_printf("doc %d\n", tmp);
            vi_push_back(&docs, tmp);
        }

        // insert result into token's struct
        tkn->docs = docs;

        free(enc_data);
        free(data);
    }

    #ifdef VERBOSE
    ocall_printf("Got all docs from server!\n\n");
    #endif
}

void search(unsigned char **output, unsigned long long *out_len, const unsigned char* in, const unsigned long long in_len) {
    #ifdef VERBOSE
    ocall_printf("Search!\n");
    #endif

    vec_token query;
    vt_init(&query, MAX_QUERY_TOKENS);

    int nDocs = -1;
    int count_words = 0; // useful for get_docs_from_server

    //read in
    int pos = 1;
    while(pos < in_len) {
        iee_token tkn;
        tkn.word = NULL;

        char* tmp_type = (char*)malloc(sizeof(char));
        iee_readFromArr(tmp_type, 1, in, &pos);

        tkn.type = tmp_type[0];
        free(tmp_type);

        if(tkn.type == WORD_TOKEN) {
            count_words++;

            // read counter
            tkn.counter = iee_readIntFromArr(in, &pos);

            // read word
            tkn.word = (char*)malloc(sizeof(char) * MAX_WORD_SIZE);
            char* tmp = (char*)malloc(sizeof(char)); // TODO could this be more efficient since we're copying from
                                                     // one char array to another and then to a third one?
                                                     // (in->tmp->tkn.word)
            int counter = 0;
            do {
                iee_readFromArr(tmp, 1, in, &pos);
                tkn.word[counter++] = tmp[0];
            } while(tmp[0] != '\0' && counter < MAX_WORD_SIZE);
            free(tmp);

            // guarantee string is terminated
            tkn.word[MAX_WORD_SIZE - 1] = '\0';
        } else if(tkn.type == META_TOKEN) {
            nDocs = iee_readIntFromArr(in, &pos);
            continue;
        }

        vt_push_back(&query, tkn);
    }

    // get documents from uee
    get_docs_from_server(&query, count_words);

    #ifdef VERBOSE
    ocall_printf("parsed: ");
    for(unsigned i = 0; i < vt_size(query); i++) {
        iee_token x = query.array[i];
        if(x.type == WORD_TOKEN) {
            ocall_printf("%s (", x.word);
            for(unsigned i = 0; i < vi_size(x.docs); i++) {
                if(i < vi_size(x.docs) - 1)
                    ocall_printf("%i,", x.docs.array[i]);
                else
                    ocall_printf("%i); ", x.docs.array[i]);
            }
        } else {
            ocall_printf("%c ", x.type);
        }
    }
    ocall_printf("\n\n");
    #endif

    //calculate boolean formula
    vec_int response_docs = evaluate(query, nDocs);

    #ifdef VERBOSE
    ocall_printf("Query Evaluated in IEE!\n");
    #endif

    // return query results
    unsigned long long output_size = vi_size(response_docs) * sizeof(int);
    *output = (unsigned char*)malloc(sizeof(unsigned char) * output_size);
    pos = 0;

    for(unsigned i = 0; i < vi_size(response_docs); i++) {
        //ocall_printf("%d ", response_docs.array[i]);
        iee_addIntToArr(response_docs.array[i], *output, &pos);
    }
    //ocall_printf("\n");

    vt_destroy(&query);
    vi_destroy(&response_docs);

    #ifdef VERBOSE
    ocall_printf("Finished Search!\n");
    #endif

    *out_len = sizeof(unsigned char) * output_size;
}