#include "Sm2Crypto.h"

#include <QByteArray>

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>

#include <memory>

namespace {

struct OpenSslDeleter
{
    void operator()(EC_KEY* key) const { EC_KEY_free(key); }
    void operator()(EC_POINT* point) const { EC_POINT_free(point); }
    void operator()(EVP_PKEY* key) const { EVP_PKEY_free(key); }
    void operator()(EVP_PKEY_CTX* ctx) const { EVP_PKEY_CTX_free(ctx); }
};

using EcKeyPtr = std::unique_ptr<EC_KEY, OpenSslDeleter>;
using EcPointPtr = std::unique_ptr<EC_POINT, OpenSslDeleter>;
using EvpKeyPtr = std::unique_ptr<EVP_PKEY, OpenSslDeleter>;
using EvpCtxPtr = std::unique_ptr<EVP_PKEY_CTX, OpenSslDeleter>;

QString opensslError()
{
    const unsigned long code = ERR_get_error();
    if (code == 0) {
        return QStringLiteral("OpenSSL SM2 加密失败");
    }
    char buffer[256] = {};
    ERR_error_string_n(code, buffer, sizeof(buffer));
    return QString::fromLatin1(buffer);
}

bool readDerLength(const QByteArray& der, qsizetype* offset, qsizetype* length)
{
    if (*offset >= der.size()) {
        return false;
    }
    const unsigned char first = static_cast<unsigned char>(der.at((*offset)++));
    if ((first & 0x80) == 0) {
        *length = first;
        return true;
    }
    const int count = first & 0x7f;
    if (count <= 0 || count > 4 || *offset + count > der.size()) {
        return false;
    }
    qsizetype value = 0;
    for (int i = 0; i < count; ++i) {
        value = (value << 8) | static_cast<unsigned char>(der.at((*offset)++));
    }
    *length = value;
    return true;
}

QByteArray readDerValue(const QByteArray& der, qsizetype* offset, unsigned char expectedTag)
{
    if (*offset >= der.size() || static_cast<unsigned char>(der.at((*offset)++)) != expectedTag) {
        return {};
    }
    qsizetype length = 0;
    if (!readDerLength(der, offset, &length) || length < 0 || *offset + length > der.size()) {
        return {};
    }
    const QByteArray value = der.mid(*offset, length);
    *offset += length;
    return value;
}

QByteArray fixedInteger32(QByteArray value)
{
    while (value.size() > 32 && value.at(0) == '\0') {
        value.remove(0, 1);
    }
    if (value.size() > 32) {
        return {};
    }
    if (value.size() < 32) {
        value.prepend(QByteArray(32 - value.size(), '\0'));
    }
    return value;
}

QByteArray sm2DerToC1C2C3(const QByteArray& der)
{
    qsizetype offset = 0;
    if (offset >= der.size() || static_cast<unsigned char>(der.at(offset++)) != 0x30) {
        return {};
    }
    qsizetype sequenceLength = 0;
    if (!readDerLength(der, &offset, &sequenceLength) || offset + sequenceLength > der.size()) {
        return {};
    }

    const QByteArray x = fixedInteger32(readDerValue(der, &offset, 0x02));
    const QByteArray y = fixedInteger32(readDerValue(der, &offset, 0x02));
    const QByteArray c3 = readDerValue(der, &offset, 0x04);
    const QByteArray c2 = readDerValue(der, &offset, 0x04);
    if (x.size() != 32 || y.size() != 32 || c2.isEmpty() || c3.size() != 32) {
        return {};
    }

    QByteArray raw;
    raw.reserve(1 + x.size() + y.size() + c2.size() + c3.size());
    raw.append(char(0x04));
    raw.append(x);
    raw.append(y);
    raw.append(c2);
    raw.append(c3);
    return raw;
}

}

QString Sm2Crypto::encryptWithBase64Key(const QString& plaintext, const QString& publicKeyBase64)
{
    QByteArray publicKey = QByteArray::fromBase64(publicKeyBase64.toUtf8());
    if (publicKey.size() == 64) {
        publicKey.prepend(char(0x04));
    }
    if (publicKey.size() != 65 || static_cast<unsigned char>(publicKey.at(0)) != 0x04) {
        return {};
    }

    EcKeyPtr ecKey(EC_KEY_new_by_curve_name(NID_sm2));
    if (!ecKey) {
        return {};
    }
    const EC_GROUP* group = EC_KEY_get0_group(ecKey.get());
    EcPointPtr point(EC_POINT_new(group));
    if (!point || EC_POINT_oct2point(group,
                                     point.get(),
                                     reinterpret_cast<const unsigned char*>(publicKey.constData()),
                                     publicKey.size(),
                                     nullptr) != 1) {
        return {};
    }
    if (EC_KEY_set_public_key(ecKey.get(), point.get()) != 1) {
        return {};
    }

    EvpKeyPtr evpKey(EVP_PKEY_new());
    if (!evpKey || EVP_PKEY_assign_EC_KEY(evpKey.get(), ecKey.release()) != 1) {
        return {};
    }
    EVP_PKEY_set_alias_type(evpKey.get(), EVP_PKEY_SM2);

    EvpCtxPtr ctx(EVP_PKEY_CTX_new(evpKey.get(), nullptr));
    if (!ctx || EVP_PKEY_encrypt_init(ctx.get()) != 1) {
        Q_UNUSED(opensslError())
        return {};
    }

    const QByteArray plain = plaintext.toUtf8();
    size_t encryptedLength = 0;
    if (EVP_PKEY_encrypt(ctx.get(),
                         nullptr,
                         &encryptedLength,
                         reinterpret_cast<const unsigned char*>(plain.constData()),
                         plain.size()) != 1) {
        return {};
    }

    QByteArray der;
    der.resize(static_cast<int>(encryptedLength));
    if (EVP_PKEY_encrypt(ctx.get(),
                         reinterpret_cast<unsigned char*>(der.data()),
                         &encryptedLength,
                         reinterpret_cast<const unsigned char*>(plain.constData()),
                         plain.size()) != 1) {
        return {};
    }
    der.resize(static_cast<int>(encryptedLength));

    const QByteArray raw = sm2DerToC1C2C3(der);
    return raw.isEmpty() ? QString() : QString::fromLatin1(raw.toBase64());
}
