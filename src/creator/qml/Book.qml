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

import QtQuick 2.2

import org.kde.kirigami 1.0 as Kirigami
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.peruse 0.1 as Peruse

Kirigami.Page {
    id: root;
    property string categoryName: "book";
    title: i18nc("title of the main book editor page", "Editing %1").arg(bookModel.title == "" ? root.filename : bookModel.title);
    property string filename;

    actions {
        left: (editMetaInfo.opened || addPageSheet.opened) ? null : saveBookAction;
        main: editMetaInfo.opened ? closeEditMetaInfoAction : (addPageSheet.opened ? closeAddPageSheetAction : defaultMainAction);
        right: (editMetaInfo.opened || addPageSheet.opened) ? null : addPageAction;
    }
    Kirigami.Action {
        id: saveBookAction;
        text: i18nc("Saves the book to a file on disk", "Save Book");
        iconName: "document-save";
        onTriggered: bookModel.saveBook();
    }
    Kirigami.Action {
        id: addPageAction;
        text: i18nc("adds a new page at the end of the book", "Add Page");
        iconName: "list-add";
        onTriggered: addPage(bookModel.pageCount);
    }
    Kirigami.Action {
        id: defaultMainAction;
        text: i18nc("causes a dialog to show in which the user can edit the meta information for the entire book", "Edit Metainfo");
        iconName: "document-edit";
        onTriggered: editMetaInfo.open();
    }
    Kirigami.Action {
        id: closeEditMetaInfoAction;
        text: i18nc("closes the the meta information editing sheet", "Close Editor");
        iconName: "dialog-cancel";
        onTriggered: editMetaInfo.close();
    }
    Kirigami.Action {
        id: closeAddPageSheetAction;
        text: i18nc("closes the the add page sheet", "Do Not Add A Page");
        iconName: "dialog-cancel";
        onTriggered: addPageSheet.close();
    }

    Peruse.ArchiveBookModel {
        id: bookModel;
        qmlEngine: globalQmlEngine;
        readWrite: true;
        filename: root.filename;
    }

    function addPage(afterWhatIndex) {
        addPageSheet.addPageAfter = afterWhatIndex;
        addPageSheet.open();
    }
    AddPageSheet {
        id: addPageSheet;
        model: bookModel;
    }

    BookMetainfoSheet {
        id: editMetaInfo;
        model: bookModel;
    }

    Item {
        width: root.width - (root.leftPadding + root.rightPadding);
        height: root.height - (root.topPadding + root.bottomPadding);
        ListView {
            anchors.fill: parent;
            model: bookModel;
            delegate: Kirigami.SwipeListItem {
                id: listItem;
                height: units.iconSizes.huge + units.smallSpacing * 2;
                supportsMouseEvents: true;
                onClicked: ;
                actions: [
                    Kirigami.Action {
                        text: i18nc("swap the position of this page with the previous one", "Move Up");
                        iconName: "go-up"
                        onTriggered: { bookModel.swapPages(model.index, model.index - 1); }
                        enabled: model.index > 0;
                        visible: enabled;
                    },
                    Kirigami.Action {
                        text: i18nc("swap the position of this page with the next one", "Move Down");
                        iconName: "go-down"
                        onTriggered: { bookModel.swapPages(model.index, model.index + 1); }
                        enabled: model.index < bookModel.pageCount - 1;
                        visible: enabled;
                    },
                    Kirigami.Action {
                        text: i18nc("remove the page from the book", "Delete Page");
                        iconName: "list-remove"
                        onTriggered: {}
                    },
                    Kirigami.Action {
                        text: i18nc("add a page to the book after this one", "Add Page After This");
                        iconName: "list-add"
                        onTriggered: root.addPage(model.index);
                    }
                ]
                Item {
                    anchors.fill: parent;
                    Item {
                        id: bookCover;
                        anchors {
                            top: parent.top;
                            left: parent.left;
                            bottom: parent.bottom;
                        }
                        width: height;
                        Image {
                            id: coverImage;
                            anchors {
                                fill: parent;
                                margins: units.smallSpacing;
                            }
                            asynchronous: true;
                            fillMode: Image.PreserveAspectFit;
                            source: model.url;
                        }
                    }
                    Kirigami.Label {
                        anchors {
                            verticalCenter: parent.verticalCenter;
                            left: bookCover.right;
                            leftMargin: Kirigami.Units.largeSpacing;
                        }
                        text: model.title;
                    }
                }
            }
        }
    }
}