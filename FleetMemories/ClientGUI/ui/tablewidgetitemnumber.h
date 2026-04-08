/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef TABLEWIDGETITEMNUMBER_H
#define TABLEWIDGETITEMNUMBER_H

#include <QTableWidgetItem>

class TableWidgetItemNumber: public QTableWidgetItem {
public:
    explicit TableWidgetItemNumber(double content);
    virtual bool operator<(const QTableWidgetItem &other) const override {
        return this->text().toDouble() < other.text().toDouble();
    }
};

#endif // TABLEWIDGETITEMNUMBER_H