/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "resourcegainview.h"

#include <QFont>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include "../../../Protocol/kp.h"
#include "../../clientv2.h"

namespace {

/* Proxy that keeps source row 0 (the Total row) pinned to position 0
 * regardless of sort column or direction. */
class PinnedRowSortProxy : public QSortFilterProxyModel {
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;
protected:
    bool lessThan(const QModelIndex &left,
                  const QModelIndex &right) const override {
        if(left.row() == 0 && right.row() != 0)
            return sortOrder() == Qt::AscendingOrder;
        if(right.row() == 0 && left.row() != 0)
            return sortOrder() == Qt::DescendingOrder;
        return QSortFilterProxyModel::lessThan(left, right);
    }
};

/* 6.2-supremacy.md#resource_gain */
const QList<QString> resAttrs = {
    QStringLiteral("O"),
    QStringLiteral("E"),
    QStringLiteral("S"),
    QStringLiteral("R"),
    QStringLiteral("A"),
    QStringLiteral("W"),
    QStringLiteral("C")
};

QString resAttrHeader(const QString &attr) {
    /* the icon suffices */
    return attr;
}

QString resAttrIcon(const QString &attr) {
    if(attr == QStringLiteral("O"))
        return QStringLiteral(":/resources/resord/oil.png");
    if(attr == QStringLiteral("E"))
        return QStringLiteral(":/resources/resord/explosive.png");
    if(attr == QStringLiteral("S"))
        return QStringLiteral(":/resources/resord/steel.png");
    if(attr == QStringLiteral("R"))
        return QStringLiteral(":/resources/resord/rubber.png");
    if(attr == QStringLiteral("A"))
        return QStringLiteral(":/resources/resord/aluminum.png");
    if(attr == QStringLiteral("W"))
        return QStringLiteral(":/resources/resord/tungsten.png");
    if(attr == QStringLiteral("C"))
        return QStringLiteral(":/resources/resord/chromium.png");
    return {};
}

}

ResourceGainView::ResourceGainView(QWidget *parent)
    : QWidget(parent) {
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);

    //% "Resource gain per minute from naval supremacy, broken down by territory"
    auto *hint = new QLabel(qtTrId("res-gain-dialog-hint"), this);
    hint->setWordWrap(true);
    hint->setAlignment(Qt::AlignCenter);
    QFont hintFont = hint->font();
    hintFont.setPointSizeF(hintFont.pointSizeF() * 1.5);
    hint->setFont(hintFont);
    layout->addWidget(hint);

    model = new QStandardItemModel(0, resAttrs.size() + 1, this);
    //% "Supremacy"
    model->setHorizontalHeaderItem(
        0, new QStandardItem(qtTrId("res-gain-col-supremacy")));
    for(int col = 0; col < resAttrs.size(); ++col) {
        auto *item = new QStandardItem(resAttrHeader(resAttrs[col]));
        item->setIcon(QIcon(resAttrIcon(resAttrs[col])));
        model->setHorizontalHeaderItem(col + 1, item);
    }

    table = new QTableView(this);
    proxyModel = new PinnedRowSortProxy(this);
    proxyModel->setSourceModel(model);
    proxyModel->setSortRole(Qt::UserRole);
    table->setModel(proxyModel);
    table->setSortingEnabled(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setSizePolicy(
        QSizePolicy::Preferred, QSizePolicy::Expanding);
    table->setSizeAdjustPolicy(
        QAbstractScrollArea::AdjustToContents);
    table->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    table->verticalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    layout->addWidget(table, 1, Qt::AlignHCenter);

    Client &engine = Client::getInstance();
    connect(&engine, &Client::mapRegistryComplete,
            this, &ResourceGainView::refreshVerticalHeaders);
}

void ResourceGainView::populate(const QJsonObject &content) {
    model->removeRows(0, model->rowCount());
    Client &engine = Client::getInstance();

    /* Row 0 is the pinned Total row; insert it first so territory rows
     * start at 1 and the proxy always keeps row 0 at the top. */
    model->insertRow(0);
    //% "Total"
    model->setVerticalHeaderItem(
        0, new QStandardItem(qtTrId("res-gain-total")));
    auto *supTotalItem = new QStandardItem(QStringLiteral("-"));
    supTotalItem->setData(0.0, Qt::UserRole);
    supTotalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    model->setItem(0, 0, supTotalItem);

    QMap<QString, double> totals;
    for(const QString &attr : resAttrs) {
        totals[attr] = 0.0;
    }

    int row = 1;
    for(auto it = content.constBegin();
        it != content.constEnd();
        ++it, ++row) {
        model->insertRow(row);
        int mapId = it.key().toInt();
        QString mapName;
        if(engine.mapRegistryCache.contains(mapId)) {
            mapName = engine.mapRegistryCache[mapId]->toString();
        }
        else {
            mapName = QString::number(mapId);
        }
        model->setVerticalHeaderItem(
            row, new QStandardItem(mapName));
        QJsonObject resources = it.value().toObject();
        double supremacy = resources[
            QStringLiteral("supremacy")].toDouble();
        auto *supItem = new QStandardItem(
            QString::number(supremacy, 'f', 1) + "%");
        supItem->setData(supremacy, Qt::UserRole);
        supItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        model->setItem(row, 0, supItem);
        for(int col = 0; col < resAttrs.size(); ++col) {
            double gain = resources.value(resAttrs[col]).toDouble();
            totals[resAttrs[col]] += gain;
            auto *item = new QStandardItem(
                QString::number(gain, 'f', 2));
            item->setData(gain, Qt::UserRole);
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            model->setItem(row, col + 1, item);
        }
    }

    for(int col = 0; col < resAttrs.size(); ++col) {
        double total = totals[resAttrs[col]];
        auto *item = new QStandardItem(
            QString::number(total, 'f', 2));
        item->setData(total, Qt::UserRole);
        item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        model->setItem(0, col + 1, item);
    }

    table->updateGeometry();
}

void ResourceGainView::refreshVerticalHeaders() {
    Client &engine = Client::getInstance();
    for(int row = 0; row < model->rowCount(); ++row) {
        auto *headerItem = model->verticalHeaderItem(row);
        if(!headerItem) {
            continue;
        }
        bool ok;
        int mapId = headerItem->text().toInt(&ok);
        if(!ok) {
            continue;
        }
        if(engine.mapRegistryCache.contains(mapId)) {
            headerItem->setText(
                engine.mapRegistryCache[mapId]->toString());
        }
    }
}
