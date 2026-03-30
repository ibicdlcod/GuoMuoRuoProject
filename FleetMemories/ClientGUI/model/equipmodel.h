/* Copyright (C) 2026 Harusoft Ltd.
 * SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef EQUIPMODEL_H
#define EQUIPMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QUuid>
#include "../../Protocol/equipment.h"

class EquipModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EquipModel(QObject *parent = nullptr, bool isInArsenal = true);

    std::tuple<Equipment *, int> getEquip(QUuid);
    QHash<QUuid, Equipment *> & getClientEquips();
    QHash<QUuid, int> & getClientEquipStars();

signals:
    void sortReversedChanged(bool);
    void destructRequest(const QList<QUuid> &);
    void needReCalculateRows();
    void needReCalculatePages();
    void pageNumChanged(int currentPageNum, int totalPageNum);
    void equipModified(QUuid shipUid,
                       int equipSlotIndex,
                       QUuid equipUid);
    void equipReady();
    void demandSkillPoints(int equipDef);
    void improveRequest(const QList<QUuid> &equips);

public slots:
    virtual void switchDisplayType(int) final;
    virtual void switchDisplayType2(const QString &) final;
    virtual void firstPage();
    virtual void prevPage();
    virtual void nextPage();
    virtual void lastPage();
    virtual void setSortMode(int);
    virtual void setSortReversed(bool);
    virtual void addEquipment(QUuid, int) final;
    virtual void enactDestruct() final;
    virtual void enactModernize();
    virtual void destructedEquipment(const QList<QUuid> &) final;
    virtual void updateEquipmentList(const QJsonObject &) final;
    virtual void setPageNumHint(int);
    virtual void setRowsPerPageHint(int);
    virtual void setIsInArsenal(bool);
    virtual void wholeTableChanged();
    virtual void setShipEquip(QUuid ship, int slotPos, QUuid equip) final;
    virtual QUuid getShipEquip(QUuid ship, int slotPos) final;
    virtual std::tuple<QUuid, int> getEquipShip(QUuid equip) final;
    virtual void filterByShip(Ship *ship, bool isSlotEX) final;
    virtual void processSkillPointInfo(int equipDef, int skillPoint) final;
    virtual void modernizedEquips(const QList<std::tuple<QUuid, int>> &) final;

public:
    virtual int rowCount(const QModelIndex &parent
                         = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent
                            = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool setData(const QModelIndex &index,
                         const QVariant &value,
                         int role = Qt::EditRole) override;
    static const int uidCol = 0;
    static const int equipCol = 1;
    static const int starCol = 2;
    static const int SortByEquipDef = 0;
    static const int SortByUuid     = 1;
    static const int SortByName     = 2;
    static const int SortByStar     = 3;
    static const int SortByPrimAttr = 4;
    static const int SortBySkill    = 5;
    virtual int destructColumn() const final;
    virtual int hiddenSortColumn() const;
    virtual int selectColumn() const;
    virtual int hpColumn() const;
    virtual int fleetPosColumn() const;
    virtual int currentPageNum() const;
    virtual int maximumPageNum() const;
    virtual bool isReady() const;
    virtual void unsetShip() final;

protected:
    virtual bool defaultDescending(int mode) const;
    virtual void adjustRowCount(int oldRowCount, int newRowCount);
    virtual void customSort();
    virtual int numberOfColumns() const;
    bool isInArsenal;

    int rowsPerPage = 1;
    int pageNum = 0;
    int sortMode = SortByEquipDef;
    bool sortReversed = false;
    bool ready = false;
    bool isEquipModel = true;

    virtual int numberOfEquip() const;
    static const int attrCol = 3;

    QHash<QUuid, Equipment *> clientEquips;
    QHash<QUuid, int> clientEquipStars;
    QList<QUuid> sortedEquipIds; // not sort by uuid but equiptype
    QHash<QUuid, bool> isDestructChecked;

    QMap<QUuid, QList<QUuid>> shipEquips;
    QMap<QUuid, std::tuple<QUuid, int>> shipEquipReverse;
    Ship * currentActiveShip = nullptr;
    bool currentActiveSlotEx = false;
    QHash<QUuid, bool> isModernizationChecked;

protected slots:
    virtual void updateIllegalPage();
    virtual void clearCheckBoxes();

private:
    QObject *mainWindow = nullptr;
    QMap<int, int> skillPointReg;
};

#endif // EQUIPMODEL_H
