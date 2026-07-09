#ifndef WEEKTYPE_H
#define WEEKTYPE_H

#include <QString>

namespace SCUNexus {

enum class WeekType {
    Every = 0,
    Odd = 1,
    Even = 2
};

// Helper to convert between enum and int for DB storage
inline int weekTypeToInt(WeekType wt) {
    return static_cast<int>(wt);
}

inline WeekType intToWeekType(int val) {
    switch (val) {
    case 1: return WeekType::Odd;
    case 2: return WeekType::Even;
    default: return WeekType::Every;
    }
}

inline QString weekTypeToString(WeekType wt) {
    switch (wt) {
    case WeekType::Odd: return QStringLiteral("单周");
    case WeekType::Even: return QStringLiteral("双周");
    default: return QStringLiteral("每周");
    }
}

} // namespace SCUNexus

#endif // WEEKTYPE_H
