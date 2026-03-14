/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LOCALEAWARESORT_H
#define LOCALEAWARESORT_H

#include <QObject>
#include <QSortFilterProxyModel>

class LocaleAwareSort : public QSortFilterProxyModel
{
public:
    explicit LocaleAwareSort(QObject *parent = nullptr);

private:
    virtual bool lessThan(
        const QModelIndex &source_left,
        const QModelIndex &source_right) const override;
};

#endif // LOCALEAWARESORT_H
