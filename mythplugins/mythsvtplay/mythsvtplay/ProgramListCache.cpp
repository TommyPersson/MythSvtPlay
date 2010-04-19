#include "ProgramListCache.h"

#include <QDir>
#include <QFile>
#include <QDataStream>

#include "Program.h"
#include "ProgramListBuilder.h"

ProgramListCache::ProgramListCache()
    : programListBuilder_(new ProgramListBuilder())
{
    QObject::connect(programListBuilder_, SIGNAL(programListBuilt(QList<Program*>)),
                     this, SLOT(onProgramListBuilt(QList<Program*>)));

    QObject::connect(programListBuilder_, SIGNAL(numberOfProgramsFound(int)),
                     this, SIGNAL(numberOfProgramsFound(int)));
    QObject::connect(programListBuilder_, SIGNAL(numberOfProgramsComplete(int)),
                     this, SIGNAL(numberOfProgramsComplete(int)));
    QObject::connect(programListBuilder_, SIGNAL(programComplete(QString)),
                     this, SIGNAL(programComplete(QString)));

    load();
}

ProgramListCache::~ProgramListCache()
{
    delete programListBuilder_;

    while(!programCache_.isEmpty())
    {
        Program* p = programCache_.takeFirst();
        delete p;
    }
}

void ProgramListCache::refresh()
{
    programListBuilder_->buildProgramList();
}

QList<Program*> ProgramListCache::programs()
{
    return programCache_;
}

bool ProgramListCache::isEmpty()
{
    return programCache_.isEmpty();
}

void ProgramListCache::store()
{
    QFile cacheFile(cacheFilePath());
    cacheFile.open(QIODevice::WriteOnly);

    QDataStream stream(&cacheFile);

    stream << programCache_.count();

    for (int i = 0; i < programCache_.count(); ++i)
    {
        stream << *(programCache_[i]);
    }
}

void ProgramListCache::load()
{
    QFile cacheFile(cacheFilePath());
    if (cacheFile.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&cacheFile);

        int count;
        stream >> count;

        for (int i = 0; i < count; ++i)
        {
            Program* program = new Program();
            stream >> *program;
            programCache_.push_back(program);
        }
    }
}

void ProgramListCache::onProgramListBuilt(const QList<Program*>& programs)
{
    programCache_ = programs;
    store();
    emit cacheFilled();
}
