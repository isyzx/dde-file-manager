/**
 ** This file is part of the filemanager project.
 ** Copyright 2020 luzhen <luzhen@uniontech.com>.
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "vault/vaultmanager.h"
#include "dbusservice/dbusadaptor/vault_adaptor.h"
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusVariant>
#include <QProcess>
#include <QDebug>
#include "app/policykithelper.h"
#include "vaultclock.h"


QString VaultManager::ObjectPath = "/com/deepin/filemanager/daemon/VaultManager";
QString VaultManager::PolicyKitCreateActionId = "com.deepin.filemanager.daemon.VaultManager.Create";
QString VaultManager::PolicyKitRemoveActionId = "com.deepin.filemanager.daemon.VaultManager.Remove";

VaultManager::VaultManager(QObject *parent)
    : QObject(parent)
    , QDBusContext()
    , m_curVaultClock(nullptr)
{
    QDBusConnection::systemBus().registerObject(ObjectPath, this);
    m_vaultAdaptor = new VaultAdaptor(this);

    // create a default vault clock.
    m_curVaultClock = new VaultClock(this);
    m_curUser = getCurrentUser();
    m_mapUserClock.insert(m_curUser, m_curVaultClock);

    // launch timer to check if user changed.
    connect(&m_checkUsrChangeTimer, &QTimer::timeout, this, &VaultManager::checkUserChanged);
    m_checkUsrChangeTimer.setInterval(2000);
    m_checkUsrChangeTimer.start();
}

VaultManager::~VaultManager()
{
}

void VaultManager::setRefreshTime(quint64 time)
{
    m_curVaultClock->setRefreshTime(time);
}

quint64 VaultManager::getLastestTime() const
{
    return m_curVaultClock->getLastestTime();
}

quint64 VaultManager::getSelfTime() const
{
    return m_curVaultClock->getSelfTime();
}

bool VaultManager::checkAuthentication(QString type)
{
    bool ret = false;
    qint64 pid = 0;
    QDBusConnection c = QDBusConnection::connectToBus(QDBusConnection::SystemBus, "org.freedesktop.DBus");
    if(c.isConnected()) {
        pid = c.interface()->servicePid(message().service()).value();
    }

    if (pid){
        if (type.compare("Create") == 0) {
            ret = PolicyKitHelper::instance()->checkAuthorization(PolicyKitCreateActionId, pid);
        }else if (type.compare("Remove") == 0){
            ret = PolicyKitHelper::instance()->checkAuthorization(PolicyKitRemoveActionId, pid);
        }
    }

    if (!ret) {
        qDebug() << "Authentication failed !!";
    }
    return ret;
}

void VaultManager::checkUserChanged()
{
    QString curUser = getCurrentUser();
    if (m_curUser != curUser) {
        m_curUser = curUser;
        bool bContain = m_mapUserClock.contains(m_curUser);
        if (bContain) {
            m_curVaultClock = m_mapUserClock[m_curUser];
        } else {
            m_curVaultClock = new VaultClock(this);
            m_mapUserClock.insert(m_curUser, m_curVaultClock);
        }
    }
}

QString VaultManager::getCurrentUser() const
{
    // Aquire current acount.
    QString user = m_curUser;

    QDBusInterface sessionManagerIface("com.deepin.dde.LockService",
    "/com/deepin/dde/LockService",
    "com.deepin.dde.LockService",
    QDBusConnection::systemBus());

    if (sessionManagerIface.isValid()) {
        QDBusPendingCall call = sessionManagerIface.asyncCall("CurrentUser");
        call.waitForFinished();
        if (!call.isError()) {
            QDBusReply<QString> reply = call.reply();
            user = reply.value();
        }
    }

    return user;
}
