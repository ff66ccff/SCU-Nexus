#pragma once

#include <QObject>

class QueryStateNamespace
{
    Q_GADGET

public:
    enum QueryState {
        Idle = 0,
        Loading = 1,
        Loaded = 2,
        Refreshing = 3,
        Empty = 4,
        Error = 5,
        LoginRequired = 6
    };
    Q_ENUM(QueryState)
};

using QueryState = QueryStateNamespace::QueryState;
