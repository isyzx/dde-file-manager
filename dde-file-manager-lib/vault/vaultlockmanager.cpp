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

#include "vaultlockmanager.h"
#include "dfileservices.h"
#include "dfmsettings.h"
#include "dfmapplication.h"
#include "controllers/vaultcontroller.h"
#include "controllers/vaulterrorcode.h"
#include "../app/define.h"
#include "dialogs/dialogmanager.h"
#include "dialogs/dtaskdialog.h"

#include "../dde-file-manager-daemon/dbusservice/dbusinterface/vault_interface.h"

#define VAULT_AUTOLOCK_KEY      "AutoLock"
#define VAULT_GROUP             "Vault/AutoLock"

VaultLockManager &VaultLockManager::getInstance()
{
    static VaultLockManager instance;
    return instance;
}

VaultLockManager::VaultLockManager(QObject *parent) : QObject(parent)
  , m_autoLockState(VaultLockManager::Never)
  , m_isCacheTimeReloaded(false)
{
    m_vaultInterface = new VaultInterface("com.deepin.filemanager.daemon",
                                          "/com/deepin/filemanager/daemon/VaultManager",
                                          QDBusConnection::systemBus(),
                                          this);

    if (!isValid()) {
        qDebug() << m_vaultInterface->lastError().message();
        return;
    }

    // 自动锁计时处理
    connect(&m_alarmClock, &QTimer::timeout, this, &VaultLockManager::processAutoLock);
    m_alarmClock.setInterval(1000);

    connect(VaultController::getVaultController(), &VaultController::signalLockVault, this,  &VaultLockManager::slotLockVault);
    connect(VaultController::getVaultController(), &VaultController::signalUnlockVault, this,  &VaultLockManager::slotUnlockVault);

    loadConfig();
}

VaultLockManager::~VaultLockManager()
{

}

void VaultLockManager::loadConfig()
{
    VaultLockManager::AutoLockState state = VaultLockManager::Never;
    QVariant var = DFMApplication::genericSetting()->value(VAULT_GROUP, VAULT_AUTOLOCK_KEY);
    if (var.isValid()) {
        state = static_cast<VaultLockManager::AutoLockState>(var.toInt());
    }
    autoLock(state);
}

void VaultLockManager::resetConfig()
{
    autoLock(VaultLockManager::Never);
}

VaultLockManager::AutoLockState VaultLockManager::autoLockState() const
{
    return m_autoLockState;
}

bool VaultLockManager::autoLock(VaultLockManager::AutoLockState lockState)
{
    m_autoLockState = lockState;

    if (m_autoLockState == Never) {
        m_alarmClock.stop();
    } else {
        if (m_isCacheTimeReloaded) {
            refreshAccessTime();
        }

        m_alarmClock.start();
    }
    m_isCacheTimeReloaded = true;

    DFMApplication::genericSetting()->setValue(VAULT_GROUP, VAULT_AUTOLOCK_KEY, lockState);

    return true;
}

void VaultLockManager::refreshAccessTime()
{
    if (isValid()) {
        quint64 curTime = dbusGetSelfTime();
        dbusSetRefreshTime(static_cast<quint64>(curTime));
    }
}

bool VaultLockManager::checkAuthentication(QString type)
{
    bool res = false;
    if (m_vaultInterface->isValid()) {
        QDBusPendingReply<bool> reply = m_vaultInterface->checkAuthentication(type);
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << reply.error().message();
        } else {
            res = reply.value();
        }
    }
    return res;
}

void VaultLockManager::processAutoLock()
{
    VaultController *controller = VaultController::getVaultController();
    if (controller->state() != VaultController::Unlocked
            || m_autoLockState == Never) {

        return;
    }

    quint64 lastAccessTime = dbusGetLastestTime();

    QDateTime local(QDateTime::currentDateTime());
    quint64 curTime = dbusGetSelfTime();

    quint64 interval = curTime - lastAccessTime;
    quint32 threshold = m_autoLockState * 60;

#ifdef AUTOLOCK_TEST
    qDebug() << "vault autolock countdown > " << interval;
#endif

    if (interval > threshold) {

        // 如果正在有保险箱的移动、粘贴、删除操作，强行结束任务
        DTaskDialog *pTaskDlg = dialogManager->taskDialog();
        if(pTaskDlg){
            if(pTaskDlg->bHaveNotCompletedVaultTask()){
                pTaskDlg->stopVaultTask();
            }
        }

        controller->lockVault();
    }
}

void VaultLockManager::slotLockVault(int msg)
{
    if (static_cast<ErrorCode>(msg) == ErrorCode::Success) {
        m_alarmClock.stop();
    } else {
        qDebug() << "vault cannot lock";
    }
}

void VaultLockManager::slotUnlockVault(int msg)
{
    if (static_cast<ErrorCode>(msg) == ErrorCode::Success) {
        autoLock(m_autoLockState);
    }
}

bool VaultLockManager::isValid() const
{
    bool bValid = false;

    if (m_vaultInterface->isValid()) {
        QDBusPendingReply<quint64> reply = m_vaultInterface->getLastestTime();
        reply.waitForFinished();
        bValid = !reply.isError();
    }
    return bValid;
}

void VaultLockManager::dbusSetRefreshTime(quint64 time)
{
    if (m_vaultInterface->isValid()) {
        QDBusPendingReply<> reply = m_vaultInterface->setRefreshTime(time);
        reply.waitForFinished();
        if(reply.isError()) {
            qDebug() << reply.error().message();
        }
    }
}

quint64 VaultLockManager::dbusGetLastestTime() const
{
    quint64 latestTime = 0;
    if (m_vaultInterface->isValid()) {
        QDBusPendingReply<quint64> reply = m_vaultInterface->getLastestTime();
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << reply.error().message();
        } else {
            latestTime = reply.value();
        }
    }
    return latestTime;
}

quint64 VaultLockManager::dbusGetSelfTime() const
{
    quint64 selfTime = 0;
    if (m_vaultInterface->isValid()) {
        QDBusPendingReply<quint64> reply = m_vaultInterface->getSelfTime();
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << reply.error().message();
        } else {
            selfTime = reply.value();
        }
    }
    return selfTime;
}
