#ifndef NETWORKSETTINGS_H
#define NETWORKSETTINGS_H

#include <QString>

namespace NetworkSettings {

inline constexpr int kDefaultTimeoutMs = 15000;
inline constexpr int kMaxRedirects = 10;
// 所有校园站点请求共用同一浏览器 UA，避免不同模块各自复制并逐渐失配。
inline const QString kDefaultUserAgent =
    QStringLiteral("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                   "(KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36 Edg/131.0.0.0");

}

#endif // NETWORKSETTINGS_H
