#ifndef RANKMODEL_H
#define RANKMODEL_H

#include <QObject>
#include "equipmodel.h"
#include "steam/steamclientpublic.h"
#include "steam/isteamfriends.h"

class RankModel : public EquipModel
{
    Q_OBJECT
public:
    explicit RankModel(QObject *parent = nullptr);
    virtual QVariant data(const QModelIndex &index,
                          int role = Qt::DisplayRole) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role = Qt::DisplayRole) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual bool setData(const QModelIndex &index,
                         const QVariant &value,
                         int role = Qt::EditRole) override;

public slots:
    virtual void updateList(const QJsonArray &, int);

protected:
    virtual int numberOfEquip() const override;
    virtual int numberOfColumns() const override;

private slots:
    void pageNumChange(int currentPageNum, int totalPageNum);

private:
    QMap<CSteamID, QString> names;
    QMap<CSteamID, double> previousExp;
    QMap<CSteamID, double> currentExp;
    QHash<int, CSteamID> currentDisplayed;

    int totalUsers = 1;

    int nameCol = 0;
    int cvpCol = 1;
    int pvpCol = 2;
    int peCol = 3;

    STEAM_CALLBACK(RankModel, OnPersonaStateChangeHandler, PersonaStateChange_t);
};

#endif // RANKMODEL_H
