#include "localeawaresort.h"

LocaleAwareSort::LocaleAwareSort(QObject *parent)
    : QSortFilterProxyModel{parent}
{}

bool LocaleAwareSort::lessThan(
    const QModelIndex &source_left,
    const QModelIndex &source_right) const {
    return sourceModel()->data(source_left).toString()
               .localeAwareCompare(sourceModel()->data(source_right).toString()) < 0;
}
