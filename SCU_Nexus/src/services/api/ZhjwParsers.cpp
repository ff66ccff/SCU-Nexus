#include "ZhjwParsers.h"

#include <QRegularExpression>

namespace {

// 去除 HTML 标签并规整空白字符。
QString stripTags(QString text)
{
    text.remove(QRegularExpression("<[^>]+>"));
    text.replace("&nbsp;", " ");
    text.replace("&amp;", "&");
    text.replace("&lt;", "<");
    text.replace("&gt;", ">");
    return text.simplified();
}

// 返回正则表达式的第一个捕获结果。
QString firstCapture(const QString& text, const QString& pattern)
{
    const QRegularExpression regex(pattern, QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = regex.match(text);
    return match.hasMatch() ? stripTags(match.captured(1)) : QString();
}

// 从文本中提取指定标签后的值。
QString labeledValue(const QString& text, const QString& label)
{
    return firstCapture(text, label + R"(\s*[:：]\s*(?:&nbsp;|\s)*([^<]+))");
}

// 从 HTML 中提取指定后缀的回调路径。
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

// 判断条件是否成立并返回布尔结果。
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

// 解析外部数据并转换为内部结构。
int parseCurrentWeek(const QString& html)
{
    const QRegularExpression regex(QStringLiteral("第(\\d+)周"));
    const QRegularExpressionMatch match = regex.match(html);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

// 解析外部数据并转换为内部结构。
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

// 解析考表条目，并区分页面结构、明确空结果和无法识别的页面。
ExamPlanParseResult parseExamPlanResult(const QString& html)
{
    ExamPlanParseResult result;
    result.explicitlyEmpty = html.contains(QStringLiteral("暂无考试安排"))
        || html.contains(QStringLiteral("暂无数据"))
        || html.contains(QStringLiteral("没有查询到"));

    const QRegularExpression blockRegex(
        R"((<div[^>]+class\s*=\s*"[^"]*widget-box\s+widget-color-[^"]*"[^>]*>.*?)(?=<div[^>]+class\s*=\s*"[^"]*widget-box\s+widget-color-|$))",
        QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator blocks = blockRegex.globalMatch(html);
    while (blocks.hasNext()) {
        result.recognized = true;
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
            result.items.append(item);
        }
    }
    return result;
}

// 保留旧调用接口，仅返回成功解析出的考表条目。
QList<ExamPlanItemDto> parseExamPlan(const QString& html)
{
    return parseExamPlanResult(html).items;
}

// 提取培养方案成绩接口回调路径。
QString extractSchemeScoresCallback(const QString& html)
{
    return extractCallback(html, QStringLiteral("schemeScores"));
}

// 提取及格成绩接口回调路径。
QString extractPassingScoresCallback(const QString& html)
{
    return extractCallback(html, QStringLiteral("allPassingScores"));
}

}
