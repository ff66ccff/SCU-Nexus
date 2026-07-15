#include <QtTest>

#include <QDate>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace {

struct ExpectedCalendar {
    QString publishedAt;
    QString pagePath;
    QString fallImageUrl;
    QString springImageUrl;
};

const QMap<QString, ExpectedCalendar> expectedCalendars{
    {QStringLiteral("2021-2022"),
     {QStringLiteral("2021-05-24"),
      QStringLiteral("/info/1101/8144.htm"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/6/1F/6E/7DA367AD1C7E42F35A667442707_3ABFA264_2D4EA.jpg"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/0/89/36/717B557D6086EE645232F51A95B_377ECBF0_33C38.jpg")}},
    {QStringLiteral("2022-2023"),
     {QStringLiteral("2022-06-27"),
      QStringLiteral("/info/1101/8152.htm"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/3/43/DF/C3DB35A3CE91BEA5EA302498E96_77214235_26338.jpg"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/6/93/58/4EA55A7E31E7A2361B77CD89A44_7A765565_28F3C.jpg")}},
    {QStringLiteral("2023-2024"),
     {QStringLiteral("2023-06-20"),
      QStringLiteral("/info/1101/8147.htm"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/4/6C/49/5D877DC7D401583A5709DD65F0A_FFFB93EF_F483.png"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/2/7B/28/B542C13B4BC14E7ED8F51AEE225_1B06AC08_100DB.png")}},
    {QStringLiteral("2024-2025"),
     {QStringLiteral("2024-07-05"),
      QStringLiteral("/info/1101/9308.htm"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/6/D6/09/B059784043EE513BBA4A21C555B_A404354A_5FD8E.jpg"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/1/3C/E9/9ECD9920A088D712D67111E195F_8DED01A4_2CECA.png")}},
    {QStringLiteral("2025-2026"),
     {QStringLiteral("2025-06-24"),
      QStringLiteral("/info/1101/10035.htm"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/E/AF/7B/0B6F0E05CBAFA093563D4B80D98_063B2760_3FA13.jpg"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/9/B0/79/25F4792D1F01ABE8C1A959C6B3C_6253CD73_39F95.jpg")}},
    {QStringLiteral("2026-2027"),
     {QStringLiteral("2026-07-08"),
      QStringLiteral("/info/1101/10727.htm"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/8/E7/12/2F3548E5719628BB4141F3846D0_CDB03031_480AA.png"),
      QStringLiteral("https://jwc.scu.edu.cn/__local/5/8A/8E/E91209EBD27E116771B161CCEBF_8F5B0F08_464B9.png")}},
};

QDate isoDate(const QJsonObject &object, const char *key)
{
    const QString value = object.value(QLatin1String(key)).toString();
    return QDate::fromString(value, Qt::ISODate);
}

} // namespace

class CalendarDataTests final : public QObject
{
    Q_OBJECT

private slots:
    void resourceMatchesPublishedCalendarCorpus()
    {
        QFile file(QStringLiteral(SCU_NEXUS_SOURCE_DIR
                                  "/resources/calendar/academic_calendars.json"));
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        QCOMPARE(parseError.error, QJsonParseError::NoError);
        QVERIFY(document.isObject());

        const QJsonObject root = document.object();
        QCOMPARE(root.value(QStringLiteral("schemaVersion")).toInt(), 1);
        QCOMPARE(root.value(QStringLiteral("sourceIndexUrl")).toString(),
                 QStringLiteral("https://jwc.scu.edu.cn/cdxl.htm"));
        QCOMPARE(root.value(QStringLiteral("capturedAt")).toString(),
                 QStringLiteral("2026-07-15"));

        const QJsonArray calendars = root.value(QStringLiteral("calendars")).toArray();
        QCOMPARE(calendars.size(), 6);

        QSet<QString> seenYears;
        QSet<QString> termNames;
        int termCount = 0;

        for (const QJsonValue &calendarValue : calendars) {
            QVERIFY(calendarValue.isObject());
            const QJsonObject calendar = calendarValue.toObject();
            const QString year = calendar.value(QStringLiteral("academicYear")).toString();
            QVERIFY2(expectedCalendars.contains(year), qPrintable(year));
            QVERIFY2(!seenYears.contains(year), qPrintable(year));
            seenYears.insert(year);

            const ExpectedCalendar expected = expectedCalendars.value(year);
            QCOMPARE(calendar.value(QStringLiteral("publishedAt")).toString(),
                     expected.publishedAt);
            QCOMPARE(calendar.value(QStringLiteral("sourcePagePath")).toString(),
                     expected.pagePath);
            QVERIFY(!calendar.value(QStringLiteral("title")).toString().trimmed().isEmpty());

            const QJsonArray terms = calendar.value(QStringLiteral("terms")).toArray();
            QCOMPARE(terms.size(), 2);
            QSet<QString> yearTermNames;

            for (const QJsonValue &termValue : terms) {
                QVERIFY(termValue.isObject());
                const QJsonObject term = termValue.toObject();
                const QString termName = term.value(QStringLiteral("name")).toString();
                QVERIFY(termName == QStringLiteral("秋季学期")
                        || termName == QStringLiteral("春季学期"));
                QVERIFY(!yearTermNames.contains(termName));
                yearTermNames.insert(termName);
                termNames.insert(termName);
                ++termCount;

                const QString expectedImage = termName == QStringLiteral("秋季学期")
                    ? expected.fallImageUrl
                    : expected.springImageUrl;
                QCOMPARE(term.value(QStringLiteral("sourceImageUrl")).toString(), expectedImage);
                QVERIFY(!term.value(QStringLiteral("id")).toString().trimmed().isEmpty());

                const QDate termStart = isoDate(term, "startDate");
                const QDate termEnd = isoDate(term, "endDate");
                QVERIFY(termStart.isValid());
                QVERIFY(termEnd.isValid());
                QVERIFY(termStart <= termEnd);

                const QJsonArray weeks = term.value(QStringLiteral("weeks")).toArray();
                QVERIFY2(weeks.size() >= 20, qPrintable(term.value("id").toString()));
                QSet<int> weekNumbers;
                for (const QJsonValue &weekValue : weeks) {
                    QVERIFY(weekValue.isObject());
                    const QJsonObject week = weekValue.toObject();
                    const int weekNo = week.value(QStringLiteral("weekNo")).toInt();
                    QVERIFY(weekNo > 0);
                    QVERIFY(!weekNumbers.contains(weekNo));
                    weekNumbers.insert(weekNo);
                    const QDate start = isoDate(week, "startDate");
                    const QDate end = isoDate(week, "endDate");
                    QVERIFY(start.isValid());
                    QVERIFY(end.isValid());
                    QCOMPARE(start.daysTo(end), 6);
                    QVERIFY(!week.value(QStringLiteral("phase")).toString().trimmed().isEmpty());
                    QVERIFY(!week.value(QStringLiteral("label")).toString().trimmed().isEmpty());
                }

                const QJsonArray events = term.value(QStringLiteral("events")).toArray();
                QVERIFY(!events.isEmpty());
                for (const QJsonValue &eventValue : events) {
                    QVERIFY(eventValue.isObject());
                    const QJsonObject event = eventValue.toObject();
                    QVERIFY(!event.value(QStringLiteral("title")).toString().trimmed().isEmpty());
                    const QDate start = isoDate(event, "startDate");
                    const QDate end = isoDate(event, "endDate");
                    QVERIFY(start.isValid());
                    QVERIFY(end.isValid());
                    QVERIFY(start <= end);
                }

                const QJsonArray notes = term.value(QStringLiteral("notes")).toArray();
                QVERIFY(!notes.isEmpty());
                int expectedNumber = 1;
                for (const QJsonValue &noteValue : notes) {
                    QVERIFY(noteValue.isObject());
                    const QJsonObject note = noteValue.toObject();
                    QCOMPARE(note.value(QStringLiteral("number")).toInt(), expectedNumber++);
                    QVERIFY(!note.value(QStringLiteral("text")).toString().trimmed().isEmpty());
                }
            }

            QCOMPARE(yearTermNames,
                     QSet<QString>({QStringLiteral("秋季学期"), QStringLiteral("春季学期")}));
        }

        QCOMPARE(seenYears,
                 QSet<QString>({QStringLiteral("2021-2022"), QStringLiteral("2022-2023"),
                                QStringLiteral("2023-2024"), QStringLiteral("2024-2025"),
                                QStringLiteral("2025-2026"), QStringLiteral("2026-2027")}));
        QCOMPARE(termCount, 12);
        QCOMPARE(termNames,
                 QSet<QString>({QStringLiteral("秋季学期"), QStringLiteral("春季学期")}));
    }
};

QTEST_APPLESS_MAIN(CalendarDataTests)

#include "test_calendar_data.moc"
