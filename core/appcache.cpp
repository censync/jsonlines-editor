#include "appcache.h"

#include <QtSql/QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>

AppCache::AppCache()
{

}

AppCache::~AppCache()
{
    if (this->dbCache.isOpen()) {
        this->dbCache.close();
    }
}


bool AppCache::init()
{
    QString appCacheDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);


    if (!QDir(appCacheDir).exists()) {
        QDir().mkpath(appCacheDir);
    }

    this->appCacheDir = appCacheDir;
    this->appCacheFilepath = appCacheDir+"/cache.db";

    this->dbCache = QSqlDatabase::addDatabase("QSQLITE");
    this->dbCache.setDatabaseName(appCacheFilepath);

    if (!this->dbCache.open())
    {
        QMessageBox::critical(nullptr,
                              "Cannot open cache file",
                              QString("Cannot open cache file:\n%1").arg("%1", appCacheFilepath),
                              QMessageBox::Ok);
        return false;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS _config (_key VARCHAR (50) PRIMARY KEY, value TEXT NOT NULL)");

    // query.exec("CREATE TABLE IF NOT EXISTS _cache (_key VARCHAR (32) PRIMARY KEY, value CLOB NOT NULL, filename VARCHAR (150), tm DATETIME DEFAULT current_timestamp)");

    return true;
}

QString AppCache::getCacheFilepath()
{
    return this->appCacheFilepath;
}

QString AppCache::getCacheDir()
{
    return this->appCacheDir;
}

void AppCache::setLastPath(const QString &path)
{
    QSqlQuery query(this->dbCache);
    query.prepare("INSERT INTO _config(_key, value) VALUES('last_path', :path) ON CONFLICT(_key) DO UPDATE SET value = :path WHERE _key = 'last_path'");
    query.bindValue(":path", path);
    if (!query.exec()) {
        QMessageBox::critical(nullptr,
                              "Cannot save value",
                              QString("Cannot save last path:\n%1").arg("%1", query.lastError().databaseText()),
                              QMessageBox::Ok);
    }
}

QString AppCache::getLastPath()
{
    QSqlQuery query(this->dbCache);
    if (query.exec("SELECT value FROM _config WHERE _key = 'last_path' LIMIT 1")) {
        query.first();
        return query.value(0).toString();
    }
    return "";
}

/*void AppCache::updateCacheRow(const QString &key, const QString &value, const QString &filename)
{
    QSqlQuery query(this->dbCache);

    query.prepare("INSERT INTO _cache(_key, value, filename, tm) VALUES (:key, :value, :filename, current_timestamp) ON CONFLICT(_key) DO NOTHING");
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    query.bindValue(":filename", filename);
    if (!query.exec()) {
        qDebug() << query.lastError().databaseText();
    }
}*/

void AppCache::cleanCache()
{
    if (this->dbCache.isOpen()) {
        this->dbCache.close();
    }
    QString appCachePath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir(appCachePath).removeRecursively();
}
