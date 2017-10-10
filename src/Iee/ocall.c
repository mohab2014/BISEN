#include "ocall.h"

void ocall_printf(const char *fmt, ...)
{
    //TODO do something
    /*#define BUFSIZ 512
    char buf[BUFSIZ+1] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf+1, BUFSIZ+1, fmt, ap);
    va_end(ap);

    // op code
    buf[0] = 0x70;

    unsigned char * output;
    unsigned long long output_size;
    fserver(&output, &output_size, buf, BUFSIZ+1);*/
}

int ocall_open(const char *path, int oflags)
{
    size_t path_len = iee_strlen(path);

    size_t inlen = sizeof(int) + (path_len * sizeof(char));
    unsigned char* in = (unsigned char*)malloc(inlen);
    in[0] = OCALL_OPEN;

    ocall_printf("before path %lu %s %d\n", path_len, path, oflags);
    int pos = 1;
    iee_addIntToArr(oflags, in, &pos);
    iee_addToArr((const void *)path, path_len, in, &pos);

    // execute ocall
    unsigned char * out;
    unsigned long long outlen;
    fserver(&out, &outlen, in, inlen);

    // read ocall output
    pos = 0;
    int ocall_ret = iee_readIntFromArr(out, &pos);
    ocall_printf("ret ocall: %d\n", ocall_ret);

    return ocall_ret;
}

int ocall_close(int fildes)
{
    size_t inlen = sizeof(int);
    unsigned char* in = (unsigned char*)malloc(inlen);
    in[0] = OCALL_CLOSE;

    int pos = 1;
    iee_addIntToArr(fildes, in, &pos);

    // execute ocall
    unsigned char * out;
    unsigned long long outlen;
    fserver(&out, &outlen, in, inlen);

    // read ocall output
    pos = 0;
    int ocall_ret = iee_readIntFromArr(out, &pos);
    ocall_printf("ret close ocall: %d\n", ocall_ret);

    return ocall_ret;
}

ssize_t ocall_read(int fildes, unsigned char* buf, size_t nbytes)
{
    size_t inlen = sizeof(unsigned char) + sizeof(int) + sizeof(size_t);
    unsigned char* in = (unsigned char*)malloc(inlen);
    in[0] = OCALL_READ;

    int pos = 1;
    iee_addIntToArr(fildes, in, &pos);
    iee_add_size_t(nbytes, in, &pos);
    ocall_printf("nbytes pre ocall: %lu\n", nbytes);

    // execute ocall, which returns the out buffer, whose data
    // we copy to the original buf, allocated outside of this call
    unsigned char * out;
    unsigned long long outlen;
    fserver(&out, &outlen, in, inlen);

    // read ocall output
    pos = 0;
    ssize_t ocall_ret = iee_read_ssize_t(out, &pos);
    ocall_printf("ret ocall: %lu\n", ocall_ret);

    ocall_printf("%ld\n", ocall_ret);
    
    for(int i = 0; i < ocall_ret; i++)
        buf[i] = (void*)out[i+8];

    return ocall_ret;
}

ssize_t ocall_write(int fildes, const void *buf, size_t nbytes)
{
    size_t buf_size = sizeof(unsigned char) + sizeof(int) + sizeof(size_t) + nbytes * sizeof(unsigned char);
    unsigned char* buffer = (unsigned char*)malloc(buf_size);
    buffer[0] = OCALL_WRITE;
    ocall_printf("ocall fildes write is %d\n", fildes);
    int pos = 1;
    iee_addIntToArr(fildes, buffer, &pos);
    iee_add_size_t(nbytes, buffer, &pos);
    iee_addToArr((const void *)buf, nbytes, buffer, &pos);

    // execute ocall
    unsigned char * output;
    unsigned long long output_size;
    fserver(&output, &output_size, buffer, buf_size);

    // read ocall output
    pos = 0;
    ssize_t ocall_ret = iee_read_ssize_t(output, &pos);
    //ocall_printf("ret: %lu\n", ocall_ret);

    free(buffer);
    free(output);
    return ocall_ret;
}
