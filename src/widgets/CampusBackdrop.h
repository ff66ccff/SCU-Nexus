#pragma once

#include <QWidget>

class CampusBackdrop : public QWidget
{
    Q_OBJECT

public:
    explicit CampusBackdrop(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};
