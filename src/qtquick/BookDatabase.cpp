/*
 * Copyright (C) 2017 Dan Leinir Turthra Jensen <admin@leinir.dk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "BookDatabase.h"

#include "CategoryEntriesModel.h"

#include <QDebug>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include <QDir>

class BookDatabase::Private {
public:
    Private() {
        db = QSqlDatabase::addDatabase("QSQLITE");

        QDir location{QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)};
        if(!location.exists())
            location.mkpath(".");

        dbfile = location.absoluteFilePath("library.sqlite");
        db.setDatabaseName(dbfile);
    }

    QSqlDatabase db;
    QString dbfile;
    QStringList fieldNames;

    bool prepareDb() {
        if (!db.open()) {
            qDebug() << "Failed to open the book database file" << dbfile << db.lastError();
            return false;
        }

        QStringList tables = db.tables();
        if (tables.contains("books", Qt::CaseInsensitive)) {
            if (fieldNames.isEmpty()) {
                QSqlQuery qu("SELECT * FROM books");
                for (int i=0; i< qu.record().count(); i++) {
                    fieldNames.append(qu.record().fieldName(i));
                }
                qDebug() << Q_FUNC_INFO << ": opening database with following fieldNames:" << fieldNames;
            }
            return true;
        }

        QSqlQuery q;
        QStringList entryNames;
        entryNames << "fileName varchar primary key" << "fileTitle varchar" << "title varchar" << "genres varchar"
                   << "keywords varchar" << "characters varchar" << "description varchar" << "series varchar"
                   << "seriesNumbers varchar" << "seriesVolumes varchar" << "author varchar" << "publisher varchar"
                   << "created datetime" << "lastOpenedTime datetime" << "totalPages integer" << "currentPage integer"
                   << "thumbnail varchar" << "comment varchar" << "tags varchar" << "rating varchar";
        if (!q.exec(QString("create table books("+entryNames.join(", ")+")"))) {
            qDebug() << "Database could not create the table books";
            return false;
        }
        for (int i=0; i< entryNames.size(); i++) {
            QString fieldName = entryNames.at(i).split(" ").first();
            fieldNames.append(fieldName);
        }
        qDebug() << Q_FUNC_INFO << ": making database with following fieldNames:" << fieldNames;

        return true;
    }

    void closeDb() {
        db.close();
    }
};

BookDatabase::BookDatabase(QObject* parent)
    : QObject(parent)
    , d(new Private)
{
}

BookDatabase::~BookDatabase()
{
    delete d;
}

QList<BookEntry*> BookDatabase::loadEntries()
{
    if(!d->prepareDb()) {
        return QList<BookEntry*>();
    }

    QList<BookEntry*> entries;
    QStringList entryNames = d->fieldNames;
    QSqlQuery allEntries("SELECT " + d->fieldNames.join(", ") + " FROM books");
    while(allEntries.next())
    {
        BookEntry* entry = new BookEntry();
        entry->filename       = allEntries.value(d->fieldNames.indexOf("fileName")).toString();
        entry->filetitle      = allEntries.value(d->fieldNames.indexOf("fileTitle")).toString();
        entry->title          = allEntries.value(d->fieldNames.indexOf("title")).toString();
        entry->series         = allEntries.value(d->fieldNames.indexOf("series")).toString().split(",", QString::SkipEmptyParts);
        entry->author         = allEntries.value(d->fieldNames.indexOf("author")).toString().split(",", QString::SkipEmptyParts);
        entry->publisher      = allEntries.value(d->fieldNames.indexOf("publisher")).toString();
        entry->created        = allEntries.value(d->fieldNames.indexOf("created")).toDateTime();
        entry->lastOpenedTime = allEntries.value(d->fieldNames.indexOf("lastOpenedTime")).toDateTime();
        entry->totalPages     = allEntries.value(d->fieldNames.indexOf("totalPages")).toInt();
        entry->currentPage    = allEntries.value(d->fieldNames.indexOf("currentPage")).toInt();
        entry->thumbnail      = allEntries.value(d->fieldNames.indexOf("thumbnail")).toString();
        entry->description    = allEntries.value(d->fieldNames.indexOf("description")).toString().split("\n", QString::SkipEmptyParts);
        entry->comment        = allEntries.value(d->fieldNames.indexOf("comment")).toString();
        entry->tags           = allEntries.value(d->fieldNames.indexOf("tags")).toString().split(",", QString::SkipEmptyParts);
        entry->rating         = allEntries.value(d->fieldNames.indexOf("rating")).toInt();
        entry->seriesNumbers  = allEntries.value(d->fieldNames.indexOf("seriesNumbers")).toString().split(",", QString::SkipEmptyParts);
        entry->seriesVolumes  = allEntries.value(d->fieldNames.indexOf("seriesVolumes")).toString().split(",", QString::SkipEmptyParts);
        entry->genres         = allEntries.value(d->fieldNames.indexOf("genres")).toString().split(",", QString::SkipEmptyParts);
        entry->keywords       = allEntries.value(d->fieldNames.indexOf("keywords")).toString().split(",", QString::SkipEmptyParts);
        entry->characters     = allEntries.value(d->fieldNames.indexOf("characters")).toString().split(",", QString::SkipEmptyParts);
        entries.append(entry);
    }

    d->closeDb();
    return entries;
}

void BookDatabase::addEntry(BookEntry* entry)
{
    if(!d->prepareDb()) {
        return;
    }
    qDebug() << "Adding newly discovered book to the database" << entry->filename;

    QStringList valueNames;
    for (int i=0; i< d->fieldNames.size(); i++) {
        valueNames.append(QString(":").append(d->fieldNames.at(i)));
    }
    QSqlQuery newEntry;
    newEntry.prepare("INSERT INTO books (" + d->fieldNames.join(", ") + ") "
                     "VALUES (" + valueNames.join(", ") + ")");
    newEntry.bindValue(":fileName", entry->filename);
    newEntry.bindValue(":fileTitle", entry->filetitle);
    newEntry.bindValue(":title", entry->title);
    newEntry.bindValue(":series", entry->series.join(","));
    newEntry.bindValue(":author", entry->author.join(","));
    newEntry.bindValue(":publisher", entry->publisher);
    newEntry.bindValue(":publisher", entry->publisher);
    newEntry.bindValue(":created", entry->created);
    newEntry.bindValue(":lastOpenedTime", entry->lastOpenedTime);
    newEntry.bindValue(":totalPages", entry->totalPages);
    newEntry.bindValue(":currentPage", entry->currentPage);
    newEntry.bindValue(":thumbnail", entry->thumbnail);
    newEntry.bindValue(":description", entry->description.join("\n"));
    newEntry.bindValue(":comment", entry->comment);
    newEntry.bindValue(":tags", entry->tags.join(","));
    newEntry.bindValue(":rating", entry->rating);
    newEntry.bindValue(":seriesNumbers", entry->seriesNumbers.join(","));
    newEntry.bindValue(":seriesVolumes", entry->seriesVolumes.join(","));
    newEntry.bindValue(":genres", entry->genres.join(","));
    newEntry.bindValue(":keywords", entry->keywords.join(","));
    newEntry.bindValue(":characters", entry->characters.join(","));
    newEntry.exec();

    d->closeDb();
}

void BookDatabase::removeEntry(BookEntry* entry)
{
    if(!d->prepareDb()) {
        return;
    }
    qDebug() << "Removing book from the database" << entry->filename;

    QSqlQuery removeEntry;
    removeEntry.prepare("DELETE FROM books WHERE filename='"+entry->filename+"';");
    removeEntry.exec();

    d->closeDb();
}
