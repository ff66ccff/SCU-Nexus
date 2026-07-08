#include "ZhjwParsers.h"

#include <QRegularExpression>

namespace {

QString stripTags(QString text)
{
    text.remove(QRegularExpression("<[^>]+>"));
    text.replace("&nbsp;", " ");
    text.replace("&amp;", "&");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    return text.simplified();
}

QString firstCapture(const QString& text, const QString& pattern)
{
    const QRegularExpression regex(pattern, QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = regex.match(text);
    return match.hasMatch() ? stripTags(match.captured(1)) : QString();
}

QString labeledValue(const QString& text, const QString& label)
{
    return firstCapture(text, label + R"(\s*:\s*(?:&nbsp;|\s)*([^<]+))");
}

QString extractCallback(const QString& html, const QString& suffix)
{
    const QRegularExpression regex(
        QStringLiteral("var\\s+url\\s*=\\s*\"(/student/integratedQuery/scoreQuery/[^/]+/%1/callback)\"")
            .arg(suffix));
    const QRegularExpressionMatch match = regex.match(html);
    return match.hasMatch() ? match.captured(1) : QString();
}

}

namespace ZhjwParsers {

bool isSessionExpired(const QString& body, int statusCode)
{
    const QString trimmed = body.trimmed();
    if (statusCode == 302 || trimmed.isEmpty()) {
        return true;
    }
    if (trimmed.startsWith('<') && trimmed.contains("login", Qt::CaseInsensitive)) {
        return true;
    }
    return trimmed.contains("统一身份认证") || trimmed.contains("用户登录");
}

int parseCurrentWeek(const QString& html)
{
    const QRegularExpression regex(QStringLiteral("第(\\d+)周"));
    const QRegularExpressionMatch match = regex.match(html);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

QList<SemesterDto> parseSemesters(const QString& html)
{
    QList<SemesterDto> semesters;
    const QRegularExpression regex(
        QStringLiteral("<option[^>]+value\\s*=\\s*\"([^\"]+)\"[^>]*>(.*?)</option>"),
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator matches = regex.globalMatch(html);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        semesters.append({match.captured(1).trimmed(), stripTags(match.captured(2))});
    }
    return semesters;
}

QList<ExamPlanItemDto> parseExamPlan(const QString& html)
{
    QList<ExamPlanItemDto> items;
    const QRegularExpression blockRegex(
        R"((<div[^>]+class\s*=\s*"[^"]*widget-box\s+widget-color-[^"]*"[^>]*>.*?)(?=<div[^>]+class\s*=\s*"[^"]*widget-box\s+widget-color-|$))",
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator blocks = blockRegex.globalMatch(html);
    while (blocks.hasNext()) {
        const QString block = blocks.next().captured(1);
        ExamPlanItemDto item;
        item.courseName = firstCapture(
            block,
            QStringLiteral("<h5[^>]*class\\s*=\\s*\"[^\"]*widget-title\\s+smaller[^\"]*\"[^>]*>(.*?)</h5>"));
        item.courseName.remove(QStringLiteral("（已结束）"));
        item.courseName = item.courseName.trimmed();
        item.week = firstCapture(block, QStringLiteral("第(\\d+)周"));
        item.date = firstCapture(block, QStringLiteral("(\\d{4}-\\d{2}-\\d{2})"));
        item.weekday = firstCapture(block, QStringLiteral("(星期[一二三四五六日])"));
        item.timeRange = firstCapture(block, QStringLiteral("(\\d{2}:\\d{2}-\\d{2}:\\d{2})"));
        item.location = labeledValue(block, QStringLiteral("地点"));
        item.seatNumber = labeledValue(block, QStringLiteral("座位号"));
        item.ticketNumber = labeledValue(block, QStringLiteral("准考证号"));
        item.tip = labeledValue(block, QStringLiteral("考试提示信息"));
        if (item.tip.isEmpty()) {
            item.tip = QStringLiteral("无");
        }
        if (!item.courseName.isEmpty()) {
            items.append(item);
        }
    }
    return items;
}

QString extractSchemeScoresCallback(const QString& html)
{
    return extractCallback(html, QStringLiteral("schemeScores"));
}

QString extractPassingScoresCallback(const QString& html)
{
    return extractCallback(html, QStringLiteral("allPassingScores"));
}

}
