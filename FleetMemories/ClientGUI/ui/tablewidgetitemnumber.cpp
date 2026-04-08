/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#include "tablewidgetitemnumber.h"

TableWidgetItemNumber::TableWidgetItemNumber(double content)
{
    QTableWidgetItem::setText(QString::number(content));
}