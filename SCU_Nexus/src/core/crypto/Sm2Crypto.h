#ifndef SM2CRYPTO_H
#define SM2CRYPTO_H

#include <QString>

// 四川大学统一认证登录所需的 SM2 密码加密工具。
// 输入是服务端返回的 Base64 非压缩公钥，输出是 Base64(C1C2C3)；该排列与
// OpenSSL 默认的 DER 密文不同，不能直接把 EVP_PKEY_encrypt 的结果提交给接口。
class Sm2Crypto
{
public:
    static QString encryptWithBase64Key(const QString& plaintext, const QString& publicKeyBase64);
};

#endif // SM2CRYPTO_H
