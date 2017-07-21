/*
 * Copyright (C) 2015 Dan Leinir Turthra Jensen <admin@leinir.dk>
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

#include "CategoryEntriesModel.h"
#include "PropertyContainer.h"
#include <KFileMetaData/UserMetaData>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

class CategoryEntriesModel::Private {
public:
    Private(CategoryEntriesModel* qq)
        : q(qq)
    {};
    ~Private()
    {
        // No deleting the entries - this is done by the master BookListModel already, so do that at your own risk
    }
    CategoryEntriesModel* q;
    QString name;
    QList<BookEntry*> entries;
    QList<CategoryEntriesModel*> categoryModels;

    QObject* wrapBookEntry(const BookEntry* entry) {
        PropertyContainer* obj = new PropertyContainer("book", q);
        obj->setProperty("author", entry->author);
        obj->setProperty("currentPage", QString::number(entry->currentPage));
        obj->setProperty("filename", entry->filename);
        obj->setProperty("filetitle", entry->filetitle);
        obj->setProperty("created", entry->created);
        obj->setProperty("lastOpenedTime", entry->lastOpenedTime);
        obj->setProperty("publisher", entry->publisher);
        obj->setProperty("series", entry->series);
        obj->setProperty("title", entry->title);
        obj->setProperty("totalPages", entry->totalPages);
        obj->setProperty("thumbnail", entry->thumbnail);
        return obj;
    }
};

CategoryEntriesModel::CategoryEntriesModel(QObject* parent)
    : QAbstractListModel(parent)
    , d(new Private(this))
{
    connect(this, SIGNAL(entryDataUpdated(BookEntry*)), this, SLOT(entryDataChanged(BookEntry*)));
    connect(this, SIGNAL(entryRemoved(BookEntry*)), this, SLOT(entryRemove(BookEntry*)));
}

CategoryEntriesModel::~CategoryEntriesModel()
{
    delete d;
}

QHash<int, QByteArray> CategoryEntriesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FilenameRole] = "filename";
    roles[FiletitleRole] = "filetitle";
    roles[TitleRole] = "title";
    roles[SeriesRole] = "series";
    roles[AuthorRole] = "author";
    roles[PublisherRole] = "publisher";
    roles[CreatedRole] = "created";
    roles[LastOpenedTimeRole] = "lastOpenedTime";
    roles[TotalPagesRole] = "totalPages";
    roles[CurrentPageRole] = "currentPage";
    roles[CategoryEntriesModelRole] = "categoryEntriesModel";
    roles[CategoryEntryCountRole] = "categoryEntriesCount";
    roles[ThumbnailRole] = "thumbnail";
    return roles;
}

QVariant CategoryEntriesModel::data(const QModelIndex& index, int role) const
{
    QVariant result;
    if(index.isValid() && index.row() > -1)
    {
        if(index.row() < d->categoryModels.count())
        {
            CategoryEntriesModel* model = d->categoryModels[index.row()];
            switch(role)
            {
                case Qt::DisplayRole:
                case TitleRole:
                    result.setValue(model->name());
                    break;
                case CategoryEntryCountRole:
                    result.setValue(model->bookCount());
                    break;
                case CategoryEntriesModelRole:
                    result.setValue(model);
                    break;
                default:
                    result.setValue(QString("Unknown role"));
                    break;
            }
        }
        else
        {
            const BookEntry* entry = d->entries[index.row() - d->categoryModels.count()];
            switch(role)
            {
                case Qt::DisplayRole:
                case FilenameRole:
                    result.setValue(entry->filename);
                    break;
                case FiletitleRole:
                    result.setValue(entry->filetitle);
                    break;
                case TitleRole:
                    result.setValue(entry->title);
                    break;
                case SeriesRole:
                    result.setValue(entry->series);
                    break;
                case AuthorRole:
                    result.setValue(entry->author);
                    break;
                case PublisherRole:
                    result.setValue(entry->publisher);
                    break;
                case CreatedRole:
                    result.setValue(entry->created);
                    break;
                case LastOpenedTimeRole:
                    result.setValue(entry->lastOpenedTime);
                    break;
                case TotalPagesRole:
                    result.setValue(entry->totalPages);
                    break;
                case CurrentPageRole:
                    result.setValue(entry->currentPage);
                    break;
                case CategoryEntriesModelRole:
                    // Nothing, if we're not equipped with one such...
                    break;
                case CategoryEntryCountRole:
                    result.setValue<int>(0);
                    break;
                case ThumbnailRole:
                    result.setValue(entry->thumbnail);
                    break;
                default:
                    result.setValue(QString("Unknown role"));
                    break;
            }
        }
    }
    return result;
}

int CategoryEntriesModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;
    return d->categoryModels.count() + d->entries.count();
}

void CategoryEntriesModel::append(BookEntry* entry, Roles compareRole)
{
    int insertionIndex = 0;
    for(; insertionIndex < d->entries.count(); ++insertionIndex)
    {
        if(compareRole == CreatedRole)
        {
            if(entry->created <= d->entries.at(insertionIndex)->created)
            { continue; }
            break;
        }
        else
        {
            if(QString::localeAwareCompare(d->entries.at(insertionIndex)->title, entry->title) > 0)
            { break; }
        }
    }
    beginInsertRows(QModelIndex(), insertionIndex, insertionIndex);
    d->entries.insert(insertionIndex, entry);
    endInsertRows();
}

QString CategoryEntriesModel::name() const
{
    return d->name;
}

void CategoryEntriesModel::setName(const QString& newName)
{
    d->name = newName;
}

QObject * CategoryEntriesModel::leafModelForEntry(BookEntry* entry)
{
    QObject* model(0);
    if(d->categoryModels.count() == 0)
    {
        if(d->entries.contains(entry)) {
            model = this;
        }
    }
    else
    {
        Q_FOREACH(CategoryEntriesModel* testModel, d->categoryModels)
        {
            model = testModel->leafModelForEntry(entry);
            if(model) {
                break;
            }
        }
    }
    return model;
}

void CategoryEntriesModel::addCategoryEntry(const QString& categoryName, BookEntry* entry)
{
    if(categoryName.length() > 0)
    {
        QStringList splitName = categoryName.split("/");
//         qDebug() << "Parsing" << categoryName;
        QString nextCategory = splitName.takeFirst();
        CategoryEntriesModel* categoryModel = 0;
        Q_FOREACH(CategoryEntriesModel* existingModel, d->categoryModels)
        {
            if(existingModel->name() == nextCategory)
            {
                categoryModel = existingModel;
                break;
            }
        }
        if(!categoryModel)
        {
            categoryModel = new CategoryEntriesModel(this);
            connect(this, SIGNAL(entryDataUpdated(BookEntry*)), categoryModel, SIGNAL(entryDataUpdated(BookEntry*)));
            connect(this, SIGNAL(entryRemoved(BookEntry*)), categoryModel, SIGNAL(entryRemoved(BookEntry*)));
            categoryModel->setName(nextCategory);

            int insertionIndex = 0;
            for(; insertionIndex < d->categoryModels.count(); ++insertionIndex)
            {
                if(QString::localeAwareCompare(d->categoryModels.at(insertionIndex)->name(), categoryModel->name()) > 0)
                {
                    break;
                }
            }
            beginInsertRows(QModelIndex(), insertionIndex, insertionIndex);
            d->categoryModels.insert(insertionIndex, categoryModel);
            endInsertRows();
        }
        categoryModel->append(entry);
        categoryModel->addCategoryEntry(splitName.join("/"), entry);
    }
}

QObject* CategoryEntriesModel::get(int index)
{
    BookEntry* entry = new BookEntry();
    bool deleteEntry = true;
    if(index > -1 && index < d->entries.count())
    {
        entry = d->entries.at(index);
        deleteEntry = false;
    }
    QObject* obj = d->wrapBookEntry(entry);
    if(deleteEntry)
    {
        delete entry;
    }
    return obj;
}

int CategoryEntriesModel::indexOfFile(QString filename)
{
    int index = -1, i = 0;
    if(QFile::exists(filename))
    {
        Q_FOREACH(BookEntry* entry, d->entries)
        {
            if(entry->filename == filename)
            {
                index = i;
                break;
            }
            ++i;
        }
    }
    return index;
}

bool CategoryEntriesModel::indexIsBook(int index)
{
    if(index < d->categoryModels.count() || index >= rowCount()) {
        return false;
    }
    return true;
}

int CategoryEntriesModel::bookCount() const
{
    return d->entries.count();
}

QObject* CategoryEntriesModel::getEntry(int index)
{
    PropertyContainer* obj = new PropertyContainer("book", this);
    if(index < 0 && index > rowCount() -1) {
        // don't be a silly person, you can't get a nothing...
    }
    else if(index > d->categoryModels.count()) {
        // This is a book - get a book!
        obj = qobject_cast<PropertyContainer*>(get(index - d->categoryModels.count()));
    }
    else {
        CategoryEntriesModel* catEntry = d->categoryModels.at(index);
        obj->setProperty("title", catEntry->name());
        obj->setProperty("categoryEntriesCount", catEntry->bookCount());
        obj->setProperty("entriesModel", QVariant::fromValue(catEntry));
    }
    return obj;
}

QObject* CategoryEntriesModel::bookFromFile(QString filename)
{
    PropertyContainer* obj = qobject_cast<PropertyContainer*>(get(indexOfFile(filename)));
    if(obj->property("filename").toString().isEmpty()) {
        if(QFileInfo::exists(filename)) {
            QFileInfo info(filename);
            obj->setProperty("title", info.completeBaseName());
            obj->setProperty("created", info.created());

            KFileMetaData::UserMetaData data(filename);
            if (data.hasAttribute("peruse.currentPage")) {
                int currentPage = data.attribute("peruse.currentPage").toInt();
                obj->setProperty("currentPage", QVariant::fromValue<int>(currentPage));
            }
            if (data.hasAttribute("peruse.totalPages")) {
                int totalPages = data.attribute("peruse.totalPages").toInt();
                obj->setProperty("totalPages", QVariant::fromValue<int>(totalPages));
            }
            obj->setProperty("filename", filename);

            QString thumbnail;
            if(filename.toLower().endsWith("cbr")) {
                thumbnail = QString("image://comiccover/").append(filename);
            }
#ifdef USE_PERUSE_PDFTHUMBNAILER
            else if(filename.toLower().endsWith("pdf")) {
                thumbnail = QString("image://pdfcover/").append(filename);
            }
#endif
            else {
                thumbnail = QString("image://preview/").append(filename);
            }
            obj->setProperty("thumbnail", thumbnail);
        }
    }
    return obj;
}

void CategoryEntriesModel::entryDataChanged(BookEntry* entry)
{
    int entryIndex = d->entries.indexOf(entry) + d->categoryModels.count();
    QModelIndex changed = index(entryIndex);
    dataChanged(changed, changed);
}

void CategoryEntriesModel::entryRemove(BookEntry* entry)
{
    int listIndex = d->entries.indexOf(entry);
    if(listIndex > -1) {
        int entryIndex = listIndex + d->categoryModels.count();
        beginRemoveRows(QModelIndex(), entryIndex, entryIndex);
        d->entries.removeAll(entry);
        endRemoveRows();
    }
}
