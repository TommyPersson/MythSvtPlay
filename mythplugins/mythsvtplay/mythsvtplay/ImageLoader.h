#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QThread>
#include <QReadWriteLock>
#include <QList>

class MythUIImage;

class ImageLoader : public QThread
{
public:
    ImageLoader();

    void loadImage(MythUIImage* image);

    void run();

private:
    QList<MythUIImage*> images_;
    QReadWriteLock lock_;
};

#endif // IMAGELOADER_H
