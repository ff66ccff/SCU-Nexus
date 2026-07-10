#ifndef ZHJWPARSERS_H
#define ZHJWPARSERS_H

#include "src/services/api/ApiDtos.h"

#include <QList>
#include <QString>

namespace ZhjwParsers {

struct ExamPlanParseResult {
    QList<ExamPlanItemDto> items;
    bool recognized = false;
    bool explicitlyEmpty = false;
};

bool isSessionExpired(const QString& body, int statusCode);
int parseCurrentWeek(const QString& html);
QList<SemesterDto> parseSemesters(const QString& html);
ExamPlanParseResult parseExamPlanResult(const QString& html);
QList<ExamPlanItemDto> parseExamPlan(const QString& html);
QString extractSchemeScoresCallback(const QString& html);
QString extractPassingScoresCallback(const QString& html);

}

#endif // ZHJWPARSERS_H
