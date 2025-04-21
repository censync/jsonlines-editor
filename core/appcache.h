#ifndef APPCACHE_H
#define APPCACHE_H

#include <QString>

#include <QtSql/QSqlDatabase>



class AppCache
{
private:
    QString appCacheFilepath;
    QString appCacheDir;
    QSqlDatabase dbCache;

public:
    AppCache();
    ~AppCache();
    bool init();
    QString getCacheFilepath();
    QString getCacheDir();

    void setLastPath(const QString &path);
    QString getLastPath();
    // void updateCacheRow(const QString &key, const QString &value, const QString &filename);
    void cleanCache();
};

#endif // APPCACHE_H
