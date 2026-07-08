#pragma once

#include <QNetworkAccessManager>
#include <QWidget>

class AcademicCalendarViewModel;
class QComboBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QVBoxLayout;
class QueryStateWidget;

class AcademicCalendarWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AcademicCalendarWidget(AcademicCalendarViewModel *viewModel, QWidget *parent = nullptr);

private slots:
    void syncFromViewModel();
    void rebuildImages();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void clearImageLayout();

    AcademicCalendarViewModel *m_viewModel = nullptr;
    QComboBox *m_yearCombo = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QLabel *m_lastUpdatedLabel = nullptr;
    QueryStateWidget *m_stateWidget = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_imageContainer = nullptr;
    QVBoxLayout *m_imageLayout = nullptr;
    QNetworkAccessManager m_network;
    bool m_updatingCombo = false;
};
