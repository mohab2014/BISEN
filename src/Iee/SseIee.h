//
//  SseIee.h
//  BooleanSSE
//
//  Created by Bernardo Ferreira on 27/03/17.
//  Copyright © 2017 Bernardo Ferreira. All rights reserved.
//

#ifndef SSE_IEE_H
#define SSE_IEE_H

#include <stdio.h>
#include <stdlib.h>

#include "util/IeeUtils.h"
#include "util/vec_token.h"
#include "IeeCrypt.h"
#include "QueryEvaluator.h"
#include "types.h" // mpc data types

#define OP_SETUP 0x69
#define OP_ADD 0x61
#define OP_SRC 0x73
#define OP_BENCH 0x5 // only for benchmarking, prints stats

#define RES_OK 0x90

#define MAX_BATCH_UPDATE 1000
#define MAX_BATCH_SEARCH 2000

int server_socket;

static void init_pipes();
static void destroy_pipes();
static void benchmarking_print(); // only for benchmarking, prints stats

extern void f(bytes* out, size* out_len, const unsigned long long pid, const bytes in, const size in_len);

static void setup(bytes* out, size* out_len, const bytes in, const size in_len);
static void add(bytes* out, size* out_len, const bytes in, const size in_len);
static void search(bytes* out, size* out_len, const bytes in, const size in_len);

static void get_docs_from_server(vec_token *query, const unsigned count_words, const unsigned total_labels);
#endif /* SSE_IEE_H */
