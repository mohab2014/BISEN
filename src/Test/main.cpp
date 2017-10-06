//
//  main.cpp
//  BISEN
//
//  Created by Bernardo Ferreira and Guilherme Borges.
//  Copyright © 2016 Bernardo Ferreira. All rights reserved.
//

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "types.h"
#include "../Common/Utils.h"
extern "C" {
#include "../Iee/SseIee.h"
}
#include "../Client/SseClient.hpp"

// DEBUGGING
//#include <map>
//map<int, int> f;

/*
extern void f(
  bytes *out,
  size *out_len,
  const label pid,
  const bytes in,
  const size in_len
);
*/
extern int f_assert(
  size test_index,
  bytes out,
  size outlen
);

void printResults (vector<int> results) {
    #ifdef VERBOSE
    if(!results.size()) {
        printf("none\n");
        return;
    }

    sort(results.begin(), results.end());
    for(unsigned i = 0; i < results.size(); i++)
        printf("%i ", results[i]);

    printf("\n");
    //f[results.size()]++;
    #endif
}

int main(int argc, const char * argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    // init iee
    init_pipes();

    SseClient client;
    char* data;
    int data_size = client.setup(&data);

    ////////////////////////////////////////////////////////////////////////////
    // SETUP ///////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    #ifdef VERBOSE
    printf("Starting IEE communication\n");
    #endif

    unsigned char ** output;
    unsigned long long * output_size;printf("HI\n");
    f(output, output_size, 0, (const unsigned char*) data, data_size);
    for(unsigned long long i = 0; i < *output_size; i++)
        printf("%02x", (*output)[i]);
    printf("\n");

    const string base_dir = "../Data/parsed/";
    const int num_queries = 1000;

    // random query parameters
    const int size = 2; // size will be about between [size, size * 2]
    const int not_prob = 5;
    const int and_prob = 30;

    // get list of docs for test
    vector<string> doc_paths;
    client.listTxtFiles(base_dir, doc_paths);

    ////////////////////////////////////////////////////////////////////////////
    // UPDATE //////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    // add documents from the directory
    set<string> all_words_set; // for client-side random query generation only
    for(string doc : doc_paths){
        set<string> text = client.extractUniqueKeywords(base_dir + doc);

        // generate the byte* to send to the server
        char* data;
        int data_size = client.add_new_document(text, &data);

        // int SseIee::f(char* data, int data_size, char* output)
        output_size = 0;//iee.f(data, data_size, &output);
        //f(&output, &output_size, 0, data, data_size);

        // add all new words to a set, used later to generate queries
        all_words_set.insert(text.begin(), text.end());
    }

    ////////////////////////////////////////////////////////////////////////////
    // QUERIES /////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////

    // generate random queries
    vector<string> all_words(all_words_set.size());
    copy(all_words_set.begin(), all_words_set.end(), all_words.begin());

    for(unsigned i = 0; i < num_queries; i++) {
        string query = client.generate_random_query(all_words, size, not_prob, and_prob);

        #ifdef VERBOSE
        cout << "\n----------------------------\nQuery: " << query << endl;
        #endif

        char* data;
        int data_size = client.search(query, &data);

        #ifdef VERBOSE
        printf("\n***** Entering IEE *****\n");
        #endif

        // int SseIee::f(char* data, int data_size, char* output)
        output_size = 0;//iee.f(data, data_size, &output);
        //f(&output, &output_size, 0, data, data_size);

        #ifdef VERBOSE
        printf("\n***** Leaving IEE *****\n");
        #endif

        //process results
        const int nDocs = (*output_size) / sizeof(int);

        #ifdef VERBOSE
        printf("\n***** CLIENT RESULT *****\n");
        printf("Number of docs: %d\n", nDocs);
        #endif

        vector<int> results(nDocs);
        int pos = 0;
        for (int i = 0; i < nDocs; i++) {
            results[i] = readIntFromArr((char*)output, &pos);
        }

        printResults(results);
    }

    //for (auto const& x : f)
    //    cout << x.first << ':' << x.second << endl;

    return 0;
}
