#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QThread>
#include <QTimer>

class ImageLoader : public QThread
{
    Q_OBJECT

public:
    ImageLoader();

    void loadImage(const QString& imageFilename);

    void run();

signals:
    void imageReady();

private slots:
    void onCheckTimerTimeout();

private:
    QString imageFilename_;
    QTimer checkTimer_;
};

#endif // IMAGELOADER_H
