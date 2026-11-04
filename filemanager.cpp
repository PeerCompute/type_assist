#include "filemanager.h"
#include <QFile>
#include <QTextStream>

bool FileManager::openFile(const QString &fileName, QString &content)
{
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    content = in.readAll();

    file.close();
    return true;
}

bool FileManager::saveFile(const QString &fileName, const QString &content)
{
    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << content;

    file.close();
    return true;
}