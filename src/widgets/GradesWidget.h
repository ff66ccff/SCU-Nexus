#pragma once

#include <QSet>
#include <QWidget>

class GradesViewModel;
class QLabel;
class QLineEdit;
class QTabWidget;
class QTreeWidget;
class QComboBox;
class QPushButton;
class QueryStateWidget;

class GradesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GradesWidget(GradesViewModel *viewModel, QWidget *parent = nullptr);

private slots:
    void syncScheme();
    void syncPassing();
    void syncCustom();

private:
    QWidget *createSchemeTab();
    QWidget *createPassingTab();
    QWidget *createCustomTab();
    void fillSummary(QLabel *label, const QVariantMap &summary, const QStringList &keys, const QStringList &names);
    void fillGroupedCourses(QTreeWidget *tree, const QVariantList &groups, bool checkable);
    QVariantList checkedKeys() const;

    GradesViewModel *m_viewModel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QTabWidget *m_tabs = nullptr;

    QPushButton *m_refreshSchemeButton = nullptr;
    QLabel *m_schemeUpdatedLabel = nullptr;
    QLabel *m_schemeSummaryLabel = nullptr;
    QueryStateWidget *m_schemeState = nullptr;
    QTreeWidget *m_schemeTree = nullptr;

    QPushButton *m_refreshPassingButton = nullptr;
    QLabel *m_passingUpdatedLabel = nullptr;
    QLabel *m_passingSummaryLabel = nullptr;
    QueryStateWidget *m_passingState = nullptr;
    QTreeWidget *m_passingTree = nullptr;

    QComboBox *m_attrFilter = nullptr;
    QPushButton *m_clearSelectionButton = nullptr;
    QLabel *m_customSummaryLabel = nullptr;
    QueryStateWidget *m_customState = nullptr;
    QTreeWidget *m_customTree = nullptr;
    QSet<QString> m_selectedCourseKeys;
};
