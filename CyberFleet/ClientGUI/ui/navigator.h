#ifndef NAVIGATOR_H
#define NAVIGATOR_H

#include <QIcon>
#include <QToolButton>
#include <QHBoxLayout>

void addNavigator(QHBoxLayout *layout) {
QIcon first = QIcon(":/resources/navigation/first.svg");
QIcon last = QIcon(":/resources/navigation/last.svg");
QIcon prev = QIcon(":/resources/navigation/prev.svg");
QIcon next = QIcon(":/resources/navigation/next.svg");

QToolButton *firstbutton = new QToolButton();
QToolButton *lastbutton = new QToolButton();
QToolButton *prevbutton = new QToolButton();
QToolButton *nextbutton = new QToolButton();

firstbutton->setIcon(first);
lastbutton->setIcon(last);
prevbutton->setIcon(prev);
nextbutton->setIcon(next);

layout->addWidget(firstbutton);
layout->addWidget(prevbutton);
layout->addWidget(nextbutton);
layout->addWidget(lastbutton);

}

#endif // NAVIGATOR_H
