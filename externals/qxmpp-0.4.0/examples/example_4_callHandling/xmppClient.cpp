/*
 * Copyright (C) 2008-2011 The QXmpp developers
 *
 * Authors:
 *	Ian Reinhart Geiser
 *  Jeremy Lainé
 *
 * Source:
 *	http://code.google.com/p/qxmpp
 *
 * This file is a part of QXmpp library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#include <QAudioInput>
#include <QAudioOutput>
#include <QDebug>

#include "QXmppCallManager.h"
#include "QXmppJingleIq.h"
#include "QXmppRtpChannel.h"
#include "QXmppUtils.h"

#include "xmppClient.h"

xmppClient::xmppClient(QObject *parent)
    : QXmppClient(parent)
{
    // add QXmppCallManager extension
    callManager = new QXmppCallManager;
    addExtension(callManager);

    bool check = connect(this, SIGNAL(presenceReceived(QXmppPresence)),
                         this, SLOT(slotPresenceReceived(QXmppPresence)));
    Q_ASSERT(check);

    check = connect(callManager, SIGNAL(callReceived(QXmppCall*)),
                    this, SLOT(slotCallReceived(QXmppCall*)));
    Q_ASSERT(check);
}

/// The audio mode of a call changed.

void xmppClient::slotAudioModeChanged(QIODevice::OpenMode mode)
{
    QXmppCall *call = qobject_cast<QXmppCall*>(sender());
    Q_ASSERT(call);
    QXmppRtpAudioChannel *channel = call->audioChannel();

    // prepare audio format
    QAudioFormat format;
    format.setFrequency(channel->payloadType().clockrate());
    format.setChannels(channel->payloadType().channels());
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    // the size in bytes of the audio buffers to/from sound devices
    // 160 ms seems to be the minimum to work consistently on Linux/Mac/Windows
    const int bufferSize = (format.frequency() * format.channels() * (format.sampleSize() / 8) * 160) / 1000;

    if (mode & QIODevice::ReadOnly) {
        // initialise audio output
        QAudioOutput *audioOutput = new QAudioOutput(format, this);
        audioOutput->setBufferSize(bufferSize);
        audioOutput->start(channel);
    }

    if (mode & QIODevice::WriteOnly) {
        // initialise audio input
        QAudioInput *audioInput = new QAudioInput(format, this);
        audioInput->setBufferSize(bufferSize);
        audioInput->start(channel);
    }
}


/// A call was received.

void xmppClient::slotCallReceived(QXmppCall *call)
{
    qDebug() << "Got call from:" << call->jid();

    bool check;
    check = connect(call, SIGNAL(stateChanged(QXmppCall::State)),
                    this, SLOT(slotCallStateChanged(QXmppCall::State)));
    Q_ASSERT(check);

    check = connect(call, SIGNAL(audioModeChanged(QIODevice::OpenMode)),
                    this, SLOT(slotAudioModeChanged(QIODevice::OpenMode)));
    Q_ASSERT(check);

    // accept call    
    call->accept();
}

/// A call changed state.

void xmppClient::slotCallStateChanged(QXmppCall::State state)
{
    if (state == QXmppCall::ActiveState)
        qDebug("Call active");
    else if (state == QXmppCall::DisconnectingState)
        qDebug("Call disconnecting");
    else if (state == QXmppCall::FinishedState)
        qDebug("Call finished");
}

/// A presence was received.

void xmppClient::slotPresenceReceived(const QXmppPresence &presence)
{
    const QLatin1String recipient("qxmpp.test2@gmail.com");

    // if we are the recipient, or if the presence is not from the recipient,
    // do nothing
    if (jidToBareJid(configuration().jid()) == recipient ||
        jidToBareJid(presence.from()) != recipient ||
        presence.type() != QXmppPresence::Available)
        return;

    // start the call and connect to the its signals
    QXmppCall *call = callManager->call(presence.from());

    bool check;
    check = connect(call, SIGNAL(stateChanged(QXmppCall:State)),
                    this, SLOT(slotCallStateChanged(QXmppCall::State)));
    Q_ASSERT(check);

    check = connect(call, SIGNAL(audioModeChanged(QIODevice::OpenMode)),
                    this, SLOT(slotAudioModeChanged(QIODevice::OpenMode)));
    Q_ASSERT(check);
}

