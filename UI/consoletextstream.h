#ifndef CONSOLETEXTSTREAM_H
#define CONSOLETEXTSTREAM_H

#include <QTextStream>

#ifdef Q_OS_WIN
#include <windows.h>
#include <iostream>
#endif

#include <QIODevice>
#include <QSocketNotifier>

/* usage of QTextStreamManipulator on this class is prohibited throughout due to unfixable bugs,
 * that is, qout << qSetFieldWidth result in QTextStream & rather than this class.
 */
class ConsoleTextStream : public QTextStream
{
public:
    ConsoleTextStream();
    ConsoleTextStream& operator<<(QString);
    ConsoleTextStream& operator<<(int);
    void setFieldWidth(int width);
    void setFieldAlignment(QTextStream::FieldAlignment alignment);
};

ConsoleTextStream & operator<<(ConsoleTextStream &s, QTextStreamManipulator &input);

class ConsoleInput : public QObject
{
    Q_OBJECT
public:
    explicit ConsoleInput(QObject *parent = nullptr);
    QString readlineOld();

signals:
    void textReceived(QString);
public slots:
    void readline();

private:
    QTextStream stream;
    QSocketNotifier notifier;
};

#endif // CONSOLETEXTSTREAM_H
