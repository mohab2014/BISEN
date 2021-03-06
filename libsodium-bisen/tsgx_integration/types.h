#ifndef __TYPES__
#define __TYPES__

typedef unsigned char byte;
typedef unsigned char *bytes;
typedef unsigned long long size;
typedef unsigned long long label;

void f(
  bytes *out,
  size *outlen,
  const label pid,
  const bytes in,
  const size inlen
);

void fserver(
  bytes *out,
  size *outlen,
  const bytes in,
  const size inlen
);

#endif 
