/****************************************************************************
**   ncs is the backend's server of nodecast
**   Copyright (C) 2010-2013  Frédéric Logier <frederic@logier.org>
**
**   https://github.com/nodecast/ncs
**
**   This program is free software: you can redistribute it and/or modify
**   it under the terms of the GNU Affero General Public License as
**   published by the Free Software Foundation, either version 3 of the
**   License, or (at your option) any later version.
**
**   This program is distributed in the hope that it will be useful,
**   but WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**   GNU Affero General Public License for more details.
**
**   You should have received a copy of the GNU Affero General Public License
**   along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include <QObject>
#include <QStateMachine>
#include <QSharedPointer>
#include <QThreadPool>
#include <QRunnable>



class workflow_statemachine : public QObject
{
    Q_OBJECT
public:
    explicit workflow_statemachine();
    ~workflow_statemachine();


private:
   //  QStateMachine *machine;


public slots:
    void spawn_workflow(QString session);
};


//typedef QSharedPointer<workflow_statemachine> workflow_statemachine_Ptr;

class Statemachine : public QObject
{
    Q_OBJECT
public:
    explicit Statemachine(QObject *parent = 0);
    ~Statemachine();
    void init();

signals:
    void invoke(QString session);


private:
  //  QHash<QString, workflow_statemachine_Ptr> workflow_statemachine_thread;
};

#endif // STATEMACHINE_H
