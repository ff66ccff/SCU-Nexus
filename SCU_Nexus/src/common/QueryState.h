#pragma once

#include <QObject>

class QueryStateNamespace
{
    Q_GADGET

public:
    enum QueryState {
        Idle,
        Loading,
        Loaded,
        Empty,
        Error,
        LoginRequired
    };
    Q_ENUM(QueryState)
};

using QueryState = QueryStateNamespace::QueryState;
