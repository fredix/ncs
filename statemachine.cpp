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

#include "statemachine.h"

Statemachine::Statemachine(QObject *parent) :
    QObject(parent)
{
}


Statemachine::~Statemachine()
{
}

void Statemachine::init()
{

}


//void Statemachine::invoke(QString session)
//{
    //workflow_statemachine *workflow = new workflow_statemachine(session);

    //QThreadPool::globalInstance()->start(workflow);

/*
    QThread thread_workflow = new QThread(this);
    workflow_statemachine_thread[session] = QSharedPointer<workflow_statemachine> (new workflow_statemachine(session), &QObject::deleteLater);
    connect(thread_workflow, SIGNAL(started()), workflow_statemachine_thread[session].data(), SLOT(init()));
    connect(workflow_statemachine_thread[session].data(), SIGNAL(destroyed()), thread_workflow, SLOT(quit()), Qt::DirectConnection);

    workflow_statemachine_thread[session]->moveToThread(thread_workflow);
    thread_workflow->start();
    */
//}


workflow_statemachine::workflow_statemachine()
{
    //qDebug() << "workflow_statemachine::workflow_statemachine()";
 //   machine = new QStateMachine();
}

workflow_statemachine::~workflow_statemachine()
{
}


void workflow_statemachine::spawn_workflow(QString session)
{

}
