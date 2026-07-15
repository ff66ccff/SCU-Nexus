#include "ZhjwParsers.h"

#include <QRegularExpression>

namespace {

// 教务页面不是严格 XML；这里只做字段级轻量清洗，不承担通用 HTML 解析。
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

// 从文本中提取指定标签后的值。
QString labeledValue(const QString& text, const QString& label)
{
    return firstCapture(text, label + R"(\s*[:：]\s*(?:&nbsp;|\s)*([^<]+))");
}

// callback 中间段由服务端动态生成，只约束固定前缀和业务后缀。
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

// 教务会话过期没有统一响应格式：重定向、空白 200、英文 login 页和中文统一认证页都可能出现。
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

// 未匹配时返回 0，由服务层统一映射为 ParseFailed，而不是猜测教学周。
int parseCurrentWeek(const QString& html)
{
    const QRegularExpression regex(QStringLiteral("第(\\d+)周"));
    const QRegularExpressionMatch match = regex.match(html);
    return match.hasMatch() ? match.captured(1).toInt() : 0;
}

// 保持服务端 option 顺序和 label 原文；当前学期标记由课表导入层处理。
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

// 解析考表条目，并区分“确实无考试”和“页面改版/返回了错误页面”。
ExamPlanParseResult parseExamPlanResult(const QString& html)
{
    ExamPlanParseResult result;
    result.explicitlyEmpty = html.contains(QStringLiteral("暂无考试安排"))
        || html.contains(QStringLiteral("暂无数据"))
        || html.contains(QStringLiteral("没有查询到"));

    // 以相邻 widget-box 为卡片边界，避免一个全局正则跨卡片串联字段。
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

// 保留旧调用接口；需要区分空态与解析失败的新代码应使用 parseExamPlanResult。
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
