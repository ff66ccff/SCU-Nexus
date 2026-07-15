#include <QtTest>

#include <QAccessible>
#include <QDate>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJSValue>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickView>
#include <QTemporaryDir>
#include <QUrl>

namespace {

QStringList *capturedWarnings = nullptr;
QtMessageHandler forwardedHandler = nullptr;

void captureWarnings(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (capturedWarnings && (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg))
        capturedWarnings->append(message);
    if (forwardedHandler)
        forwardedHandler(type, context, message);
}

class WarningCapture final
{
public:
    explicit WarningCapture(QStringList *warnings)
        : m_previous(qInstallMessageHandler(captureWarnings))
    {
        forwardedHandler = m_previous;
        capturedWarnings = warnings;
    }

    ~WarningCapture()
    {
        capturedWarnings = nullptr;
        forwardedHandler = nullptr;
        qInstallMessageHandler(m_previous);
    }

private:
    QtMessageHandler m_previous;
};

bool copyQmlTreeWithLocalThemeMetadata(const QString &destination)
{
    const QString sourceRoot = QStringLiteral(SCU_NEXUS_SOURCE_DIR "/qml");
    QDirIterator files(sourceRoot,
                       QStringList{QStringLiteral("*.qml")},
                       QDir::Files,
                       QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const QString sourcePath = files.next();
        const QString relativePath = QDir(sourceRoot).relativeFilePath(sourcePath);
        const QString destinationPath = QDir(destination).filePath(relativePath);
        if (!QDir().mkpath(QFileInfo(destinationPath).absolutePath())
            || !QFile::copy(sourcePath, destinationPath)) {
            return false;
        }
    }

    QFile themeMetadata(QDir(destination).filePath(QStringLiteral("styles/qmldir")));
    if (!themeMetadata.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    return themeMetadata.write("singleton Theme 1.0 Theme.qml\n") > 0;
}

class CalendarView final
{
public:
    bool initialize()
    {
        if (!qmlTree.isValid() || !copyQmlTreeWithLocalThemeMetadata(qmlTree.path()))
            return false;

        QQmlPropertyMap *themeManager = QQmlPropertyMap::create(view.engine());
        themeManager->insert(QStringLiteral("dark"), false);
        view.engine()->rootContext()->setContextProperty(
            QStringLiteral("themeManager"), themeManager);
        view.setInitialProperties({
            {QStringLiteral("width"), 1000},
            {QStringLiteral("height"), 760},
        });
        view.setSource(QUrl::fromLocalFile(QDir(qmlTree.path()).filePath(
            QStringLiteral("pages/calendar/StructuredAcademicCalendarView.qml"))));
        if (view.status() != QQuickView::Ready || !view.rootObject())
            return false;
        view.show();
        QCoreApplication::processEvents();
        return true;
    }

    QTemporaryDir qmlTree;
    QQuickView view;
};

QVariantMap event(const QString &id, const QDate &date)
{
    return {
        {QStringLiteral("id"), id},
        {QStringLiteral("type"), QStringLiteral("instruction")},
        {QStringLiteral("title"), id},
        {QStringLiteral("startDate"), date},
        {QStringLiteral("endDate"), date},
    };
}

QVariantMap note(int order, const QString &text)
{
    return {
        {QStringLiteral("order"), order},
        {QStringLiteral("text"), text},
    };
}

QVariantMap normalCalendar()
{
    const QVariantMap firstTerm{
        {QStringLiteral("id"), QStringLiteral("fall")},
        {QStringLiteral("name"), QStringLiteral("秋季学期")},
        {QStringLiteral("startDate"), QDate(2026, 8, 1)},
        {QStringLiteral("endDate"), QDate(2027, 1, 31)},
        {QStringLiteral("sourceImageUrl"), QUrl(QStringLiteral("https://example.test/fall.png"))},
        {QStringLiteral("weeks"), QVariantList{}},
        {QStringLiteral("events"), QVariantList{}},
        {QStringLiteral("notes"), QVariantList{}},
    };
    const QVariantMap secondTerm{
        {QStringLiteral("id"), QStringLiteral("spring")},
        {QStringLiteral("name"), QStringLiteral("春季学期")},
        {QStringLiteral("startDate"), QDate(2027, 2, 1)},
        {QStringLiteral("endDate"), QDate(2027, 7, 31)},
        {QStringLiteral("sourceImageUrl"), QUrl(QStringLiteral("https://example.test/spring.png"))},
        {QStringLiteral("weeks"), QVariantList{}},
        {QStringLiteral("events"), QVariantList{
            event(QStringLiteral("late"), QDate(2027, 5, 1)),
            event(QStringLiteral("same-a"), QDate(2027, 3, 1)),
            event(QStringLiteral("early"), QDate(2027, 2, 1)),
            event(QStringLiteral("same-b"), QDate(2027, 3, 1)),
        }},
        {QStringLiteral("notes"), QVariantList{
            note(2, QStringLiteral("第二条")),
            note(1, QStringLiteral("第一条甲")),
            note(1, QStringLiteral("第一条乙")),
        }},
    };
    return {
        {QStringLiteral("title"), QStringLiteral("测试校历")},
        {QStringLiteral("terms"), QVariantList{firstTerm, secondTerm}},
    };
}

qreal sceneY(QQuickItem *item, QQuickItem *root)
{
    return item->mapToItem(root, QPointF()).y();
}

void collectObjectNames(QQuickItem *item, QStringList *names)
{
    if (!item->objectName().isEmpty())
        names->append(item->objectName());
    for (QQuickItem *child : item->childItems())
        collectObjectNames(child, names);
}

QQuickItem *findQuickItem(QQuickItem *item, const QString &objectName)
{
    if (item->objectName() == objectName)
        return item;
    for (QQuickItem *child : item->childItems()) {
        if (QQuickItem *match = findQuickItem(child, objectName))
            return match;
    }
    return nullptr;
}

} // namespace

class StructuredCalendarQmlTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));
    }

    void malformedCalendarDataStaysWarningFreeAndEmpty()
    {
        CalendarView fixture;
        QStringList warnings;
        WarningCapture capture(&warnings);
        QVERIFY(fixture.initialize());
        warnings.clear();

        QQuickItem *root = fixture.view.rootObject();
        QQuickItem *emptyState = root->findChild<QQuickItem *>(
            QStringLiteral("structuredCalendarEmptyState"));
        QVERIFY(emptyState);

        const QVariant nullValue = QVariant::fromValue(
            fixture.view.engine()->evaluate(QStringLiteral("null")));
        const QList<QVariant> malformedValues{
            nullValue,
            QStringLiteral("not a calendar"),
            QVariantList{QStringLiteral("not a map")},
            QVariantMap{{QStringLiteral("terms"), nullValue}},
            QVariantMap{{QStringLiteral("terms"), QStringLiteral("not a list")}},
            QVariantMap{{QStringLiteral("terms"), QVariantMap{
                {QStringLiteral("length"), 1},
                {QStringLiteral("0"), normalCalendar()},
            }}},
            QVariantMap{{QStringLiteral("terms"), QVariantList{nullValue}}},
            QVariantMap{{QStringLiteral("terms"), QVariantList{42}}},
        };

        for (const QVariant &value : malformedValues) {
            QVERIFY(root->setProperty("calendarData", value));
            QCoreApplication::processEvents();
            QCOMPARE(root->property("terms").toList().size(), 0);
            QVERIFY(root->property("selectedTerm").toMap().isEmpty());
            QVERIFY(emptyState->isVisible());
        }

        const QVariantMap malformedTerm{
            {QStringLiteral("id"), QStringLiteral("malformed")},
            {QStringLiteral("weeks"), QVariantMap{{QStringLiteral("length"), 2}}},
            {QStringLiteral("events"), QStringLiteral("not a list")},
            {QStringLiteral("notes"), nullValue},
            {QStringLiteral("sourceImageUrl"), nullValue},
        };
        QVERIFY(root->setProperty("calendarData", QVariantMap{
            {QStringLiteral("terms"), QVariantList{malformedTerm}},
        }));
        QCoreApplication::processEvents();
        QCOMPARE(root->property("terms").toList().size(), 1);
        QCOMPARE(root->property("selectedTerm").toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("malformed"));

        const QVariantMap nullEntryTerm{
            {QStringLiteral("id"), QStringLiteral("null-entries")},
            {QStringLiteral("weeks"), QVariantList{nullValue, 42}},
            {QStringLiteral("events"), QVariantList{nullValue, 42}},
            {QStringLiteral("notes"), QVariantList{nullValue, 42}},
            {QStringLiteral("sourceImageUrl"), nullValue},
        };
        QVERIFY(root->setProperty("calendarData", QVariantMap{
            {QStringLiteral("terms"), QVariantList{nullEntryTerm}},
        }));
        QCoreApplication::processEvents();
        QCOMPARE(root->property("selectedTerm").toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("null-entries"));
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
    }

    void switchingTermsSortsEventsAndEmitsSelectedSource()
    {
        CalendarView fixture;
        QStringList warnings;
        WarningCapture capture(&warnings);
        QVERIFY(fixture.initialize());
        warnings.clear();

        QQuickItem *root = fixture.view.rootObject();
        QVERIFY(root->setProperty("calendarData", normalCalendar()));
        QQuickItem *selector = findQuickItem(root, QStringLiteral("structuredTermSelector"));
        QVERIFY(selector);
        QCOMPARE(root->property("selectedTermIndex").toInt(), 0);
        QVERIFY(QMetaObject::invokeMethod(selector, "activated",
                                          Q_ARG(QString, QStringLiteral("1"))));
        QTRY_COMPARE(root->property("selectedTermIndex").toInt(), 1);
        QTRY_COMPARE(root->property("selectedTerm").toMap()
                         .value(QStringLiteral("id")).toString(),
                     QStringLiteral("spring"));

        QQuickItem *early = findQuickItem(root, QStringLiteral("structuredEventCard-early"));
        QQuickItem *sameA = findQuickItem(root, QStringLiteral("structuredEventCard-same-a"));
        QQuickItem *sameB = findQuickItem(root, QStringLiteral("structuredEventCard-same-b"));
        QQuickItem *late = findQuickItem(root, QStringLiteral("structuredEventCard-late"));
        QStringList objectNames;
        collectObjectNames(root, &objectNames);
        QVERIFY2(early && sameA && sameB && late,
                 qPrintable(objectNames.join(QLatin1Char('\n'))));
        QTRY_VERIFY_WITH_TIMEOUT(sceneY(early, root) < sceneY(sameA, root), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(sceneY(sameA, root) < sceneY(sameB, root), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(sceneY(sameB, root) < sceneY(late, root), 1000);

        QQuickItem *originalButton = root->findChild<QQuickItem *>(
            QStringLiteral("structuredCalendarOriginalButton"));
        QVERIFY(originalButton);
        QSignalSpy originalSpy(root, SIGNAL(viewOriginalRequested(QString)));
        QVERIFY(originalSpy.isValid());
        QVERIFY(QMetaObject::invokeMethod(originalButton, "click"));
        QCOMPARE(originalSpy.size(), 1);
        QCOMPARE(originalSpy.first().first().toString(),
                 QStringLiteral("https://example.test/spring.png"));
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
    }

    void notesAreStableOrderedAndKeyboardExpandable()
    {
        CalendarView fixture;
        QStringList warnings;
        WarningCapture capture(&warnings);
        QVERIFY(fixture.initialize());
        warnings.clear();

        QQuickItem *root = fixture.view.rootObject();
        QVERIFY(root->setProperty("calendarData", normalCalendar()));
        QVERIFY(root->setProperty("selectedTermIndex", 1));
        QCoreApplication::processEvents();

        QQuickItem *toggle = findQuickItem(root, QStringLiteral("structuredNotesToggle"));
        QQuickItem *firstA = findQuickItem(root, QStringLiteral("structuredNote-1-0"));
        QQuickItem *firstB = findQuickItem(root, QStringLiteral("structuredNote-1-1"));
        QQuickItem *second = findQuickItem(root, QStringLiteral("structuredNote-2-2"));
        QStringList objectNames;
        collectObjectNames(root, &objectNames);
        QVERIFY2(toggle && firstA && firstB && second,
                 qPrintable(objectNames.join(QLatin1Char('\n'))));
        QVERIFY(!toggle->property("checked").toBool());
        QVERIFY(!firstA->isVisible());

        QAccessibleInterface *accessible = QAccessible::queryAccessibleInterface(toggle);
        QVERIFY(accessible);
        QCOMPARE(accessible->role(), QAccessible::Button);
        QVERIFY(!accessible->text(QAccessible::Name).isEmpty());
        QVERIFY(accessible->state().checkable);

        toggle->forceActiveFocus();
        QTest::keyClick(&fixture.view, Qt::Key_Space);
        QTRY_VERIFY(toggle->property("checked").toBool());
        QTRY_VERIFY(firstA->isVisible());
        QTRY_VERIFY_WITH_TIMEOUT(sceneY(firstA, root) < sceneY(firstB, root), 1000);
        QTRY_VERIFY_WITH_TIMEOUT(sceneY(firstB, root) < sceneY(second, root), 1000);
        QQuickItem *firstAText = findQuickItem(firstA, QStringLiteral("structuredNoteText"));
        QQuickItem *firstBText = findQuickItem(firstB, QStringLiteral("structuredNoteText"));
        QQuickItem *secondText = findQuickItem(second, QStringLiteral("structuredNoteText"));
        QVERIFY(firstAText && firstBText && secondText);
        QCOMPARE(firstAText->property("text").toString(), QStringLiteral("第一条甲"));
        QCOMPARE(firstBText->property("text").toString(), QStringLiteral("第一条乙"));
        QCOMPARE(secondText->property("text").toString(), QStringLiteral("第二条"));
        QTRY_VERIFY(QAccessible::queryAccessibleInterface(toggle)->state().checked);

        QTest::keyClick(&fixture.view, Qt::Key_Return);
        QTRY_VERIFY(!toggle->property("checked").toBool());
        QTRY_VERIFY(!firstA->isVisible());
        QTRY_VERIFY(!QAccessible::queryAccessibleInterface(toggle)->state().checked);
        QVERIFY2(warnings.isEmpty(), qPrintable(warnings.join(QLatin1Char('\n'))));
    }
};

QTEST_MAIN(StructuredCalendarQmlTests)
#include "test_structured_calendar_qml.moc"
