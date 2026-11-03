#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <QString>

class FileManager
{
public:
    static bool openFile(const QString &fileName, QString &content);
    static bool saveFile(const QString &fileName, const QString &content);
};

#endif