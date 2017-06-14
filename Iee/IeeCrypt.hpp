//
//  IeeCrypt.hpp
//  BooleanSSE
//
//  Created by Bernardo Ferreira on 29/03/17.
//  Copyright © 2017 Bernardo Ferreira. All rights reserved.
//

#ifndef IeeCrypt_hpp
#define IeeCrypt_hpp

#include <stdio.h>
#include <vector>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include "IeeUtils.h"

using namespace std;

class IeeCrypt {
    
private:
    RSA* IeePrivK;
    unsigned char* kCom;
    unsigned char* kEnc;
    unsigned char* kF;
    
    void spc_rand(unsigned char *buf, int l);
    
public:
    static const int symKsize = 16;
    static const int fKsize = 20;
    
    IeeCrypt();
    ~IeeCrypt();
    void initKeys();
    void storeKcom(vector<unsigned char> key);
    bool hasStoredKcom();
    vector<unsigned char> decryptPublic (unsigned char* data, int size);
    int encryptSymmetric (unsigned char* data, int size, unsigned char* ciphertext);
    int decryptSymmetric (unsigned char* plaintext, unsigned char* ciphertext, int ciphertextSize);
    void f (unsigned char* key, unsigned char* data, int dataSize, unsigned char* md);
    unsigned char* get_kF();
};



#endif /* IeeCrypt_hpp */
