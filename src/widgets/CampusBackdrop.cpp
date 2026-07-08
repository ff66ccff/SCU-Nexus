#include "widgets/CampusBackdrop.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

CampusBackdrop::CampusBackdrop(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("PageSurface"));
    setAutoFillBackground(false);
}

void CampusBackdrop::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient wash(rect().topLeft(), rect().bottomRight());
    wash.setColorAt(0.0, QColor(QStringLiteral("#edf7fb")));
    wash.setColorAt(0.50, QColor(QStringLiteral("#e9f1f8")));
    wash.setColorAt(1.0, QColor(QStringLiteral("#f7fbff")));
    painter.fillRect(rect(), wash);

    QPen gridPen(QColor(47, 111, 221, 18), 1);
    painter.setPen(gridPen);
    constexpr int grid = 36;
    for (int x = 0; x < width(); x += grid) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += grid) {
        painter.drawLine(0, y, width(), y);
    }

    QPen routePen(QColor(47, 111, 221, 34), 2);
    routePen.setCapStyle(Qt::RoundCap);
    painter.setPen(routePen);
    QPainterPath route;
    route.moveTo(width() * 0.04, height() * 0.82);
    route.cubicTo(width() * 0.24, height() * 0.66,
                  width() * 0.36, height() * 0.98,
                  width() * 0.56, height() * 0.76);
    route.cubicTo(width() * 0.72, height() * 0.58,
                  width() * 0.82, height() * 0.76,
                  width() * 0.97, height() * 0.48);
    painter.drawPath(route);

    QPen edgePen(QColor(255, 255, 255, 96), 1);
    painter.setPen(edgePen);
    painter.drawLine(0, 0, width(), 0);
}
