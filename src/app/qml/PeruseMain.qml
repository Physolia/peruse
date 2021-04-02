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

import QtQuick 2.15

import org.kde.kirigami 2.14 as Kirigami
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.14 as QQC2

import org.kde.peruse 0.1 as Peruse
import org.kde.contentlist 0.1

/**
 * @brief main application window.
 * 
 * This splits the window in two sections:
 * - A section where you can select comics.
 * - A "global drawer" which can be used to switch between categories
 *   and access settings and the open book dialog.
 * 
 * The global drawer controls which is the main component on the left.
 * It initializes on WelcomePage. The category filters are each handled
 * by a BookShelf. The store page by Store and the settings by Settings.
 * 
 * This also controls the bookViewer, which is a Book object where the
 * main reading of comics is done.
 * 
 * There is also the PeruseContextDrawer, which is only accessible on the book
 * page and requires flicking in from the right.
 */
Kirigami.ApplicationWindow {
    id: mainWindow;
    title: i18nc("@title:window the generic descriptive title of the application", "Comic Book Reader");
    property int animationDuration: 200;
    property bool isLoading: true;
    pageStack.initialPage: welcomePage;
    visible: true;
    // If the controls are not visible, being able to drag the pagestack feels really weird,
    // so we just turn that ability off :)
    pageStack.interactive: controlsVisible;

    property bool bookOpen: mainWindow.pageStack.layers.currentItem.objectName === "bookViewer";
    function showBook(filename, currentPage) {
        if(bookOpen) {
            mainWindow.pageStack.layers.pop();
        }
        mainWindow.pageStack.layers.push(bookViewer, { focus: true, file: filename, currentPage: currentPage })
        peruseConfig.bookOpened(filename);
    }

    Peruse.BookListModel {
        id: contentList;
        contentModel: ContentList {
            autoSearch: false

            onSearchStarted: { mainWindow.isLoading = true; }
            onSearchCompleted: { mainWindow.isLoading = false; }

            ContentQuery {
                type: ContentQuery.Comics
                locations: peruseConfig.bookLocations
            }
        }
        onCacheLoadedChanged: {
            if(!cacheLoaded) {
                return;
            }
            contentList.contentModel.setKnownFiles(contentList.knownBookFiles());
            contentList.contentModel.startSearch()
        }
    }

    Peruse.Config {
        id: peruseConfig;
    }
    function homeDir() {
        return peruseConfig.homeDir();
    }

    contextDrawer: PeruseContextDrawer {
        id: contextDrawer;
    }

    globalDrawer: Kirigami.GlobalDrawer {
        id: globalDrawer
        title: i18nc("application title for the sidebar", "Peruse");
        titleIcon: "peruse";
        drawerOpen: !Kirigami.Settings.isMobile && mainWindow.width > globalDrawer.availableWidth * 3
        modal: Kirigami.Settings.isMobile || mainWindow.width <= globalDrawer.availableWidth * 3

        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        contentItem.implicitWidth: Kirigami.Units.gridUnit * 14

        header: Kirigami.AbstractApplicationHeader {
            topPadding: Kirigami.Units.smallSpacing / 2;
            bottomPadding: Kirigami.Units.smallSpacing / 2;
            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.smallSpacing

            RowLayout {
                anchors.fill: parent

                Kirigami.Heading {
                    text: i18n("Navigation")
                    Layout.fillWidth: true
                }

                QQC2.ToolButton {
                    icon.name: "go-home"

                    enabled: mainWindow.currentCategory !== "welcomePage";
                    onClicked: {
                        if (changeCategory(welcomePage)) {
                            pageStack.currentItem.updateRecent();
                        }
                    }

                    QQC2.ToolTip {
                        text: i18n("Show intro page")
                    }
                }
            }
        }

        QQC2.ButtonGroup {
            id: placeGroup
        }

        QQC2.ScrollView {
            id: scrollView

            Layout.topMargin: -Kirigami.Units.smallSpacing;
            Layout.bottomMargin: -Kirigami.Units.smallSpacing;
            Layout.fillHeight: true
            Layout.fillWidth: true

            // In case we want to upstream it to Kirigami later:
            // PlaceHeading and PlaceItem have been contributed by Carl Schwan under LGPL-2.1-or-later license.
            component PlaceHeading : Kirigami.Heading {
                topPadding: Kirigami.Units.largeSpacing
                leftPadding: Kirigami.Units.largeSpacing
                Layout.fillWidth: true
                level: 6
                opacity: 0.7
            }

            component PlaceItem : QQC2.ItemDelegate {
                id: item
                signal triggered;
                checkable: true
                Layout.fillWidth: true
                Keys.onDownPressed: nextItemInFocusChain().forceActiveFocus(Qt.TabFocusReason)
                Keys.onUpPressed: nextItemInFocusChain(false).forceActiveFocus(Qt.TabFocusReason)
                Accessible.role: Accessible.MenuItem
                highlighted: checked
                onToggled: if (checked) {
                    item.triggered();
                }
                contentItem: Row {
                    Kirigami.Icon {
                        source: item.icon.name
                        width: height
                        height: Kirigami.Units.iconSizes.small
                        color: item.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                    }
                    QQC2.Label {
                        leftPadding: Kirigami.Units.smallSpacing
                        text: item.text
                        color: item.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                    }
                }
            }

            ColumnLayout {
                spacing: 1
                width: scrollView.width
                PlaceItem {
                    text: i18nc("Switch to the listing page showing the most recently read books", "Home");
                    icon.name: "go-home";
                    checked: true
                    QQC2.ButtonGroup.group: placeGroup
                    onTriggered: {
                        if (changeCategory(welcomePage)) {
                            pageStack.currentItem.updateRecent();
                        }
                    }
                }
                PlaceItem {
                    text: i18nc("Switch to the listing page showing the most recently discovered books", "Recently Added Books");
                    icon.name: "appointment-new";
                    QQC2.ButtonGroup.group: placeGroup
                    onTriggered: changeCategory(bookshelfAdded);
                }
                PlaceItem {
                    text: i18nc("Open a book from somewhere on disk (uses the open dialog, or a drilldown on touch devices)", "Open Other...");
                    icon.name: "document-open";
                    onClicked: openOther();
                    QQC2.ButtonGroup.group: undefined
                    checkable: false
                }
                PlaceHeading {
                    text: i18nc("Heading for switching to listing page showing items grouped by some properties", "Group By")
                }
                PlaceItem {
                    text: i18nc("Switch to the listing page showing items grouped by title", "Title");
                    icon.name: "view-media-title";
                    onTriggered: changeCategory(bookshelfTitle);
                    QQC2.ButtonGroup.group: placeGroup
                }
                PlaceItem {
                    text: i18nc("Switch to the listing page showing items grouped by author", "Author");
                    icon.name: "actor";
                    onTriggered: changeCategory(bookshelfAuthor);
                    QQC2.ButtonGroup.group: placeGroup
                }
                PlaceItem {
                    text: i18nc("Switch to the listing page showing items grouped by series", "Series");
                    icon.name: "edit-group";
                    onTriggered: changeCategory(bookshelfSeries);
                    QQC2.ButtonGroup.group: placeGroup
                }
                PlaceItem {
                    text: i18nc("Switch to the listing page showing items grouped by publisher", "Publisher");
                    icon.name: "view-media-publisher";
                    onTriggered: changeCategory(bookshelfPublisher);
                    QQC2.ButtonGroup.group: placeGroup
                }
                PlaceItem {
                    text: i18nc("Switch to the listing page showing items grouped by keywords, characters or genres", "Keywords");
                    icon.name: "tag";
                    onTriggered: changeCategory(bookshelfKeywords);
                    QQC2.ButtonGroup.group: placeGroup
                }
                PlaceItem {
                    text: i18nc("Switch to the listing page showing items grouped by their filesystem folder", "Folder");
                    icon.name: "tag-folder";
                    onTriggered: changeCategory(bookshelfFolder);
                    QQC2.ButtonGroup.group: placeGroup
                }

                PlaceHeading {
                    text: i18n("Peruse")
                }

                PlaceItem {
                    text: i18nc("Open the settings page", "Settings");
                    icon.name: "configure"
                    onTriggered: changeCategory(settingsPage);
                    QQC2.ButtonGroup.group: placeGroup
                }
                PlaceItem {
                    text: i18nc("Open the about page", "About");
                    icon.name: "help-about"
                    onTriggered: changeCategory(aboutPage);
                    QQC2.ButtonGroup.group: placeGroup
                }
            }
        }

        // HACK: this is needed because when clicking on the close button, drawerOpen get set to false (instead of the binding)
        // and when !Kirigami.Settings.isMobile && mainWindow.width > globalDrawer.availableWidth * 3 change, the Binding element
        // overwrite the last assignment to false and set drawerOpen to true or false depending on the value of the condition
        Binding {
            target: globalDrawer
            property: "drawerOpen"
            value: !Kirigami.Settings.isMobile && mainWindow.width > globalDrawer.availableWidth * 3
        }
    }

    Component {
        id: welcomePage;
        WelcomePage {
            onBookSelected: mainWindow.showBook(filename, currentPage);
        }
    }

    Component {
        id: bookViewer;
        Book {
            id: viewerRoot;
            onCurrentPageChanged: {
                contentList.setBookData(viewerRoot.file, "currentPage", viewerRoot.currentPage);
            }
            onTotalPagesChanged: {
                contentList.setBookData(viewerRoot.file, "totalPages", viewerRoot.totalPages);
            }
        }
    }

    Component {
        id: bookshelfTitle;
        Bookshelf {
            model: contentList.titleCategoryModel;
            headerText: i18nc("Title of the page with books grouped by the title start letters", "Group by Title");
            onBookSelected: mainWindow.showBook(filename, currentPage);
            categoryName: "bookshelfTitle";
        }
    }

    Component {
        id: bookshelfAdded;
        Bookshelf {
            model: contentList.newlyAddedCategoryModel;
            headerText: i18nc("Title of the page with all books ordered by which was added most recently", "Recently Added Books");
            sectionRole: "created";
            sectionCriteria: ViewSection.FullString;
            onBookSelected: mainWindow.showBook(filename, currentPage);
            categoryName: "bookshelfAdded";
        }
    }

    Component {
        id: bookshelfSeries;
        Bookshelf {
            model: contentList.seriesCategoryModel;
            headerText: i18nc("Title of the page with books grouped by what series they are in", "Group by Series");
            onBookSelected: mainWindow.showBook(filename, currentPage);
            categoryName: "bookshelfSeries";
        }
    }

    Component {
        id: bookshelfAuthor;
        Bookshelf {
            model: contentList.authorCategoryModel;
            headerText: i18nc("Title of the page with books grouped by author", "Group by Author");
            onBookSelected: mainWindow.showBook(filename, currentPage);
            categoryName: "bookshelfAuthor";
        }
    }

    Component {
        id: bookshelfPublisher;
        Bookshelf {
            model: contentList.publisherCategoryModel;
            headerText: i18nc("Title of the page with books grouped by who published them", "Group by Publisher");
            onBookSelected: mainWindow.showBook(filename, currentPage);
            categoryName: "bookshelfPublisher";
        }
    }

    Component {
        id: bookshelfKeywords;
        Bookshelf {
            model: contentList.keywordCategoryModel;
            headerText: i18nc("Title of the page with books grouped by keywords, character or genres", "Group by Keywords, Characters and Genres");
            onBookSelected: mainWindow.showBook(filename, currentPage);
            categoryName: "bookshelfKeywords";
        }
    }

    Component {
        id: bookshelfFolder;
        Bookshelf {
            model: contentList.folderCategoryModel;
            headerText: i18nc("Title of the page with books grouped by what folder they are in", "Filter by Folder");
            onBookSelected: mainWindow.showBook(filename, currentPage);
            categoryName: "bookshelfFolder";
        }
    }

    Component {
        id: bookshelf;
        Bookshelf {
            onBookSelected: mainWindow.showBook(filename, currentPage);
        }
    }

    Component {
        id: storePage;
        Store {
        }
    }

    Component {
        id: settingsPage;
        Settings {
        }
    }

    Component {
        id: aboutPage
        About {
        }
    }

    property string currentCategory: "welcomePage";
    property Component currentCategoryItem: welcomePage;
    function changeCategory(categoryItem) {
        if (categoryItem === mainWindow.currentCategoryItem) {
            return false;
        }
        // Clear all the way to the welcome page if we change the category...
        mainWindow.pageStack.clear();
        mainWindow.pageStack.push(categoryItem);
        currentCategory = mainWindow.pageStack.currentItem.categoryName;
        currentCategoryItem = categoryItem;
        if (PLASMA_PLATFORM.substring(0, 5) === "phone") {
            globalDrawer.close();
        }
        return true;
    }


    Component.onCompleted: {
        if (fileToOpen !== "") {
            mainWindow.showBook(fileToOpen, 0);
        }
    }
}

