#include "bookmarkmanager.h"
#include "fileinfo.h"
#include "../app/global.h"

#include <QJsonObject>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QByteArray>
#include <QDateTime>
#include <QUrl>

#include <stdlib.h>

BookMarkManager::BookMarkManager(QObject *parent)
    : AbstractFileController(parent)
    , BaseManager()
{
    load();
    fileService->setFileUrlHandler(BOOKMARK_SCHEME, "", this);
}

BookMarkManager::~BookMarkManager()
{

}

void BookMarkManager::load()
{
    //TODO: check permission and existence of the path
    QString user = getenv("USER");
    QString configPath = "/home/" + user + "/.cache/dde-file-manager/bookmark.json";
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Couldn't open bookmark file!";
        return;
    }
    QByteArray data = file.readAll();
    QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
    loadJson(jsonDoc.object());
}

void BookMarkManager::save()
{
    //TODO: check permission and existence of the path
    QString user = getenv("USER");
    QDir dir("/home/" + user + "/.cache");
    if(dir.exists())
        dir.mkdir("dde-file-manager");

    QString configPath = "/home/" + user + "/.cache/dde-file-manager/bookmark.json";
    QFile file(configPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        qDebug() << "Couldn't write bookmark file!";
        return;
    }
    QJsonObject object;
    writeJson(object);
    QJsonDocument jsonDoc(object);
    file.write(jsonDoc.toJson());
}

QList<BookMark *> BookMarkManager::getBookmarks()
{
    return m_bookmarks;
}

void BookMarkManager::loadJson(const QJsonObject &json)
{
    QJsonArray jsonArray = json["Bookmark"].toArray();
    for(int i = 0; i < jsonArray.size(); i++)
    {
        QJsonObject object = jsonArray[i].toObject();
        QString time = object["t"].toString();
        QString name = object["n"].toString();
        QString url = object["u"].toString();
        m_bookmarks.append(new BookMark(QDateTime::fromString(time), name, DUrl(url)));
    }
}

void BookMarkManager::writeJson(QJsonObject &json)
{
    QJsonArray localArray;
    for(int i = 0; i < m_bookmarks.size(); i++)
    {
        QJsonObject object;
        object["t"] = m_bookmarks.at(i)->getDateTime().toString();
        object["n"] = m_bookmarks.at(i)->getName();
        object["u"] = m_bookmarks.at(i)->getUrl().toString();
        localArray.append(object);
    }
    json["Bookmark"] = localArray;
}

BookMark * BookMarkManager::writeIntoBookmark(int index, const QString &name, const DUrl &url)
{
    BookMark * bookmark = new BookMark(QDateTime::currentDateTime(), name, url);
    m_bookmarks.insert(index, bookmark);
    save();
    return bookmark;
}

void BookMarkManager::removeBookmark(const QString &name, const DUrl &url)
{
    for(int i = 0; i < m_bookmarks.size(); i++)
    {
        BookMark * bookmark = m_bookmarks.at(i);
        if(bookmark->getName() == name && bookmark->getUrl() == url)
        {
            m_bookmarks.removeOne(bookmark);
            break;
        }
    }
    save();
}

void BookMarkManager::renameBookmark(const QString &oldname, const QString &newname, const DUrl &url)
{
    for(int i = 0; i < m_bookmarks.size(); i++)
    {
        BookMark * bookmark = m_bookmarks.at(i);
        if(bookmark->getName() == oldname && bookmark->getUrl() == url)
        {
            bookmark->setName(newname);
            break;
        }
    }
    save();
}

void BookMarkManager::moveBookmark(int from, int to)
{
    m_bookmarks.move(from, to);
    save();
}

const QList<AbstractFileInfoPointer> BookMarkManager::getChildren(const DUrl &fileUrl, QDir::Filters filter, bool &accepted) const
{
    Q_UNUSED(fileUrl)
    Q_UNUSED(filter)

    accepted = true;

    const QString &frav = fileUrl.fragment();

    if(!frav.isEmpty())
    {
        DUrl localUrl = DUrl::fromLocalFile(frav);

        QList<AbstractFileInfoPointer> list = fileService->getChildren(localUrl, filter);

        return list;
    }

    QList<AbstractFileInfoPointer> infolist;

    for (int i = 0; i < m_bookmarks.size(); i++)
    {
        BookMark * bm = m_bookmarks.at(i);

        infolist.append(AbstractFileInfoPointer(new BookMark(*bm)));
    }

    return infolist;
}

const AbstractFileInfoPointer BookMarkManager::createFileInfo(const DUrl &fileUrl, bool &accepted) const
{
    accepted = true;

    return AbstractFileInfoPointer(new BookMark(fileUrl));
}
