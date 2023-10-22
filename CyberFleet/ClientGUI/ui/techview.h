#ifndef TECHVIEW_H
#define TECHVIEW_H

#include <QFrame>
#include <QTableWidgetItem>

namespace Ui {
class TechView;
}

class TechView : public QFrame
{
    Q_OBJECT

public:
    explicit TechView(QWidget *parent = nullptr);
    ~TechView();
    void resetLocalListName(int);

public slots:
    void demandLocalTech(int);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateGlobalTech(const QJsonObject &);
    void updateGlobalTechViewTable(const QJsonObject &);
    void updateLocalTech(const QJsonObject &);
    void updateLocalTechViewTable(const QJsonObject &);

private:
    void resizeColumns(bool);

    Ui::TechView *ui;
};

class TableWidgetItemNumber: public QTableWidgetItem {

public:
    explicit TableWidgetItemNumber(double);
    virtual bool operator<(const QTableWidgetItem &other) const override {
        return this->text().toDouble() < other.text().toDouble();
    }
};

#endif // TECHVIEW_H
