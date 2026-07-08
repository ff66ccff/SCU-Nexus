#include "widgets/GradesWidget.h"

#include "viewmodels/GradesViewModel.h"
#include "widgets/QueryStateWidget.h"

#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

GradesWidget::GradesWidget(GradesViewModel *viewModel, QWidget *parent)
    : QWidget(parent),
      m_viewModel(viewModel)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 8, 0, 0);
    root->setSpacing(12);
    auto *toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(16, 12, 16, 12);
    toolbar->setSpacing(12);
    auto *toolbarWidget = new QWidget(this);
    toolbarWidget->setObjectName(QStringLiteral("Toolbar"));
    toolbarWidget->setLayout(toolbar);
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索课程名称"));
    m_searchEdit->setMinimumWidth(280);
    m_searchEdit->setMaximumWidth(420);
    auto *searchLabel = new QLabel(tr("课程检索"), toolbarWidget);
    searchLabel->setObjectName(QStringLiteral("SectionTitle"));
    toolbar->addWidget(searchLabel);
    toolbar->addWidget(m_searchEdit);
    toolbar->addStretch();
    root->addWidget(toolbarWidget);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(createSchemeTab(), tr("方案成绩"));
    m_tabs->addTab(createPassingTab(), tr("及格成绩"));
    m_tabs->addTab(createCustomTab(), tr("自定义统计"));
    root->addWidget(m_tabs, 1);

    connect(m_searchEdit, &QLineEdit::textChanged, m_viewModel, &GradesViewModel::setSearchQuery);
    connect(m_viewModel, &GradesViewModel::schemeChanged, this, &GradesWidget::syncScheme);
    connect(m_viewModel, &GradesViewModel::passingChanged, this, &GradesWidget::syncPassing);
    connect(m_viewModel, &GradesViewModel::searchQueryChanged, this, [this]() {
        syncScheme();
        syncPassing();
        syncCustom();
    });
    syncScheme();
    syncPassing();
    syncCustom();
}

QWidget *GradesWidget::createSchemeTab()
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(10);
    auto *toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(0, 0, 0, 0);
    m_refreshSchemeButton = new QPushButton(tr("刷新"), tab);
    m_refreshSchemeButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_schemeUpdatedLabel = new QLabel(tr("尚未更新"), tab);
    m_schemeUpdatedLabel->setObjectName(QStringLiteral("LastUpdated"));
    toolbar->addWidget(m_refreshSchemeButton);
    toolbar->addWidget(m_schemeUpdatedLabel);
    toolbar->addStretch();
    layout->addLayout(toolbar);
    m_schemeSummaryLabel = new QLabel(tab);
    m_schemeSummaryLabel->setObjectName(QStringLiteral("SummaryCard"));
    m_schemeSummaryLabel->setMargin(14);
    m_schemeSummaryLabel->setWordWrap(true);
    layout->addWidget(m_schemeSummaryLabel);
    m_schemeState = new QueryStateWidget(tab);
    layout->addWidget(m_schemeState);
    m_schemeTree = new QTreeWidget(tab);
    m_schemeTree->setHeaderLabels({tr("课程"), tr("属性"), tr("学分"), tr("成绩"), tr("绩点"), tr("等级")});
    m_schemeTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_schemeTree->setColumnWidth(1, 90);
    m_schemeTree->setColumnWidth(2, 70);
    m_schemeTree->setColumnWidth(3, 82);
    m_schemeTree->setColumnWidth(4, 70);
    m_schemeTree->setColumnWidth(5, 70);
    m_schemeTree->setAlternatingRowColors(true);
    m_schemeTree->setRootIsDecorated(true);
    m_schemeTree->setIndentation(18);
    layout->addWidget(m_schemeTree, 1);
    connect(m_refreshSchemeButton, &QPushButton::clicked, m_viewModel, &GradesViewModel::refreshSchemeScores);
    connect(m_schemeState, &QueryStateWidget::retryRequested, m_viewModel, &GradesViewModel::refreshSchemeScores);
    return tab;
}

QWidget *GradesWidget::createPassingTab()
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(10);
    auto *toolbar = new QHBoxLayout();
    m_refreshPassingButton = new QPushButton(tr("刷新"), tab);
    m_refreshPassingButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_passingUpdatedLabel = new QLabel(tr("尚未更新"), tab);
    m_passingUpdatedLabel->setObjectName(QStringLiteral("LastUpdated"));
    toolbar->addWidget(m_refreshPassingButton);
    toolbar->addWidget(m_passingUpdatedLabel);
    toolbar->addStretch();
    layout->addLayout(toolbar);
    m_passingSummaryLabel = new QLabel(tab);
    m_passingSummaryLabel->setObjectName(QStringLiteral("SummaryCard"));
    m_passingSummaryLabel->setMargin(14);
    m_passingSummaryLabel->setWordWrap(true);
    layout->addWidget(m_passingSummaryLabel);
    m_passingState = new QueryStateWidget(tab);
    layout->addWidget(m_passingState);
    m_passingTree = new QTreeWidget(tab);
    m_passingTree->setHeaderLabels({tr("课程"), tr("属性"), tr("学分"), tr("成绩"), tr("绩点"), tr("等级")});
    m_passingTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_passingTree->setColumnWidth(1, 90);
    m_passingTree->setColumnWidth(2, 70);
    m_passingTree->setColumnWidth(3, 82);
    m_passingTree->setColumnWidth(4, 70);
    m_passingTree->setColumnWidth(5, 70);
    m_passingTree->setAlternatingRowColors(true);
    m_passingTree->setRootIsDecorated(true);
    m_passingTree->setIndentation(18);
    layout->addWidget(m_passingTree, 1);
    connect(m_refreshPassingButton, &QPushButton::clicked, m_viewModel, &GradesViewModel::refreshPassingScores);
    connect(m_passingState, &QueryStateWidget::retryRequested, m_viewModel, &GradesViewModel::refreshPassingScores);
    return tab;
}

QWidget *GradesWidget::createCustomTab()
{
    auto *tab = new QWidget(this);
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(0, 10, 0, 0);
    layout->setSpacing(10);
    auto *toolbar = new QHBoxLayout();
    m_attrFilter = new QComboBox(tab);
    m_attrFilter->addItems({tr("全部"), tr("必修"), tr("选修"), tr("任选")});
    auto *selectAllButton = new QPushButton(tr("全选"), tab);
    m_clearSelectionButton = new QPushButton(tr("取消全选"), tab);
    selectAllButton->setObjectName(QStringLiteral("PrimaryButton"));
    toolbar->addWidget(m_attrFilter);
    toolbar->addWidget(selectAllButton);
    toolbar->addWidget(m_clearSelectionButton);
    toolbar->addStretch();
    layout->addLayout(toolbar);
    m_customSummaryLabel = new QLabel(tab);
    m_customSummaryLabel->setObjectName(QStringLiteral("SummaryCard"));
    m_customSummaryLabel->setMargin(14);
    m_customSummaryLabel->setWordWrap(true);
    layout->addWidget(m_customSummaryLabel);
    m_customState = new QueryStateWidget(tab);
    layout->addWidget(m_customState);
    m_customTree = new QTreeWidget(tab);
    m_customTree->setHeaderLabels({tr("课程"), tr("属性"), tr("学分"), tr("成绩"), tr("绩点"), tr("等级")});
    m_customTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_customTree->setColumnWidth(1, 90);
    m_customTree->setColumnWidth(2, 70);
    m_customTree->setColumnWidth(3, 82);
    m_customTree->setColumnWidth(4, 70);
    m_customTree->setColumnWidth(5, 70);
    m_customTree->setAlternatingRowColors(true);
    m_customTree->setRootIsDecorated(true);
    m_customTree->setIndentation(18);
    layout->addWidget(m_customTree, 1);
    connect(m_attrFilter, &QComboBox::currentTextChanged, this, &GradesWidget::syncCustom);

    connect(selectAllButton, &QPushButton::clicked, this, [this]() {
        const QString attr = m_attrFilter->currentText();
        for (int i = 0; i < m_customTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *group = m_customTree->topLevelItem(i);
            for (int j = 0; j < group->childCount(); ++j) {
                QTreeWidgetItem *course = group->child(j);
                const bool attrVisible = attr == tr("全部") || course->text(1) == attr;
                if (attrVisible && !course->isHidden()) {
                    course->setCheckState(0, Qt::Checked);
                    m_selectedCourseKeys.insert(course->data(0, Qt::UserRole).toString());
                }
            }
        }
        syncCustom();
    });

    connect(m_clearSelectionButton, &QPushButton::clicked, this, [this]() {
        m_selectedCourseKeys.clear();
        for (int i = 0; i < m_customTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *group = m_customTree->topLevelItem(i);
            for (int j = 0; j < group->childCount(); ++j) {
                group->child(j)->setCheckState(0, Qt::Unchecked);
            }
        }
        syncCustom();
    });
    connect(m_customTree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item) {
        if (!item) {
            return;
        }
        if (item->childCount() > 0) {
            const bool check = item->checkState(0) == Qt::Checked;
            const QSignalBlocker blocker(m_customTree);
            for (int j = 0; j < item->childCount(); ++j) {
                QTreeWidgetItem *course = item->child(j);
                if (!course->isHidden()) {
                    course->setCheckState(0, check ? Qt::Checked : Qt::Unchecked);
                    if (check) {
                        m_selectedCourseKeys.insert(course->data(0, Qt::UserRole).toString());
                    } else {
                        m_selectedCourseKeys.remove(course->data(0, Qt::UserRole).toString());
                    }
                }
            }
        } else {
            const QString key = item->data(0, Qt::UserRole).toString();
            if (item->checkState(0) == Qt::Checked) {
                m_selectedCourseKeys.insert(key);
            } else {
                m_selectedCourseKeys.remove(key);
            }
        }
        syncCustom();
    });
    return tab;
}

void GradesWidget::syncScheme()
{
    m_refreshSchemeButton->setEnabled(m_viewModel->schemeState() != QueryState::Loading);
    m_schemeUpdatedLabel->setText(m_viewModel->schemeLastUpdated().isValid()
                                      ? tr("更新于 %1").arg(m_viewModel->schemeLastUpdated().toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")))
                                      : tr("尚未更新"));
    m_schemeState->setState(m_viewModel->schemeState(), m_viewModel->schemeErrorMessage());
    m_schemeTree->setVisible(m_viewModel->schemeState() == QueryState::Loaded);
    m_schemeSummaryLabel->setVisible(m_viewModel->schemeState() == QueryState::Loaded);
    fillSummary(m_schemeSummaryLabel, m_viewModel->schemeSummary(),
                {QStringLiteral("gpa"), QStringLiteral("requiredGpa"), QStringLiteral("passedCount"), QStringLiteral("failedCount"),
                 QStringLiteral("weightedAvgScore"), QStringLiteral("requiredWeightedAvgScore"), QStringLiteral("earnedCredits"), QStringLiteral("totalCredits")},
                {tr("GPA"), tr("必修 GPA"), tr("通过门数"), tr("未通过门数"), tr("平均分"), tr("必修平均分"), tr("已修学分"), tr("要求学分")});
    fillGroupedCourses(m_schemeTree, m_viewModel->filteredSchemeGroups(), false);
    syncCustom();
}

void GradesWidget::syncPassing()
{
    m_refreshPassingButton->setEnabled(m_viewModel->passingState() != QueryState::Loading);
    m_passingUpdatedLabel->setText(m_viewModel->passingLastUpdated().isValid()
                                       ? tr("更新于 %1").arg(m_viewModel->passingLastUpdated().toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")))
                                       : tr("尚未更新"));
    m_passingState->setState(m_viewModel->passingState(), m_viewModel->passingErrorMessage());
    m_passingTree->setVisible(m_viewModel->passingState() == QueryState::Loaded);
    m_passingSummaryLabel->setVisible(m_viewModel->passingState() == QueryState::Loaded);
    fillSummary(m_passingSummaryLabel, m_viewModel->passingSummary(),
                {QStringLiteral("gpa"), QStringLiteral("totalCredits"), QStringLiteral("totalPassed"), QStringLiteral("termCount"),
                 QStringLiteral("weightedAvgScore"), QStringLiteral("requiredWeightedAvgScore")},
                {tr("总 GPA"), tr("累计学分"), tr("总通过门数"), tr("学期数"), tr("平均分"), tr("必修平均分")});
    fillGroupedCourses(m_passingTree, m_viewModel->filteredPassingGroups(), false);
}

void GradesWidget::syncCustom()
{
    if (!m_customTree) {
        return;
    }
    m_customState->setState(m_viewModel->schemeState(), m_viewModel->schemeErrorMessage());
    m_customTree->setVisible(m_viewModel->schemeState() == QueryState::Loaded);
    m_customSummaryLabel->setVisible(m_viewModel->schemeState() == QueryState::Loaded);

    const QSignalBlocker blocker(m_customTree);
    fillGroupedCourses(m_customTree, m_viewModel->filteredSchemeGroups(), true);
    const QString attr = m_attrFilter->currentText();
    for (int i = 0; i < m_customTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *group = m_customTree->topLevelItem(i);
        for (int j = 0; j < group->childCount(); ++j) {
            QTreeWidgetItem *course = group->child(j);
            const bool attrVisible = attr == tr("全部") || course->text(1) == attr;
            course->setHidden(!attrVisible);
            course->setCheckState(0, m_selectedCourseKeys.contains(course->data(0, Qt::UserRole).toString()) ? Qt::Checked : Qt::Unchecked);
        }
    }
    fillSummary(m_customSummaryLabel, m_viewModel->customStatsForSelected(checkedKeys()),
                {QStringLiteral("gpa"), QStringLiteral("credits"), QStringLiteral("weightedAvgScore"),
                 QStringLiteral("requiredWeightedAvgScore"), QStringLiteral("passedCount"), QStringLiteral("failedCount"), QStringLiteral("selectedCount")},
                {tr("GPA"), tr("学分"), tr("加权均分"), tr("必修均分"), tr("通过"), tr("未通过"), tr("已选")});
}

void GradesWidget::fillSummary(QLabel *label, const QVariantMap &summary, const QStringList &keys, const QStringList &names)
{
    QStringList parts;
    for (int i = 0; i < keys.size(); ++i) {
        parts.append(tr("<span style=\"color:#607083;\">%1</span> <b style=\"color:#102033;\">%2</b>")
                         .arg(names.value(i), summary.value(keys.at(i)).toString()));
    }
    label->setText(parts.join(QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;")));
}

void GradesWidget::fillGroupedCourses(QTreeWidget *tree, const QVariantList &groups, bool checkable)
{
    tree->clear();
    for (const QVariant &groupValue : groups) {
        const QVariantMap groupMap = groupValue.toMap();
        const QString label = groupMap.value(QStringLiteral("label")).toString();
        const int passed = groupMap.value(QStringLiteral("passedCount")).toInt();
        const double credits = groupMap.value(QStringLiteral("credits")).toDouble();
        const QString groupTitle = label + tr("  (通过 %1 门, 学分 %2)")
                                       .arg(passed)
                                       .arg(credits, 0, 'f', 1);
        auto *group = new QTreeWidgetItem(tree, {groupTitle});
        QFont groupFont = group->font(0);
        groupFont.setBold(true);
        group->setFont(0, groupFont);
        group->setForeground(0, QColor(QStringLiteral("#1f2937")));
        if (checkable) {
            group->setFlags(group->flags() | Qt::ItemIsUserCheckable);
            group->setCheckState(0, Qt::Unchecked);
        }
        group->setFirstColumnSpanned(true);
        const QVariantList courses = groupMap.value(QStringLiteral("items")).toList();
        for (const QVariant &courseValue : courses) {
            const QVariantMap course = courseValue.toMap();
            auto *item = new QTreeWidgetItem(group, {
                course.value(QStringLiteral("courseName")).toString(),
                course.value(QStringLiteral("courseAttributeName")).toString(),
                course.value(QStringLiteral("credit")).toString(),
                course.value(QStringLiteral("rawScore")).toString(),
                course.value(QStringLiteral("gradePointScore")).toString(),
                course.value(QStringLiteral("gradeName")).toString()
            });
            item->setData(0, Qt::UserRole, course.value(QStringLiteral("key")));
            if (checkable) {
                item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                item->setCheckState(0, Qt::Unchecked);
            }
            if (!course.value(QStringLiteral("passed")).toBool()) {
                item->setForeground(0, QColor(QStringLiteral("#b91c1c")));
            }
        }
        group->setExpanded(true);
    }
}

QVariantList GradesWidget::checkedKeys() const
{
    QVariantList keys;
    for (const QString &key : m_selectedCourseKeys) {
        keys.append(key);
    }
    return keys;
}
