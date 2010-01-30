#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QThread>
#include <QReadWriteLock>
#include <QList>

class MythUIImage;

class ImageLoader : public QThread
{
    Q_OBJECT

public:
    ImageLoader();

    void loadImage(MythUIImage* image);

    void run();

signals:
    void imageReady(MythUIImage*);

private:
    QList<MythUIImage*> images_;
    QReadWriteLock lock_;
};

#endif // IMAGELOADER_H
