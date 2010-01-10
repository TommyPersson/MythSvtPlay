#include "ImageLoader.h"

#include <QFile>

#include <mythuiimage.h>

#include <iostream>

ImageLoader::ImageLoader()
{}

void ImageLoader::loadImage(MythUIImage* image)
{
    lock_.lockForWrite();
    images_.push_back(image);
    lock_.unlock();

    start();
}

void ImageLoader::run()
{
    while(!images_.isEmpty())
    {
        int failCount = 0;

        lock_.lockForRead();
        MythUIImage* image = images_.takeFirst();
        lock_.unlock();

        while(!QFile::exists(image->GetFilename()) && failCount < 10)
        {
            // File not downloaded yet
            failCount++;
            sleep(1);
        }

        image->Load();
    }
}
