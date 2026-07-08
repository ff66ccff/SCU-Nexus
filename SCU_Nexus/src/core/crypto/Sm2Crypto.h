#ifndef SM2CRYPTO_H
#define SM2CRYPTO_H

#include <QString>

class Sm2Crypto
{
public:
    static QString encryptWithBase64Key(const QString& plaintext, const QString& publicKeyBase64);
};

#endif // SM2CRYPTO_H
