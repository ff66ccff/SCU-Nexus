#include "widgets/AcademicCalendarWidget.h"

#include "viewmodels/AcademicCalendarViewModel.h"
#include "widgets/QueryStateWidget.h"

#include <QComboBox>
#include <QDialog>
#include <QEvent>
#include <QLabel>
#include <QNetworkReply>
#include <QPixmap>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>

// 读取指定资源并返回结果。
AcademicCalendarWidget::AcademicCalendarWidget(AcademicCalendarViewModel *viewModel, QWidget *parent)
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
    m_yearCombo = new QComboBox(this);
    m_yearCombo->setMinimumWidth(170);
    m_refreshButton = new QPushButton(tr("刷新"), this);
    m_refreshButton->setObjectName(QStringLiteral("PrimaryButton"));
    m_lastUpdatedLabel = new QLabel(tr("尚未更新"), this);
    m_lastUpdatedLabel->setObjectName(QStringLiteral("LastUpdated"));
    auto *yearLabel = new QLabel(tr("学年"), toolbarWidget);
    yearLabel->setObjectName(QStringLiteral("MutedText"));
    toolbar->addWidget(yearLabel);
    toolbar->addWidget(m_yearCombo);
    toolbar->addWidget(m_refreshButton);
    toolbar->addWidget(m_lastUpdatedLabel);
    toolbar->addStretch();
    root->addWidget(toolbarWidget);

    m_stateWidget = new QueryStateWidget(this);
    root->addWidget(m_stateWidget);

    m_imageContainer = new QWidget(this);
    m_imageLayout = new QVBoxLayout(m_imageContainer);
    m_imageLayout->setContentsMargins(0, 0, 0, 0);
    m_imageLayout->setSpacing(16);
    m_imageLayout->addStretch();
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setWidget(m_imageContainer);
    root->addWidget(m_scrollArea, 1);

    connect(m_refreshButton, &QPushButton::clicked, m_viewModel, &AcademicCalendarViewModel::refresh);
    connect(m_yearCombo, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        if (!m_updatingCombo) {
            m_viewModel->selectEntry(index);
        }
    });
    connect(m_stateWidget, &QueryStateWidget::retryRequested, m_viewModel, &AcademicCalendarViewModel::reloadList);
    connect(m_viewModel, &AcademicCalendarViewModel::stateChanged, this, &AcademicCalendarWidget::syncFromViewModel);
    connect(m_viewModel, &AcademicCalendarViewModel::entriesChanged, this, &AcademicCalendarWidget::syncFromViewModel);
    connect(m_viewModel, &AcademicCalendarViewModel::selectedChanged, this, &AcademicCalendarWidget::syncFromViewModel);
    connect(m_viewModel, &AcademicCalendarViewModel::imageUrlsChanged, this, &AcademicCalendarWidget::rebuildImages);
    connect(m_viewModel, &AcademicCalendarViewModel::lastUpdatedChanged, this, &AcademicCalendarWidget::syncFromViewModel);

    syncFromViewModel();
}

// 根据视图模型同步界面展示。
void AcademicCalendarWidget::syncFromViewModel()
{
    m_updatingCombo = true;
    m_yearCombo->clear();
    for (const QVariant &value : m_viewModel->entries()) {
        const QVariantMap entry = value.toMap();
        m_yearCombo->addItem(entry.value(QStringLiteral("title")).toString());
    }
    m_yearCombo->setCurrentIndex(m_viewModel->selectedIndex());
    m_updatingCombo = false;

    m_refreshButton->setEnabled(!m_viewModel->loading());
    m_lastUpdatedLabel->setText(m_viewModel->lastUpdated().isValid()
                                    ? tr("更新于 %1").arg(m_viewModel->lastUpdated().toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm")))
                                    : tr("尚未更新"));
    m_stateWidget->setState(m_viewModel->state(), m_viewModel->errorMessage());
    m_scrollArea->setVisible(m_viewModel->state() == QueryState::Loaded);
}

// 重建动态界面内容。
void AcademicCalendarWidget::rebuildImages()
{
    clearImageLayout();
    for (const QString &url : m_viewModel->imageUrls()) {
        auto *label = new QLabel(tr("图片加载中：%1").arg(url), m_imageContainer);
        label->setAlignment(Qt::AlignCenter);
        label->setWordWrap(true);
        label->setMinimumHeight(120);
        label->setObjectName(QStringLiteral("Card"));
        label->setStyleSheet(QStringLiteral("QLabel#Card { background: #ffffff; border: 1px solid #e5e7eb; border-radius: 8px; padding: 12px; color: #64748b; }"));
        m_imageLayout->insertWidget(m_imageLayout->count() - 1, label);

        QNetworkReply *reply = m_network.get(QNetworkRequest(QUrl(url)));
        connect(reply, &QNetworkReply::finished, this, [this, reply, label, url]() {
            reply->deleteLater();
            QPixmap pixmap;
            if (reply->error() == QNetworkReply::NoError && pixmap.loadFromData(reply->readAll())) {
                label->setPixmap(pixmap.scaledToWidth(qMax(320, m_scrollArea->viewport()->width() - 48), Qt::SmoothTransformation));
                label->setCursor(Qt::PointingHandCursor);
                label->setToolTip(tr("点击查看大图"));
                label->installEventFilter(this);
                label->setProperty("fullPixmap", pixmap);
            } else {
                label->setText(tr("图片加载失败：%1").arg(url));
            }
        });
    }
    syncFromViewModel();
}

// 清理内部状态或持久化数据。
void AcademicCalendarWidget::clearImageLayout()
{
    while (m_imageLayout->count() > 1) {
        QLayoutItem *item = m_imageLayout->takeAt(0);
        delete item->widget();
        delete item;
    }
}

// 拦截控件事件并处理交互预览。
bool AcademicCalendarWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto *label = qobject_cast<QLabel *>(watched);
        const QPixmap pixmap = label ? label->property("fullPixmap").value<QPixmap>() : QPixmap();
        if (!pixmap.isNull()) {
            QDialog dialog(this);
            dialog.setWindowTitle(tr("校历图片"));
            dialog.resize(980, 700);
            auto *layout = new QVBoxLayout(&dialog);
            auto *scroll = new QScrollArea(&dialog);
            auto *image = new QLabel(scroll);
            image->setPixmap(pixmap);
            image->setAlignment(Qt::AlignCenter);
            scroll->setWidget(image);
            layout->addWidget(scroll);
            dialog.exec();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// 响应窗口尺寸变化并调整子控件布局。
void AcademicCalendarWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    const int targetWidth = qMax(320, m_scrollArea->viewport()->width() - 48);
    for (int i = 0; i < m_imageLayout->count(); ++i) {
        auto *label = qobject_cast<QLabel *>(m_imageLayout->itemAt(i)->widget());
        if (!label) {
            continue;
        }
        const QPixmap pixmap = label->property("fullPixmap").value<QPixmap>();
        if (!pixmap.isNull()) {
            label->setPixmap(pixmap.scaledToWidth(targetWidth, Qt::SmoothTransformation));
        }
    }
}
