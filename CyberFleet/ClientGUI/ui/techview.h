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

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateGlobalTech(const QJsonObject &);
    void updateGlobalTechViewTable(const QJsonObject &);

private:
    void resizeColumns();

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
