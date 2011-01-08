#include "Settings.h"
#include <QFile>
#include <QStringList>
#include <mythtv/mythdirs.h>

#include <limits>
#include <iostream>

Settings::Settings()
{
}

int Settings::GetMaxBitrate()
{
    QFile file(GetConfDir() + "/mythsvtplay/config");

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!file.atEnd())
        {
            QByteArray line = file.readLine();
            QString strLine = QString(line);

            if (strLine.contains("MaxBitrate:"))
            {
                return strLine.split(":", QString::SkipEmptyParts).at(1).toInt();
            }
        }

        return std::numeric_limits<int>::max();
    }
    else
    {
        return std::numeric_limits<int>::max();
    }
}
