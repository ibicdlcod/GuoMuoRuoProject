#include "mapdetail.h"
#include "ui_mapdetail.h"
#include "maprender.h"
#include <QPainter>
#include <QStyleHints>

MapDetail::MapDetail(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MapDetail) {
    ui->setupUi(this);
    antialiased = true;

    QImage image(":/Assets/Image/rudder.png");
    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    rudder = QPixmap::fromImage(image);
}

MapDetail::~MapDetail() {
    delete ui;
}

void MapDetail::displayDetailedMap(Map *map) {
    mapPointer = map;
    update();
}

void MapDetail::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);

    for(const auto &node: std::as_const(mapPointer->nodes)) {
        QPointF thisNodePos(node.x * width(), node.y * height());
        for(const auto &nextNodeId: node.nextNodes) {
            /* TODO: deal with map unlock stages */
            if(!mapPointer->nodes.contains(nextNodeId))
                continue;
            painter.setBrush(QBrush());
            QPen pen;
            auto nextNode = mapPointer->nodes[nextNodeId];
            QPointF nextNodePos(nextNode.x * width(), nextNode.y * height());
            switch(QApplication::styleHints()->colorScheme()) {
            case Qt::ColorScheme::Dark:
                pen.setColor(Qt::white);
                break;
            case Qt::ColorScheme::Light: [[fallthrough]];
            default:
                pen.setColor(Qt::black);
                break;
            }
            pen.setWidth(circleBorderSize * 2);

            static QList<qreal> dashes = {4, 6};
            pen.setDashPattern(dashes);

            painter.setPen(pen);
            auto delta = nextNodePos - thisNodePos;
            delta /= std::hypot((double)delta.x(), (double)delta.y());
            painter.drawLine(thisNodePos + delta * circleSize * 1.5,
                             nextNodePos - delta * circleSize * 1.5);
        }
    }
    for(const auto &node: std::as_const(mapPointer->nodes)) {
        static QBrush redBrush = QBrush(Qt::red);
        painter.setBrush(redBrush);
        painter.setPen(QPen(Qt::white, circleBorderSize));
        switch(node.type) {
        case KP::STARTING:
            painter.drawPixmap(node.x * width() - circleSize,
                               node.y * height() - circleSize,
                               rudder.scaled(QSize(circleSize * 2, circleSize * 2),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation
                                             )); break;
        case KP::NORMAL:
            painter.drawEllipse(node.x * width() - circleSize / 2,
                                node.y * height() - circleSize / 2,
                                circleSize, circleSize); break;
        case KP::BOSS:
            painter.drawEllipse(node.x * width() - circleSize,
                                node.y * height() - circleSize,
                                circleSize * 2, circleSize * 2); break;
        }
    }
}
