#include "mapdetail.h"
#include "ui_mapdetail.h"

#include <QMouseEvent>
#include <QPainter>
#include <QStyleHints>

#include "../../clientv2.h"
#include "../mainwindow.h"
#include "maprender.h"

MapDetail::MapDetail(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MapDetail) {
    ui->setupUi(this);
    antialiased = true;

    rudder = recolorImage(":/Assets/Image/rudder.png", QColor(192, 192, 0));
    carrierFleetIcon = recolorImage(":/Assets/Image/fleetIcons/carrier.png", QColor(0, 192, 0));
    surfaceFleetIcon = recolorImage(":/Assets/Image/fleetIcons/battleship.png", QColor(0, 192, 0));
    transportFleetIcon = recolorImage(":/Assets/Image/fleetIcons/transport.png", QColor(0, 192, 0));
    normalFleetIcon = recolorImage(":/Assets/Image/fleetIcons/normal.png", QColor(0, 192, 0));

    /* https://stackoverflow.com/questions/43428627/applying-qpropertyanimation-to-qrect */
    animation = new QPropertyAnimation(this, "fleetcenter");
    connect(animation, &QPropertyAnimation::valueChanged, this, [this](){
        update();
    });
}

MapDetail::~MapDetail() {
    delete ui;
}

QPixmap MapDetail::recolorImage(const QString &filename, const QColor &color) {
    QImage image(filename);
    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    QPixmap newImage(image.size()); // Create a new, empty pixmap
    newImage.fill(Qt::transparent); // Ensure the new pixmap is transparent initially

    QPainter painter(&newImage);
    painter.drawPixmap(0, 0, QPixmap::fromImage(image)); // Draw the source image to use its alpha channel
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(newImage.rect(), color); // Fill with yellow (R: 255, G: 255, B: 0)
    painter.end();
    return newImage;
}

QPointF MapDetail::getFleetCenter() const {
    return fleetCenter;
}

void MapDetail::setFleetCenter(const QPointF &input) {
    if(input == fleetCenter) {
        return;
    }
    fleetCenter = input;
    emit fleetCenterChanged();
}

void MapDetail::displayDetailedMap(Map *map) {
    mapPointer = map;
    for(auto *widget: QApplication::topLevelWidgets()) {
        if(qobject_cast<MainWindow *>(widget)) {
            MainWindow *mainWindowM = qobject_cast<MainWindow *>(widget);
            auto fv = mainWindowM->getFleetArea();
            if(fv) {
                currentFleetType = fv->getCurrentFleetType();
                return;
            }
        }
    }
    /* default */
    currentFleetType = KP::NormalFleet;
}

void MapDetail::setChoiceNodes(const QList<int> &nodeIds) {
    choiceNodeIds = nodeIds;
    awaitingChoice = true;
    update();
}

void MapDetail::mousePressEvent(QMouseEvent *event) {
    if(!awaitingChoice) {
        QWidget::mousePressEvent(event);
        return;
    }
    for(int nodeId: std::as_const(choiceNodeIds)) {
        if(!mapPointer->nodes.contains(nodeId))
            continue;
        const MapNode &node = mapPointer->nodes[nodeId];
        QPointF nodePos(node.x * width(), node.y * height());
        if(QLineF(event->position(), nodePos).length() <= circleSize * 1.5) {
            awaitingChoice = false;
            choiceNodeIds.clear();
            update();
            emit nodeClicked(nodeId);
            return;
        }
    }
}

void MapDetail::changeCurrentNode(const MapNode &node) {
    if(uninitialized) {
        currentNode = node;
        setFleetCenter(QPointF(currentNode.x, currentNode.y));
        update();
        uninitialized = false;
    }
    else {
        //animation->setEasingCurve(QEasingCurve::InBack);
        animation->setDuration(1000); // in milliseconds
        animation->setStartValue(QPointF(currentNode.x,
                                         currentNode.y));
        animation->setEndValue(QPointF(node.x,
                                       node.y));
        animation->start();
        currentNode = node;
    }
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
            pen.setWidth(circleBorderSize);

            static QList<qreal> dashes = {4, 6};
            pen.setDashPattern(dashes);

            painter.setPen(pen);
            auto delta = nextNodePos - thisNodePos;
            delta /= std::hypot((double)delta.x(), (double)delta.y());
            painter.drawLine(thisNodePos + delta * circleSize * 1.5,
                             nextNodePos - delta * circleSize * 1.5);
        }
    }

    for(const auto &[id, node]: mapPointer->nodes.asKeyValueRange()) {
        static QBrush redBrush = QBrush(QColor(255,128,128));
        painter.setBrush(redBrush);
        QPen pen(Qt::red);
        pen.setWidth(circleBorderSize);
        switch(node.type) {
        case KP::STARTING:
            painter.setPen(pen);
            painter.drawPixmap(node.x * width() - circleSize,
                               node.y * height() - circleSize,
                               rudder.scaled(QSize(circleSize * 2, circleSize * 2),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation
                                             )); break;
        case KP::NORMAL:
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize / 2,
                                node.y * height() - circleSize / 2,
                                circleSize, circleSize); break;
        case KP::BOSS:
            pen.setWidth(circleBorderSize * 2);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize,
                                node.y * height() - circleSize,
                                circleSize * 2, circleSize * 2); break;
        case KP::EMPTY: {
            painter.setBrush(QBrush(Qt::cyan));
            QPen pen(QColor(0, 128, 255));
            pen.setWidth(circleBorderSize);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize,
                                node.y * height() - circleSize,
                                circleSize * 2, circleSize * 2);
        } break;
        case KP::CHOICE: {
            painter.setBrush(QBrush(QColor(255, 220, 0)));
            QPen pen(QColor(200, 140, 0));
            pen.setWidth(circleBorderSize);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize,
                                node.y * height() - circleSize,
                                circleSize * 2, circleSize * 2);
        } break;
        }
    }
    if(awaitingChoice) {
        for(int nodeId: std::as_const(choiceNodeIds)) {
            if(!mapPointer->nodes.contains(nodeId))
                continue;
            const MapNode &n = mapPointer->nodes[nodeId];
            QPen pen(QColor(255, 220, 0));
            pen.setWidth(circleBorderSize * 2);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(n.x * width() - circleSize * 1.5,
                                n.y * height() - circleSize * 1.5,
                                circleSize * 3, circleSize * 3);
        }
    }
    QPixmap *icon;
    switch(currentFleetType) {
    case KP::NormalFleet: icon = &normalFleetIcon; break;
    case KP::CarrierFleet: icon = &carrierFleetIcon; break;
    case KP::SurfaceFleet: icon = &surfaceFleetIcon; break;
    case KP::TransportFleet: icon = &transportFleetIcon; break;
    }

    painter.drawPixmap(fleetCenter.x() * width() - circleSize * 1.5,
                       fleetCenter.y() * height() - circleSize * 1.5,
                       icon->scaled(QSize(circleSize * 3, circleSize * 3),
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation
                                    ));
    if(animation->currentValue() == animation->endValue()) {
        emit moveFinished();
    }
}
