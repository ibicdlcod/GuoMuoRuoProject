#include "mapdetail.h"
#include "ui_mapdetail.h"

#include <QApplication>
#include <QColor>
#include <QDebug>
#include <QFont>
#include <QGuiApplication>
#include <QHash>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleHints>
#include <Qt>

#include "../../clientv2.h"
#include "../mainwindow.h"
#include "maprender.h"

/* Resource filenames are kept ASCII-only so AutoRcc and runtime lookups
 * work on non-UTF-8 Windows codepages (e.g. GBK), where a non-ASCII path
 * is mangled to '?' and the file is not found. Transliterate the localized
 * map name to ASCII for the on-disk lookup; the displayed name is unchanged. */
static QString asciiResourceName(const QString &name) {
    static const QHash<QChar, QString> special = {
        {QChar(0x00D8), QStringLiteral("O")},  // Ø
        {QChar(0x00F8), QStringLiteral("o")},  // ø
        {QChar(0x00C6), QStringLiteral("AE")}, // Æ
        {QChar(0x00E6), QStringLiteral("ae")}, // æ
        {QChar(0x0110), QStringLiteral("D")},  // Đ
        {QChar(0x0111), QStringLiteral("d")},  // đ
        {QChar(0x0141), QStringLiteral("L")},  // Ł
        {QChar(0x0142), QStringLiteral("l")},  // ł
        {QChar(0x00DF), QStringLiteral("ss")}, // ß
    };
    /* NFKD splits accented letters into base + combining mark; we keep the
     * ASCII base, drop combining marks, and map a few stroke/ligature
     * letters that do not decompose. */
    const QString decomposed = name.normalized(QString::NormalizationForm_KD);
    QString result;
    for(const QChar &c: decomposed) {
        if(special.contains(c)) {
            result.append(special.value(c));
        }
        else if(c.unicode() < 0x80) {
            result.append(c);
        }
    }
    return result;
}

static QColor getIconColor() {
    if(QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        return Qt::white;
    } else {
        return Qt::black;
    }
}

static QColor getRudderColor() {
    if(QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        return QColor(255, 255, 128); // bright yellow for dark mode
    } else {
        return QColor(128, 128, 0);   // dark yellow for light mode
    }
}

MapDetail::MapDetail(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MapDetail) {
    ui->setupUi(this);
    antialiased = true;

    QColor iconColor = getIconColor();
    rudder = recolorImage(":/resources/Image/rudder.png", getRudderColor());
    airNodeIcon = recolorImage(":/resources/Image/plane.png", iconColor);
    carrierFleetIcon = recolorImage(
        ":/resources/Image/fleetIcons/carrier.png", QColor(0, 192, 0));
    surfaceFleetIcon = recolorImage(
        ":/resources/Image/fleetIcons/battleship.png", QColor(0, 192, 0));
    transportFleetIcon = recolorImage(
        ":/resources/Image/fleetIcons/transport.png", QColor(0, 192, 0));
    normalFleetIcon = recolorImage(
        ":/resources/Image/fleetIcons/normal.png", QColor(0, 192, 0));

    /* https://stackoverflow.com/questions/43428627/
     * applying-qpropertyanimation-to-qrect */
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
    // Ensure the new pixmap is transparent initially
    newImage.fill(Qt::transparent);

    QPainter painter(&newImage);
    // Draw the source image to use its alpha channel
    painter.drawPixmap(0, 0, QPixmap::fromImage(image));
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    // Fill with yellow (R: 255, G: 255, B: 0)
    painter.fillRect(newImage.rect(), color);
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
    geographicalMap = QPixmap();

    QString enName = map->localNames.value("en_US");
    enName.replace(QLatin1String(" "), QLatin1String("_"));
    enName.remove(QLatin1Char('\''));
    enName = asciiResourceName(enName);
    QString path = QString(":/resources/map/geographical/map%1_%2.png")
                       .arg(map->id).arg(enName);
    QImage geoImage(path);
    if (geoImage.isNull()) {
        return;
    }
    if (geoImage.format() != QImage::Format_ARGB32) {
        geoImage = geoImage.convertToFormat(QImage::Format_ARGB32);
    }
    for (int y = 0; y < geoImage.height(); ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(geoImage.scanLine(y));
        for (int x = 0; x < geoImage.width(); ++x) {
            QRgb pixel = line[x];
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            if (b > r + 10 && b > g + 10) {
                line[x] = qRgba(r, g, b, 0);
            }
        }
    }
    geographicalMap = QPixmap::fromImage(geoImage);

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
    awaitingChoice = !nodeIds.isEmpty();
    update();
}

 void MapDetail::mousePressEvent(QMouseEvent *event) {
    if (!mapPointer) {
        QWidget::mousePressEvent(event);
        return;
    }
    
    /* Handle CHOICE node branch selection */
    if (awaitingChoice) {
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
        QWidget::mousePressEvent(event);
        return;
    }
    
    /* Expedition mode: allow clicking any node to plan battle */
    if (expeditionMode) {
        for(const auto &[nodeId, node]: mapPointer->nodes.asKeyValueRange()) {
            QPointF nodePos(node.x * width(), node.y * height());
            if(QLineF(event->position(), nodePos).length() <= circleSize * 1.5) {
                emit nodeClicked(nodeId);
                return;
            }
        }
    }
    
    QWidget::mousePressEvent(event);
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
    if (!mapPointer) {
        return;
    }
    QPainter painter(this);
    if (antialiased) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
    }

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
                               rudder.scaled(
                                   QSize(circleSize * 2, circleSize * 2),
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
            painter.drawEllipse(node.x * width() - circleSize / 2,
                                node.y * height() - circleSize / 2,
                                circleSize, circleSize);
        } break;
        case KP::DISASTER: {
            painter.setBrush(QBrush(Qt::gray));
            QPen pen(QColor(96, 96, 96));
            pen.setWidth(circleBorderSize);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize / 2,
                                node.y * height() - circleSize / 2,
                                circleSize, circleSize);
        } break;
        case KP::CHOICE: {
            painter.setBrush(QBrush(QColor(255, 220, 0)));
            QPen pen(QColor(200, 140, 0));
            pen.setWidth(circleBorderSize);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize / 2,
                                node.y * height() - circleSize / 2,
                                circleSize, circleSize);
        } break;
        case KP::AIR: {
            painter.setBrush(redBrush);
            QPen pen(Qt::red);
            pen.setWidth(circleBorderSize);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize / 2,
                                node.y * height() - circleSize / 2,
                                circleSize, circleSize);
            painter.drawPixmap(node.x * width() - circleSize / 2,
                               node.y * height() - 3 * circleSize / 2,
                               airNodeIcon.scaled(
                                   QSize(circleSize, circleSize),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation));
        } break;
        case KP::NIGHT: {
            painter.setBrush(QBrush(QColor(210, 128, 255)));
            QPen pen(QColor(128, 0, 200));
            pen.setWidth(circleBorderSize);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize / 2,
                                node.y * height() - circleSize / 2,
                                circleSize, circleSize);
        } break;
        case KP::NIGHTBOSS: {
            painter.setBrush(QBrush(QColor(210, 128, 255)));
            QPen pen(QColor(128, 0, 200));
            pen.setWidth(circleBorderSize * 2);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize,
                                node.y * height() - circleSize,
                                circleSize * 2, circleSize * 2);
        } break;
        case KP::TRANSPORT: {
            painter.setBrush(QBrush(Qt::green));
            QPen pen(QColor(64, 192, 0));
            pen.setWidth(circleBorderSize);
            painter.setPen(pen);
            painter.drawEllipse(node.x * width() - circleSize / 2,
                                node.y * height() - circleSize / 2,
                                circleSize, circleSize);
        } break;
        default: break;
        }
        if (expeditionMode && plannedNodeIds.contains(id)) {
            painter.setPen(QColor(0, 255, 0)); // green color
            QFont font = painter.font();
            font.setPixelSize(circleSize * 0.8);
            font.setBold(true);
            painter.setFont(font);
            QRectF checkRect(node.x * width() - circleSize / 2,
                             node.y * height() - circleSize / 2,
                             circleSize, circleSize);
            painter.drawText(checkRect, Qt::AlignCenter, "\u2713"); // ✓
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
    if (!expeditionMode) {
        QPixmap *icon = nullptr;
        switch(currentFleetType) {
        case KP::NormalFleet: icon = &normalFleetIcon; break;
        case KP::CarrierFleet: icon = &carrierFleetIcon; break;
        case KP::SurfaceFleet: icon = &surfaceFleetIcon; break;
        case KP::TransportFleet: icon = &transportFleetIcon; break;
        default: break;
        }
        if (icon) {
            painter.drawPixmap(fleetCenter.x() * width() - circleSize * 1.5,
                               fleetCenter.y() * height() - circleSize * 1.5,
                               icon->scaled(QSize(circleSize * 3, circleSize * 3),
                                            Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation
                                            ));
        }
    }

    if (!geographicalMap.isNull()) {
        painter.drawPixmap(rect(), geographicalMap);
    }

    if(animation->currentValue() == animation->endValue()) {
        emit moveFinished();
    }
}

void MapDetail::setExpeditionMode(bool expedition) {
    expeditionMode = expedition;
    update();
}

void MapDetail::setPlannedNodes(const QSet<int> &nodeIds) {
    plannedNodeIds = nodeIds;
    update();
}
