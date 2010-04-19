#include "FavoritesStore.h"

#include <QFile>
#include <QTextStream>

#include <mythtv/mythdirs.h>

#include <iostream>

FavoritesStore::FavoritesStore()
{
    load();
}

void FavoritesStore::add(const QString& programName)
{
    favorites_.append(programName);
    qSort(favorites_);
    save();
}

void FavoritesStore::remove(const QString& programName)
{
    favorites_.removeOne(programName);
    save();
}

void FavoritesStore::save()
{
    QFile saveFile(GetConfDir() + "/mythsvtplay/favorites.txt");
    saveFile.open(QIODevice::WriteOnly);

    for (int i = 0; i < favorites_.count(); ++i)
    {
        saveFile.write(QString(favorites_.at(i) + "\n").toUtf8());
    }

    saveFile.close();
}

void FavoritesStore::load()
{
    favorites_.clear();

    QFile saveFile(GetConfDir() + "/mythsvtplay/favorites.txt");
    saveFile.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream favStream(&saveFile);

    while (!favStream.atEnd())
    {
        QString programName = favStream.readLine();
        favorites_.append(programName);
    }

    saveFile.close();
}
