#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QLabel>

class ElidedLabel : public QLabel
{
    Q_OBJECT

public:
    using QLabel::QLabel;

protected:
    void paintEvent(QPaintEvent *e) override;
};

#endif // ELIDEDLABEL_H
