/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef RESOURCEGAINVIEW_H
#define RESOURCEGAINVIEW_H

#include <QJsonObject>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QWidget>

class ResourceGainView : public QWidget {
    Q_OBJECT

public:
    explicit ResourceGainView(QWidget *parent = nullptr);

public slots:
    void populate(const QJsonObject &content);

private slots:
    void refreshVerticalHeaders();

private:
    QSortFilterProxyModel *proxyModel;
    QTableView *table;
    QStandardItemModel *model;
};

#endif // RESOURCEGAINVIEW_H
