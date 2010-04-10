#ifndef PROGRAMLISTCACHE_H
#define PROGRAMLISTCACHE_H

#include <QObject>

#include <QList>

#include <mythtv/mythdirs.h>

class Program;
class ProgramListBuilder;

class ProgramListCache : public QObject
{
    Q_OBJECT

public:
    ProgramListCache();
    ~ProgramListCache();

    void refresh();
    QList<Program*> programs();
    bool isEmpty();

    static QString cacheFilePath() { return GetConfDir() + "/mythsvtplay/programs.cache"; }

signals:
    void numberOfProgramsFound(int count);
    void numberOfProgramsComplete(int count);
    void programComplete(const QString& title);

    void cacheFilled();

public slots:
    void onProgramListBuilt(QList<Program*>);

private:
    void store();
    void load();

    ProgramListBuilder* programListBuilder_;

    QList<Program*> programCache_;

};

#endif // PROGRAMLISTCACHE_H
