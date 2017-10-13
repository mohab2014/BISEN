#include "commands.h"

void generate_commands()
{
    // init output file
    FILE *in_f = fopen("test_all","rb");
    if (!in_f) {
		printf("Error opening test file!\n");
		exit(-1);
	}

    // get file size
    size_t f_size = get_file_size(in_f);
    printf("File size: %lu\n", f_size);

    // count number of queries
    test_len = count_queries_file(in_f, f_size);
    printf("Number of queries to execute: %llu\n", test_len);

    // prepare buffers to hold queries
    commands = (unsigned char**)malloc(test_len * sizeof(unsigned char*));
    commands_sizes = (unsigned long long*)malloc(test_len * sizeof(unsigned long long));

    // traverse the file to read the commands into the buffers
    unsigned current_query = 0;
    rewind(in_f);

    for(unsigned current_query = 0; current_query < test_len; current_query++) {
        if(fread(commands_sizes + current_query, sizeof(size), 1, in_f) != 1) {
            printf("Error reading file!\n");
            exit(-1);
        }

        // allocate the query buffer
        commands[current_query] = (unsigned char*)malloc(commands_sizes[current_query] * sizeof(unsigned char*));

        if(fread(commands[current_query], commands_sizes[current_query], 1, in_f) != 1) {
            printf("Error reading file!\n");
            exit(-1);
        }
    }

    /*for(int i = 0; i < test_len; i++)
        printf("%d\n", commands_sizes[i]);
    printf("\n");*/

    printf("File totally read!\n");
    fclose(in_f);

    /*free(commands_sizes);
    for(int i = 0; i < test_len; i++)
        free(commands[i]);

    free(commands);*/
}

size count_queries_file(FILE *f, size_t f_size)
{
    size num_queries = 0;
    size_t pos = 0;
    rewind(f);

    while(pos < f_size) {
        size query_size = 0;
        if(fread(&query_size, sizeof(size), 1, f) != 1) {
            printf("Error reading file!\n");
            exit(-1);
        }

        // ignore the query
        fseek(f, query_size, SEEK_CUR);
        num_queries++;

        // advance position
        pos += sizeof(size) + query_size;
    }

    rewind(f);
    return num_queries;
}

size_t get_file_size(FILE *f)
{
    fseek(f, 0, SEEK_END);
    size_t in_size = ftell(f);
    rewind(f);

    return in_size;
}

/*
size_t read_all(FILE *f, unsigned char** buffer)
{
    // get file size
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    // allocate buffer to read the file into
    *buffer = (unsigned char*)malloc(sizeof(unsigned char*) * size);
    size_t pos = 0;
    while(pos < size) {
        pos += fread(*buffer, sizeof(size), size, f);
    }

    return size;
}*/
