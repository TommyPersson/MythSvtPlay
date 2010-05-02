#include "ImageLoader.h"

#include <QFile>

ImageLoader::ImageLoader()
{
    QObject::connect(&checkTimer_, SIGNAL(timeout()), this, SLOT(onCheckTimerTimeout()));
}

void ImageLoader::loadImage(const QString& imageFilename)
{
    quit();

    imageFilename_ = imageFilename;

    if (QFile::exists(imageFilename_))
    {
        emit imageReady();
    }
    else
    {
        start();
    }
}

void ImageLoader::run()
{
    checkTimer_.start(200);
    exec();
    checkTimer_.stop();
}

void ImageLoader::onCheckTimerTimeout()
{
    if (QFile::exists(imageFilename_))
    {
        emit imageReady();
        quit();
    }
}
