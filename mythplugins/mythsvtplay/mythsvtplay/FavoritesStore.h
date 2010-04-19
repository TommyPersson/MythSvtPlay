#ifndef FAVORITESSTORE_H
#define FAVORITESSTORE_H

#include <QList>
#include <QString>

class FavoritesStore
{

public:
    FavoritesStore();

    void add(const QString& programName);
    void remove(const QString& programName);

    QList<QString> favorites() { return favorites_; }

private:
    void save();
    void load();

    QList<QString> favorites_;

};

#endif // FAVORITESSTORE_H
