#ifndef EQUIPMODEL_H
#define EQUIPMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QUuid>
#include "../Protocol/equipment.h"

class EquipModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit EquipModel(QObject *parent = nullptr, bool isInArsenal = true);

signals:
    void destructRequest(const QList<QUuid> &);
    void needReCalculateRows();
    void needReCalculatePages();
    void pageNumChanged(int currentPageNum, int totalPageNum);

public slots:
    virtual void switchDisplayType(int) final;
    virtual void switchDisplayType2(const QString &) final;
    virtual void firstPage();
    virtual void prevPage();
    virtual void nextPage();
    virtual void lastPage();
    virtual void addEquipment(QUuid, int) final;
    virtual void enactDestruct() final;
    virtual void destructEquipment(const QList<QUuid> &) final;
    virtual void updateEquipmentList(const QJsonObject &) final;
    virtual void setPageNumHint(int);
    virtual void setRowsPerPageHint(int);
    virtual void setIsInArsenal(bool);
    virtual void wholeTableChanged();

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
    virtual int destructColumn() const final;
    virtual int addStarColumn() const;
    virtual int hiddenSortColumn() const;
    virtual int selectColumn() const;
    virtual int hpColumn() const;
    virtual int currentPageNum() const;
    virtual int maximumPageNum() const;
    virtual bool isReady() const;

protected:
    virtual void adjustRowCount(int oldRowCount, int newRowCount);
    virtual void customSort();
    virtual int numberOfColumns() const;
    bool isInArsenal;

    int rowsPerPage = 1;
    int pageNum = 0;
    bool ready = false;

protected slots:
    virtual void updateIllegalPage();

private slots:
    void clearCheckBoxes();

private:
    int numberOfEquip() const;
    static const int attrCol = 3;

    QHash<QUuid, Equipment *> clientEquips;
    QHash<QUuid, int> clientEquipStars;
    QList<QUuid> sortedEquipIds; // not sort by uuid but equiptype
    QHash<QUuid, bool> isDestructChecked;
};

#endif // EQUIPMODEL_H
