#ifndef MAIN_H
#define MAIN_H


#include <QSettings>
#include <QxtCore/QxtCommandOptions>
#include <qstring.h>
#include <qtextstream.h>
#include <qsocketnotifier.h>
#include <QxtJSON>
//#include <QxtNetwork>
#include <QDateTime>
#include <iostream>
#include <unistd.h>
#include <QFile>
#include <QThread>
#include <QTextStream>
#include "service.h"


class Io : public QObject
{
    Q_OBJECT

public:
    Io();
    ~Io();

private slots:
    void readStdin();

private:
    QFile *io_log;
    QSocketNotifier* notifier;
    QTextStream* input;

signals:
    void parseData(QString s);

};



class NCWhandler : public QObject
{
    Q_OBJECT

public:
    NCWhandler();
    ~NCWhandler();

public slots:
    void push_payload(QString s);
    void writeStdout(QString s);

signals:
    void emit_payload(QVariant payload);

private:
    QFile *handler_log;
    QTextStream* output;
};




#endif // MAIN_H
