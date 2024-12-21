/***************************************************************************
 *   Copyright (C) 2008 by I2P-Messenger   				   *
 *   Messenger-Dev@I2P-Messenger   					   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/



#include "SessionController.h"

const QString SAM_HANDSHAKE_V3="HELLO VERSION MIN=3.1 MAX=3.1\n";

CSessionController::CSessionController(
        QString SamHost, QString SamPort, QString BridgeName,
        QString SamPrivKey,
        QString ConfigPath,QString SessionOptions)
    :mSamHost(SamHost),mSamPort(SamPort),mBridgeName(BridgeName),mConfigPath(ConfigPath),mSessionOptions(SessionOptions)
{

    mIncomingPackets=new QByteArray();
    mDoneDisconnect=false;

    mAnalyser= new CI2PSamMessageAnalyser("CStreamController");
    mHandShakeWasSuccesfullDone=false;
    mSessionWasSuccesfullCreated=false;
    mSamPrivKey=SamPrivKey;

    connect(&mTcpSocket,SIGNAL(connected() ),this,
            SLOT(slotConnected() ),Qt::DirectConnection );

    connect(&mTcpSocket,SIGNAL(disconnected()),this,
            SLOT(slotDisconnected()),Qt::DirectConnection );

    connect(&mTcpSocket,SIGNAL(readyRead() ),this,
            SLOT(slotReadFromSocket()),Qt::DirectConnection);

    emit signDebugMessages("<-- StreamController (I2P) started -->");
}

CSessionController::~ CSessionController()
{
    doDisconnect();
    mTcpSocket.deleteLater();
    emit signDebugMessages("<-- StreamController (I2P) stopped -->");
}

void CSessionController::slotConnected()
{
    emit signDebugMessages("<-- StreamController (I2P) Connected -->\n");
    emit signDebugMessages(SAM_HANDSHAKE_V3);
    if(mTcpSocket.state()==QAbstractSocket::ConnectedState){
        mTcpSocket.write(SAM_HANDSHAKE_V3.toUtf8());
        mTcpSocket.flush();
    }
}

void CSessionController::slotDisconnected()
{
    if(mDoneDisconnect==false)
    {
        mTcpSocket.close();
        emit signDebugMessages("<-- StreamController (I2P) can't connect I2P(sam) or I2P crashed -->\nSamHost:\t"+mSamHost+"\nSamPort:\t" +mSamPort+"\n");
        emit signSessionStreamStatusOK(false);

        QMessageBox msgBox(NULL);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setInformativeText("<-- StreamController (I2P) can't connect I2P(sam) or I2P crashed -->\nSamHost:\t"+mSamHost+"\nSamPort:\t" +mSamPort+"\n");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setWindowModality(Qt::NonModal);
        msgBox.exec();

        //emit SamConnectionClosed();
    }
}

void CSessionController::slotReadFromSocket()
{
    using namespace SAM_Message_Types;

    QByteArray newData =mTcpSocket.readAll();
    QByteArray CurrentPacket;
    mIncomingPackets->append(newData);

    while(mIncomingPackets->contains("\n")==true) {
        CurrentPacket=mIncomingPackets->left(mIncomingPackets->indexOf("\n",0)+1);

        QString t(CurrentPacket.data());

        SAM_MESSAGE sam=mAnalyser->Analyse(t);
        switch(sam.type) {	//emit the signals
        case HELLO_REPLAY:{
            emit signDebugMessages(t);
            if(sam.result==OK){
                this->mHandShakeWasSuccesfullDone=true;
                if(mSamPrivKey=="") {
                    QSettings settings(mConfigPath+"/application.ini",QSettings::IniFormat);
                    QString Signature="SIGNATURE_TYPE=%s";
                    settings.beginGroup("Network");
                    Signature.replace("%s",settings.value("Signature_Type","DSA_SHA1").toString());
                    this->doDestGenerate(Signature);
                    settings.endGroup();
                    settings.sync();

                }
                else{
                    doSessionCreate();
                }
            }
            else{
                emit signSessionStreamStatusOK(false);
            }

            break;
        }
        case SESSION_STATUS:{
            emit signDebugMessages(t);
            if(sam.result==OK){
                mSessionWasSuccesfullCreated=true;
                emit signSessionStreamStatusOK(true);
            }
            else{
                if(sam.result==DUPLICATED_DEST){
                    QMessageBox msgBox(NULL);
                    msgBox.setIcon(QMessageBox::Critical);
                    msgBox.setText(tr("I2P-Messenger"));
                    msgBox.setInformativeText(tr("Session: DUPLICATED_DEST\n\nOnly one Messenger per Destination,\nor SAMv3 crashed(Tunnel stay if Messenger were closed)\n "));
                    msgBox.setStandardButtons(QMessageBox::Ok);
                    msgBox.setDefaultButton(QMessageBox::Ok);
                    msgBox.setWindowModality(Qt::NonModal);
                    msgBox.exec();

                    qCritical()<<"File\t"<<__FILE__<<Qt::endl
                              <<"Line:\t"<<__LINE__<<Qt::endl
                             <<"Function:\t"<<"CStreamController::slotReadFromSocket()"<<Qt::endl
                            <<"Message:\t"<<"Session: DUPLICATED_DEST"<<Qt::endl
                           <<"Only one Messenger per Destination,\nor SAMv3 crashed(Tunnel stay if Messenger were closed"<<Qt::endl;
                }


                emit signSessionStreamStatusOK(false);
            }

            break;
        }
        case STREAM_STATUS:{
            emit signDebugMessages(t);
            //emit StreamStatusRecived(sam.result,sam.ID,sam.Message);
            break;
        }
        case NAMING_REPLY:{
            emit signDebugMessages(t);
            emit signNamingReplyRecived(sam.result,sam.Name,sam.Value,sam.Message);
            break;
        }
        case DEST_REPLY:{
            emit signDebugMessages(t);
            mSamPrivKey=sam.PRIV;
            emit signNewSamPrivKeyGenerated(mSamPrivKey);
            if(mSessionWasSuccesfullCreated==false){
                doSessionCreate();
            }
            break;
        }
        case ERROR_IN_ANALYSE:{
            emit signDebugMessages("CStreamController: <ERROR_IN_ANALYSE>\n"+t);
            break;
        }
        default:
        {
            emit signDebugMessages("CStreamController: <Unknown Packet>\n"+t);
            break;
        }
        }
        mIncomingPackets->remove(0,mIncomingPackets->indexOf("\n",0)+1);
    }//while

}

void CSessionController::doConnect()
{
    mDoneDisconnect=false;

    if(mTcpSocket.state()==QAbstractSocket::UnconnectedState)
    {
        mTcpSocket.connectToHost(mSamHost,mSamPort.toInt( ));
    }
    if(!mTcpSocket.waitForConnected(1000))
        slotDisconnected();
}

void CSessionController::doDisconnect()
{
    mDoneDisconnect=true;

    if(mTcpSocket.state()!=0){
        mTcpSocket.disconnectFromHost();
        emit signDebugMessages("<-- StreamController (I2P) Socket Disconnected -->\n");
    }
    else if(mTcpSocket.state()==QAbstractSocket::UnconnectedState){
        emit signDebugMessages("<-- StreamController (I2P) Socket allready Disconnected -->\n");
    }

}

void CSessionController::doNamingLookUP(QString Name)
{
    ConnectionReadyCheck();

    QByteArray Message="NAMING LOOKUP NAME=";
    Message+=Name.toUtf8()+"\n";
    emit signDebugMessages(Message);
    mTcpSocket.write(Message);
    mTcpSocket.flush();

}

void CSessionController::doSessionCreate()
{
    ConnectionReadyCheck();

    QByteArray Message="SESSION CREATE STYLE=STREAM ID=";
    Message+=mBridgeName.toUtf8()+" DESTINATION="+mSamPrivKey.toUtf8();

    if(mSessionOptions.isEmpty()==false){
        Message+=" "+mSessionOptions.toUtf8();
    }

    Message+="\n";
    emit signDebugMessages(Message);

    mTcpSocket.write(Message);
    mTcpSocket.flush();
}

void CSessionController::doDestGenerate(const QString Options)
{
    ConnectionReadyCheck();
    QByteArray Message="DEST GENERATE ";
    if(Options.isEmpty()==false){
        Message+=Options.toUtf8();
    }
    Message+="\n";

    emit signDebugMessages(Message);

    mTcpSocket.write(Message);
    mTcpSocket.flush();
}

