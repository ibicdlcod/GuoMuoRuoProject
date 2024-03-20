#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QToolButton>
#include "../equipmodel.h"

class Navi : public QObject {
    Q_OBJECT

public:
    explicit Navi(QHBoxLayout *layout, EquipModel *model);

    void enactPageNumChange(int currentPageNum, int totalPageNum);

private:
    QComboBox *typebox;
    QToolButton *firstbutton;
    QToolButton *prevbutton;
    QLabel *pageLabel;
    QToolButton *nextbutton;
    QToolButton *lastbutton;
    EquipModel *model;
};
#endif // NAVIGATOR_H
