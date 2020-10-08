/*
 * Copyright (C) 2020 Dan Leinir Turthra Jensen <admin@leinir.dk>
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

#include "AcbfIdentifiedObjectModel.h"
#include "AcbfDocument.h"
#include "AcbfInternalReferenceObject.h"
#include "AcbfData.h"
#include "AcbfReferences.h"

using namespace AdvancedComicBookFormat;

class IdentifiedObjectModel::Private {
public:
    Private(IdentifiedObjectModel* qq)
        : q(qq)
    {}
    IdentifiedObjectModel* q{nullptr};
    Document* document{nullptr};
    QObjectList identifiedObjects;

    void addAndConnectChild(QObject* child) {
        int idx = identifiedObjects.count();
        q->beginInsertRows(QModelIndex(), idx, idx);
        identifiedObjects.append(child);
        q->endInsertRows();
        QObject::connect(child, &QObject::destroyed, q, [this, child](){
            int idx = identifiedObjects.indexOf(child);
            q->beginRemoveRows(QModelIndex(), idx, idx);
            identifiedObjects.removeOne(child);
            q->endRemoveRows();
            child->disconnect(q);
        });
    }
};

IdentifiedObjectModel::IdentifiedObjectModel(QObject* parent)
    : QAbstractListModel(parent)
    , d(new Private(this))
{
}

IdentifiedObjectModel::~IdentifiedObjectModel() = default;

QHash<int, QByteArray> IdentifiedObjectModel::roleNames() const
{
    static const QHash<int, QByteArray> roleNames{
        {IdRole, "id"},
        {TypeRole, "type"},
        {ObjectRole, "object"}
    };
    return roleNames;
}

QVariant IdentifiedObjectModel::data(const QModelIndex& index, int role) const
{
    QVariant data;
    if (checkIndex(index) && d->document) {
        QObject* object = d->identifiedObjects.value(index.row());
        if (object) {
            switch(role) {
                case IdRole:
                    data.setValue(object->property("id"));
                    break;
                case TypeRole:
                    if (qobject_cast<Reference*>(object)) {
                        data.setValue<int>(ReferenceType);
                    } else if (qobject_cast<Binary*>(object)) {
                        data.setValue<int>(BinaryType);
                    } else {
                        data.setValue<int>(UnknownType);
                    }
                    break;
                case ObjectRole:
                    data.setValue<QObject*>(object);
                    break;
                default:
                    break;
            };
        }
    }
    return data;
}

int IdentifiedObjectModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid()) {
        return 0;
    }
    return d->identifiedObjects.count();
}

QObject * IdentifiedObjectModel::document() const
{
    return d->document;
}

void IdentifiedObjectModel::setDocument(QObject* document)
{
    if (d->document != document) {
        beginResetModel();
        for (QObject* obj : d->identifiedObjects) {
            obj->disconnect(this);
        }
        d->identifiedObjects.clear();
        d->document = qobject_cast<Document*>(document);
        if (d->document) {
            std::function<void(const QObject* parent)> findAllIdentifiedObjects;
            findAllIdentifiedObjects = [&findAllIdentifiedObjects, this](const QObject *parent) {
                for (QObject *child : parent->children()) {
                    InternalReferenceObject* refObj = qobject_cast<InternalReferenceObject*>(child);
                    if (refObj) {
                        d->addAndConnectChild(refObj);
                    }
                    findAllIdentifiedObjects(child);
                }
            };
            findAllIdentifiedObjects(d->document);
            connect(d->document->data(), &Data::binaryAdded, this, [this](QObject* child){ d->addAndConnectChild(child);});
            connect(d->document->references(), &References::referenceAdded, this, [this](QObject* child){ d->addAndConnectChild(child);});
        }
        endResetModel();
        Q_EMIT documentChanged();
    }
}
