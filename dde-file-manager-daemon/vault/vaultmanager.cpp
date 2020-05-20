/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *               2016 ~ 2018 dragondjf
 *
 * Author:     dragondjf<dingjiangfeng@deepin.com>
 *
 * Maintainer: dragondjf<dingjiangfeng@deepin.com>
 *             zccrs<zhangjide@deepin.com>
 *             Tangtong<tangtong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vault/vaultmanager.h"
#include "dbusservice/dbusadaptor/vault_adaptor.h"
#include <QDBusConnection>
#include <QDBusVariant>
#include <QProcess>
#include <QDebug>
#include "app/policykithelper.h"

QString VaultManager::ObjectPath = "/com/deepin/filemanager/daemon/VaultManager";
QString VaultManager::PolicyKitCreateActionId = "com.deepin.filemanager.daemon.VaultManager.Create";
QString VaultManager::PolicyKitRemoveActionId = "com.deepin.filemanager.daemon.VaultManager.Remove";

VaultManager::VaultManager(QObject *parent)
    : QObject(parent)
    , QDBusContext()
    , m_selfTime(0)
{
    QDBusConnection::systemBus().registerObject(ObjectPath, this);
    m_vaultAdaptor = new VaultAdaptor(this);

    connect(&m_selfTimer, &QTimer::timeout, this, &VaultManager::tick);
    m_selfTimer.setInterval(1000);
    m_selfTimer.start();
}

VaultManager::~VaultManager()
{
    m_selfTimer.stop();
}

void VaultManager::setRefreshTime(quint64 time)
{
    m_lastestTime = time;
}

quint64 VaultManager::getLastestTime() const
{
    return m_lastestTime;
}

quint64 VaultManager::getSelfTime() const
{
    return m_selfTime;
}

void VaultManager::tick()
{
    m_selfTime++;
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
