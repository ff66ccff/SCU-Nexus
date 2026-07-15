#include <QtTest>

#include <QCryptographicHash>
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

struct ExpectedTerm {
    QString id;
    QString name;
    QString startDate;
    QString endDate;
    int weekCount;
    int eventCount;
    int noteCount;
    QString phaseRuns;
    QByteArray weekHash;
    QByteArray eventHash;
    QByteArray noteHash;
};

const QStringList expectedCalendarOrder{
    QStringLiteral("2026-2027"), QStringLiteral("2025-2026"),
    QStringLiteral("2024-2025"), QStringLiteral("2023-2024"),
    QStringLiteral("2022-2023"), QStringLiteral("2021-2022")};

const QList<ExpectedTerm> expectedTerms{
    {QStringLiteral("2026-fall"), QStringLiteral("秋季学期"),
     QStringLiteral("2026-08-27"), QStringLiteral("2027-02-27"), 26, 13, 11,
     QStringLiteral("teaching:20|winter-break:6"),
     "5665863b9638f6501cc42a2138698a7cbde83884b65f825ba632ce8f69f99048",
     "00756e4d793ad9922dcfc8dc26c8231b20910e8a0186a1d6bf6ed1e8eb03a0a1",
     "1ebbedb246b5636dc9e3ba7f3562f559274d13e3b330426366e8799df73d2879"},
    {QStringLiteral("2027-spring"), QStringLiteral("春季学期"),
     QStringLiteral("2027-02-25"), QStringLiteral("2027-09-04"), 27, 7, 10,
     QStringLiteral("teaching:18|practice:2|summer-break:7"),
     "8ea8cb7851ac04325c96203c31d724fcd661c653b9f666bdf8448e575681a8c8",
     "7e31a30842eab65ef6de262e1de01d33c79f23ddc762c2d3277d33ee3cf3ca34",
     "8700004c4ccd3ee5ce2300b12c3e932d443b3328e90b4a5108d3a55f8fae192f"},
    {QStringLiteral("2025-fall"), QStringLiteral("秋季学期"),
     QStringLiteral("2025-09-02"), QStringLiteral("2026-03-07"), 26, 10, 10,
     QStringLiteral("teaching:20|winter-break:6"),
     "58933bbb0c553cb7aa8efb9be105e2ec127eaab27950db4a1a9e9bf324ccaa07",
     "8304e310e932921deb5ca1d54cdcf339140f8422c146d7ca3a8c168e2aa47a04",
     "a436a50e9232ea599b068eeed6f2fd40c773e1c9fbb4edf8b53799fa80d719a6"},
    {QStringLiteral("2026-spring"), QStringLiteral("春季学期"),
     QStringLiteral("2026-03-05"), QStringLiteral("2026-08-29"), 25, 6, 9,
     QStringLiteral("teaching:17|practice:2|summer-break:6"),
     "0344b4050a672f99bb6195b3c759e72e571941e798620b58ea230b72725ab8c3",
     "41e4e0d1a2fa7454df672dc2a2119a9fc3794d430119bc7f929dc8bc4d485d4e",
     "f4ee34028181035866ab8aeb0df383a3421969e23493fce155741f0875dd7655"},
    {QStringLiteral("2024-fall"), QStringLiteral("秋季学期"),
     QStringLiteral("2024-08-29"), QStringLiteral("2025-02-22"), 25, 10, 10,
     QStringLiteral("teaching:19|winter-break:6"),
     "f34fd626af5c109b73dd05e1b9a827938bfbc45e536890cbfddaf40c2fe8f28f",
     "bfe6f5b9142197c55399312256a0ec4fd87c9645fdaf54b37390ca326f638e2d",
     "d58d6e2db06a392bea83dc19100a2bb9de6d0c82585e6ad684ca4d59c5b86498"},
    {QStringLiteral("2025-spring"), QStringLiteral("春季学期"),
     QStringLiteral("2025-02-20"), QStringLiteral("2025-09-06"), 28, 6, 9,
     QStringLiteral("teaching:18|practice:2|summer-break:8"),
     "3500e8092065df246a8a770fca3a01a1278b2a23871a307f0910231d4be634c8",
     "0dab9365e600d8f77a1fccca511be236fe1e6c8f63cde1dfaee5392ebbd18f46",
     "14d3224f4b064179a3ea97af47bbb6e8235467287c4581e2aa77d2913bbcbd6f"},
    {QStringLiteral("2023-fall"), QStringLiteral("秋季学期"),
     QStringLiteral("2023-08-27"), QStringLiteral("2024-02-24"), 25, 6, 8,
     QStringLiteral("teaching:20|winter-break:5"),
     "3336475a0d1be06abcc8a57d936ac92c0c690e6ee32e22b403ba7d46a4187d7a",
     "ad093a6aabff73e323c49dc1e6a60156bc6e05cb6df664bef94e3fab18136913",
     "bded0703e0307bee55c5e33fc022ccff6d9631fe04260bc23c99d38e9a622999"},
    {QStringLiteral("2024-spring"), QStringLiteral("春季学期"),
     QStringLiteral("2024-02-22"), QStringLiteral("2024-08-31"), 27, 5, 8,
     QStringLiteral("teaching:18|practice:2|summer-break:7"),
     "8aec1c0375bfd9904c70937e6180aa3561438e55a23d3b6d5d333d9b7417ba38",
     "369ac03c29d30f35733d6cd5141f9777dde70a27f387ad642e12757576e631f7",
     "6f646f0bd9bd7d0af872f7e4d541a7d1520c0804fd9e5f97e574172252410d4c"},
    {QStringLiteral("2022-fall"), QStringLiteral("秋季学期"),
     QStringLiteral("2022-08-25"), QStringLiteral("2023-02-18"), 25, 7, 9,
     QStringLiteral("teaching:20|winter-break:5"),
     "846932d78fba35f8a151b4fc30c485f120aa73309b38b5b6a75752674784cac8",
     "6af82c7f408de2b16a5accb23762ae77623dc993fda8800b36c019da3d0831ca",
     "06bcc87fd048e9e114540ba89cf6e0930acd35652c6b822fda7bcace171a3182"},
    {QStringLiteral("2023-spring"), QStringLiteral("春季学期"),
     QStringLiteral("2023-02-16"), QStringLiteral("2023-08-26"), 27, 5, 8,
     QStringLiteral("teaching:18|practice:2|summer-break:7"),
     "cba1c4e77b82cbf73a7124f3d8d4155554a57bdcf9468b13b6fab1642379337c",
     "612d3b519a9c0e65a4316bb649922abaf5f54028538a7dc0e5637f4d372cde12",
     "05c525e418708b3d2312825df81d9b1d27908674b97c6046163f98997c4aaeaa"},
    {QStringLiteral("2021-fall"), QStringLiteral("秋季学期"),
     QStringLiteral("2021-08-26"), QStringLiteral("2022-02-19"), 25, 6, 8,
     QStringLiteral("teaching:20|winter-break:5"),
     "9060846ced80f078867c14bf31155c1d09fa9aecafe02ced2835f221d9d934a6",
     "23870ca33779217e2a355314b53831af414d47c6037b1bc38a705a51d697cacd",
     "32ca97ecd32b1483804265d2e61660e4a422047285d4162889c7036c17c0ab56"},
    {QStringLiteral("2022-spring"), QStringLiteral("春季学期"),
     QStringLiteral("2022-02-17"), QStringLiteral("2022-08-27"), 27, 5, 8,
     QStringLiteral("teaching:18|practice:2|summer-break:7"),
     "f39aec1b79e54c6ee644dd2172dc08a3ff25046848d9458dc0ac81bc20dc992b",
     "8721c377bef9ab86068f3db2bf6c1de49aa029eb8637edb6781ec6dd2e159496",
     "fe8c6ea9a950bd230bae52ccdd451c22b74b4c12a9105286cb142dabc8697b10"},
};

QDate isoDate(const QJsonObject &object, const char *key)
{
    const QString value = object.value(QLatin1String(key)).toString();
    return QDate::fromString(value, Qt::ISODate);
}

QJsonObject loadCalendarCorpus()
{
    QFile file(QStringLiteral(SCU_NEXUS_SOURCE_DIR
                              "/resources/calendar/academic_calendars.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object();
}

QByteArray canonicalHash(const QJsonArray &array);

QStringList validateCalendarCorpus(const QJsonObject &root)
{
    QStringList errors;
    const auto require = [&errors](bool condition, const QString &message) {
        if (!condition) {
            errors.append(message);
        }
    };

    require(root.value(QStringLiteral("schemaVersion")).toInt() == 1,
            QStringLiteral("schemaVersion"));
    require(root.value(QStringLiteral("sourceIndexUrl")).toString()
                == QStringLiteral("https://jwc.scu.edu.cn/cdxl.htm"),
            QStringLiteral("sourceIndexUrl"));
    require(root.value(QStringLiteral("capturedAt")).toString()
                == QStringLiteral("2026-07-15"),
            QStringLiteral("capturedAt"));

    const QJsonArray calendars = root.value(QStringLiteral("calendars")).toArray();
    require(calendars.size() == expectedCalendarOrder.size(), QStringLiteral("calendar count"));

    const QSet<QString> allowedPhases{
        QStringLiteral("teaching"), QStringLiteral("practice"),
        QStringLiteral("winter-break"), QStringLiteral("summer-break")};
    const QSet<QString> allowedEventTypes{
        QStringLiteral("registration"), QStringLiteral("make-up-exam"),
        QStringLiteral("ceremony"), QStringLiteral("instruction"),
        QStringLiteral("holiday"), QStringLiteral("athletics"),
        QStringLiteral("final-exam"), QStringLiteral("vacation")};

    const int calendarLimit = qMin(calendars.size(), expectedCalendarOrder.size());
    for (int calendarIndex = 0; calendarIndex < calendarLimit; ++calendarIndex) {
        const QJsonObject calendar = calendars.at(calendarIndex).toObject();
        const QString expectedYear = expectedCalendarOrder.at(calendarIndex);
        const ExpectedCalendar expectedCalendar = expectedCalendars.value(expectedYear);
        const QString prefix = QStringLiteral("calendar[%1]").arg(calendarIndex);

        require(calendar.value(QStringLiteral("academicYear")).toString() == expectedYear,
                prefix + QStringLiteral(" academicYear/order"));
        QString expectedTitle = expectedYear;
        expectedTitle.replace(QLatin1Char('-'), QChar(0x2014));
        expectedTitle += QStringLiteral("学年四川大学校历");
        require(calendar.value(QStringLiteral("title")).toString() == expectedTitle,
                prefix + QStringLiteral(" title"));
        require(calendar.value(QStringLiteral("publishedAt")).toString()
                    == expectedCalendar.publishedAt,
                prefix + QStringLiteral(" publishedAt"));
        require(calendar.value(QStringLiteral("sourcePagePath")).toString()
                    == expectedCalendar.pagePath,
                prefix + QStringLiteral(" sourcePagePath"));

        const QJsonArray terms = calendar.value(QStringLiteral("terms")).toArray();
        require(terms.size() == 2, prefix + QStringLiteral(" term count"));
        const int termLimit = qMin(terms.size(), 2);
        for (int termOffset = 0; termOffset < termLimit; ++termOffset) {
            const ExpectedTerm &expected = expectedTerms.at(calendarIndex * 2 + termOffset);
            const QJsonObject term = terms.at(termOffset).toObject();
            const QString termPrefix = prefix + QStringLiteral(" term[%1] ").arg(termOffset)
                + expected.id;

            require(term.value(QStringLiteral("id")).toString() == expected.id,
                    termPrefix + QStringLiteral(" id/order"));
            require(term.value(QStringLiteral("name")).toString() == expected.name,
                    termPrefix + QStringLiteral(" name/order"));
            require(term.value(QStringLiteral("startDate")).toString() == expected.startDate,
                    termPrefix + QStringLiteral(" startDate"));
            require(term.value(QStringLiteral("endDate")).toString() == expected.endDate,
                    termPrefix + QStringLiteral(" endDate"));
            const QString expectedImage = termOffset == 0 ? expectedCalendar.fallImageUrl
                                                          : expectedCalendar.springImageUrl;
            require(term.value(QStringLiteral("sourceImageUrl")).toString() == expectedImage,
                    termPrefix + QStringLiteral(" sourceImageUrl"));

            const QDate termStart = isoDate(term, "startDate");
            const QDate termEnd = isoDate(term, "endDate");
            require(termStart.isValid() && termEnd.isValid() && termStart <= termEnd,
                    termPrefix + QStringLiteral(" term dates"));

            const QJsonArray weeks = term.value(QStringLiteral("weeks")).toArray();
            require(weeks.size() == expected.weekCount,
                    termPrefix + QStringLiteral(" week count"));
            QStringList phaseRuns;
            QString currentPhase;
            int currentPhaseCount = 0;
            QDate previousEnd;
            for (int weekIndex = 0; weekIndex < weeks.size(); ++weekIndex) {
                const QJsonObject week = weeks.at(weekIndex).toObject();
                const QString phase = week.value(QStringLiteral("phase")).toString();
                const QDate start = isoDate(week, "startDate");
                const QDate end = isoDate(week, "endDate");
                require(week.value(QStringLiteral("weekNo")).toInt() == weekIndex + 1,
                        termPrefix + QStringLiteral(" weekNo[%1]").arg(weekIndex));
                require(allowedPhases.contains(phase),
                        termPrefix + QStringLiteral(" phase[%1]").arg(weekIndex));
                require(start.isValid() && start.dayOfWeek() == Qt::Sunday,
                        termPrefix + QStringLiteral(" Sunday[%1]").arg(weekIndex));
                require(end.isValid() && end.dayOfWeek() == Qt::Saturday,
                        termPrefix + QStringLiteral(" Saturday[%1]").arg(weekIndex));
                require(start.isValid() && end.isValid() && start.daysTo(end) == 6,
                        termPrefix + QStringLiteral(" span[%1]").arg(weekIndex));
                require(!termStart.isValid() || !termEnd.isValid()
                            || (start >= termStart && end <= termEnd),
                        termPrefix + QStringLiteral(" boundary[%1]").arg(weekIndex));
                if (weekIndex > 0) {
                    require(previousEnd.addDays(1) == start,
                            termPrefix + QStringLiteral(" continuity[%1]").arg(weekIndex));
                }
                previousEnd = end;

                if (phase != currentPhase) {
                    if (!currentPhase.isEmpty()) {
                        phaseRuns.append(currentPhase + QLatin1Char(':')
                                         + QString::number(currentPhaseCount));
                    }
                    currentPhase = phase;
                    currentPhaseCount = 1;
                } else {
                    ++currentPhaseCount;
                }
            }
            if (!currentPhase.isEmpty()) {
                phaseRuns.append(currentPhase + QLatin1Char(':')
                                 + QString::number(currentPhaseCount));
            }
            require(phaseRuns.join(QLatin1Char('|')) == expected.phaseRuns,
                    termPrefix + QStringLiteral(" phase runs"));
            require(canonicalHash(weeks) == expected.weekHash,
                    termPrefix + QStringLiteral(" canonical week hash"));

            const QJsonArray events = term.value(QStringLiteral("events")).toArray();
            require(events.size() == expected.eventCount,
                    termPrefix + QStringLiteral(" event count"));
            for (int eventIndex = 0; eventIndex < events.size(); ++eventIndex) {
                const QJsonObject event = events.at(eventIndex).toObject();
                const QString expectedEventId = QStringLiteral("%1-event-%2")
                                                    .arg(expected.id)
                                                    .arg(eventIndex + 1, 2, 10, QLatin1Char('0'));
                require(event.value(QStringLiteral("id")).toString() == expectedEventId,
                        termPrefix + QStringLiteral(" event id/order[%1]").arg(eventIndex));
                require(allowedEventTypes.contains(
                            event.value(QStringLiteral("type")).toString()),
                        termPrefix + QStringLiteral(" event type[%1]").arg(eventIndex));
                require(!event.value(QStringLiteral("title")).toString().trimmed().isEmpty(),
                        termPrefix + QStringLiteral(" event title[%1]").arg(eventIndex));
                const QDate start = isoDate(event, "startDate");
                const QDate end = isoDate(event, "endDate");
                require(start.isValid() && end.isValid() && start <= end,
                        termPrefix + QStringLiteral(" event dates[%1]").arg(eventIndex));
            }
            require(canonicalHash(events) == expected.eventHash,
                    termPrefix + QStringLiteral(" canonical event hash"));

            const QJsonArray notes = term.value(QStringLiteral("notes")).toArray();
            require(notes.size() == expected.noteCount,
                    termPrefix + QStringLiteral(" note count"));
            for (int noteIndex = 0; noteIndex < notes.size(); ++noteIndex) {
                const QJsonObject note = notes.at(noteIndex).toObject();
                require(note.value(QStringLiteral("number")).toInt() == noteIndex + 1,
                        termPrefix + QStringLiteral(" note number/order[%1]").arg(noteIndex));
                require(!note.value(QStringLiteral("text")).toString().trimmed().isEmpty(),
                        termPrefix + QStringLiteral(" note text[%1]").arg(noteIndex));
            }
            const QByteArray noteHash = canonicalHash(notes);
            require(noteHash == expected.noteHash,
                    termPrefix + QStringLiteral(" canonical note hash: ")
                        + QString::fromLatin1(noteHash));
        }
    }

    return errors;
}

QByteArray canonicalHash(const QJsonArray &array)
{
    return QCryptographicHash::hash(QJsonDocument(array).toJson(QJsonDocument::Compact),
                                    QCryptographicHash::Sha256)
        .toHex();
}

} // namespace

class CalendarDataTests final : public QObject
{
    Q_OBJECT

private slots:
    void mutationsAreRejected_data()
    {
        QTest::addColumn<QString>("mutation");

        QTest::newRow("removed middle week") << QStringLiteral("week");
        QTest::newRow("bad phase enum") << QStringLiteral("phase");
        QTest::newRow("modified event date and title") << QStringLiteral("event");
        QTest::newRow("modified note text") << QStringLiteral("note");
    }

    void mutationsAreRejected()
    {
        QFETCH(QString, mutation);

        QJsonObject root = loadCalendarCorpus();
        QVERIFY(!root.isEmpty());
        QJsonArray calendars = root.value(QStringLiteral("calendars")).toArray();
        QJsonObject calendar = calendars.at(0).toObject();
        QJsonArray terms = calendar.value(QStringLiteral("terms")).toArray();
        QJsonObject term = terms.at(0).toObject();

        if (mutation == QStringLiteral("week")) {
            QJsonArray weeks = term.value(QStringLiteral("weeks")).toArray();
            weeks.removeAt(weeks.size() / 2);
            term.insert(QStringLiteral("weeks"), weeks);
        } else if (mutation == QStringLiteral("phase")) {
            QJsonArray weeks = term.value(QStringLiteral("weeks")).toArray();
            QJsonObject week = weeks.at(0).toObject();
            week.insert(QStringLiteral("phase"), QStringLiteral("semester-ish"));
            weeks.replace(0, week);
            term.insert(QStringLiteral("weeks"), weeks);
        } else if (mutation == QStringLiteral("event")) {
            QJsonArray events = term.value(QStringLiteral("events")).toArray();
            QJsonObject event = events.at(0).toObject();
            event.insert(QStringLiteral("title"), QStringLiteral("被篡改的事件"));
            event.insert(QStringLiteral("startDate"), QStringLiteral("2099-01-01"));
            events.replace(0, event);
            term.insert(QStringLiteral("events"), events);
        } else if (mutation == QStringLiteral("note")) {
            QJsonArray notes = term.value(QStringLiteral("notes")).toArray();
            QJsonObject note = notes.at(0).toObject();
            note.insert(QStringLiteral("text"), QStringLiteral("被篡改的备注"));
            notes.replace(0, note);
            term.insert(QStringLiteral("notes"), notes);
        }

        terms.replace(0, term);
        calendar.insert(QStringLiteral("terms"), terms);
        calendars.replace(0, calendar);
        root.insert(QStringLiteral("calendars"), calendars);

        const QStringList errors = validateCalendarCorpus(root);
        QVERIFY2(!errors.isEmpty(), "corrupted corpus was accepted");
    }

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
        const QStringList validationErrors = validateCalendarCorpus(root);
        QVERIFY2(validationErrors.isEmpty(), qPrintable(validationErrors.join(QLatin1Char('\n'))));
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
