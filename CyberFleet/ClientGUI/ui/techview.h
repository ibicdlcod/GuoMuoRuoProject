#ifndef TECHVIEW_H
#define TECHVIEW_H

#include <QFrame>

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
    Ui::TechView *ui;
};

#endif // TECHVIEW_H
