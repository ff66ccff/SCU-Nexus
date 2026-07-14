#include "src/app/Router.h"
#include "src/services/zhjw/ZhjwQueryService.h"
#include "src/viewmodels/GradesViewModel.h"

#include <QtTest>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickView>
#include <QSet>
#include <QTemporaryDir>

namespace {

QJsonObject sampleSchemeRoot()
{
    const QJsonArray courses{
        QJsonObject{
            {QStringLiteral("courseName"), QStringLiteral("高等数学")},
            {QStringLiteral("englishCourseName"), QStringLiteral("Advanced Mathematics")},
            {QStringLiteral("courseAttributeName"), QStringLiteral("必修")},
            {QStringLiteral("credit"), QStringLiteral("3")},
            {QStringLiteral("cj"), QStringLiteral("90")},
            {QStringLiteral("courseScore"), 90.0},
            {QStringLiteral("gradePointScore"), 4.0},
            {QStringLiteral("gradeName"), QStringLiteral("A")},
            {QStringLiteral("academicYearCode"), QStringLiteral("2025-2026")},
            {QStringLiteral("termName"), QStringLiteral("秋")},
        },
        QJsonObject{
            {QStringLiteral("courseName"), QStringLiteral("大学英语")},
            {QStringLiteral("courseAttributeName"), QStringLiteral("任选")},
            {QStringLiteral("credit"), QStringLiteral("2")},
            {QStringLiteral("cj"), QStringLiteral("85")},
            {QStringLiteral("courseScore"), 85.0},
            {QStringLiteral("gradePointScore"), 3.5},
            {QStringLiteral("gradeName"), QStringLiteral("B+")},
            {QStringLiteral("academicYearCode"), QStringLiteral("2024-2025")},
            {QStringLiteral("termName"), QStringLiteral("春")},
        },
    };
    return QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{QJsonObject{
            {QStringLiteral("zxf"), 160.0},
            {QStringLiteral("yxxf"), 5.0},
            {QStringLiteral("tgms"), 2},
            {QStringLiteral("wtgms"), 0},
            {QStringLiteral("zms"), 2},
            {QStringLiteral("cjList"), courses},
        }}},
    };
}

QJsonObject samplePassingRoot()
{
    const QJsonArray courses = sampleSchemeRoot()
                                   .value(QStringLiteral("lnList")).toArray().first().toObject()
                                   .value(QStringLiteral("cjList")).toArray();
    return QJsonObject{
        {QStringLiteral("lnList"), QJsonArray{
            QJsonObject{
                {QStringLiteral("cjlx"), QStringLiteral("2025-2026学年秋")},
                {QStringLiteral("cjList"), QJsonArray{courses.first()}},
            },
            QJsonObject{
                {QStringLiteral("cjlx"), QStringLiteral("2024-2025学年春")},
                {QStringLiteral("cjList"), QJsonArray{courses.last()}},
            },
        }},
    };
}

class FakeZhjwQueryService final : public ZhjwQueryService
{
public:
    bool loggedIn() const override { return true; }
    void setLoggedIn(bool) override { }
    void fetchExamPlan(ExamPlanCallback callback) override { callback({}, {}); }
    void fetchSchemeScores(JsonCallback callback) override
    {
        callback(sampleSchemeRoot(), {});
    }
    void fetchPassingScores(JsonCallback callback) override
    {
        callback(samplePassingRoot(), {});
    }
};

void collectCourseCards(QQuickItem *item, QList<QQuickItem *> *cards)
{
    if (item->objectName() == QStringLiteral("gradeCourseCard")) {
        cards->append(item);
    }
    for (QQuickItem *child : item->childItems()) {
        collectCourseCards(child, cards);
    }
}

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
    if (!themeMetadata.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    return themeMetadata.write("singleton Theme 1.0 Theme.qml\n") > 0;
}

} // namespace

class GradesQmlTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));
    }

    void loadedScoresCreateVisibleCourseCards()
    {
        FakeZhjwQueryService service;
        GradesViewModel model(nullptr, &service);
        Router router;
        QTemporaryDir qmlTree;
        QVERIFY(qmlTree.isValid());
        QVERIFY(copyQmlTreeWithLocalThemeMetadata(qmlTree.path()));

        QQuickView view;
        QQmlPropertyMap *themeManager = QQmlPropertyMap::create(view.engine());
        themeManager->insert(QStringLiteral("dark"), false);
        view.engine()->rootContext()->setContextProperty(QStringLiteral("gradesViewModel"), &model);
        view.engine()->rootContext()->setContextProperty(QStringLiteral("router"), &router);
        view.engine()->rootContext()->setContextProperty(QStringLiteral("themeManager"), themeManager);
        view.setInitialProperties({
            {QStringLiteral("width"), 1100},
            {QStringLiteral("height"), 800},
        });
        view.setSource(QUrl::fromLocalFile(
            QDir(qmlTree.path()).filePath(QStringLiteral("pages/grades/GradesPage.qml"))));

        QCOMPARE(view.status(), QQuickView::Ready);
        QVERIFY(view.rootObject());
        view.show();
        QTRY_COMPARE(model.schemeState(), QueryState::Loaded);

        QQuickItem *schemeTab = view.rootObject()->findChild<QQuickItem *>(
            QStringLiteral("schemeScoresTab"));
        QVERIFY(schemeTab);
        QVERIFY(schemeTab->property("groups").toList().size() >= 2);
        QList<QQuickItem *> courseCards;
        collectCourseCards(schemeTab, &courseCards);

        QCOMPARE(courseCards.size(), 2);
        QSet<QString> courseNames;
        for (QQuickItem *card : std::as_const(courseCards)) {
            const QVariantMap course = card->property("course").toMap();
            courseNames.insert(course.value(QStringLiteral("courseName")).toString());
            QVERIFY(card->isVisible());
            QVERIFY2(card->width() > 0.0,
                     qPrintable(QStringLiteral("card width=%1").arg(card->width())));
            QVERIFY2(card->height() > 0.0,
                     qPrintable(QStringLiteral("card height=%1").arg(card->height())));
        }
        QVERIFY(courseNames.contains(QStringLiteral("高等数学")));
        QVERIFY(courseNames.contains(QStringLiteral("大学英语")));
    }
};

QTEST_MAIN(GradesQmlTests)
#include "test_grades_qml.moc"
