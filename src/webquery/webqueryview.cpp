/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#include "webqueryview.h"

#include "lokalize_debug.h"

#include "project.h"
#include "catalog.h"
#include "flowlayout.h"

#include "ui_querycontrol.h"

#include <kross/ui/view.h>
#include <kross/ui/model.h>
#include <kross/core/actioncollection.h>
#include <kross/core/manager.h>
#include <klocalizedstring.h>

#include "webquerycontroller.h"

#include <QDragEnterEvent>
#include <QTime>
#include <QSplitter>
#include <QTextBrowser>
#include <QTimer>

// #include <QShortcutEvent>
#include "myactioncollectionview.h"

using namespace Kross;

WebQueryView::WebQueryView(QWidget* parent, Catalog* catalog, const QVector<QAction*>& actions)
    : QDockWidget(i18n("Web Queries"), parent)
    , m_catalog(catalog)
    , m_splitter(new QSplitter(this))
    , m_browser(new QTextBrowser(m_splitter))
    , ui_queryControl(new Ui_QueryControl)
    , m_actions(actions)

{
    setObjectName(QStringLiteral("WebQueryView"));
    setWidget(m_splitter);

    hide();

    m_browser->viewport()->setBackgroundRole(QPalette::Window);

    m_browser->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QWidget* w = new QWidget(m_splitter);
    ui_queryControl->setupUi(w);

    QTimer::singleShot(0, this, &WebQueryView::initLater);
}

WebQueryView::~WebQueryView()
{
    delete ui_queryControl;
//     delete m_flowLayout;
}

void WebQueryView::initLater()
{
    connect(ui_queryControl->queryBtn, &QPushButton::clicked, ui_queryControl->actionzView, &MyActionCollectionView::triggerSelectedActions);

//     connect(this, &WebQueryView::addWebQueryResult, m_flowLayout, SLOT(addWebQueryResult(const QString&)));
//  ActionCollectionModel::Mode mode(
//                                      ActionCollectionModel::Icons
//                                     | ActionCollectionModel::ToolTips | ActionCollectionModel::UserCheckable );*/
    ActionCollectionModel* m = new ActionCollectionModel(ui_queryControl->actionzView, Manager::self().actionCollection()/*, mode*/);
    ui_queryControl->actionzView->setModel(m);
//     m_boxLayout->addWidget(w);
    ui_queryControl->actionzView->data.webQueryView = this;


    m_browser->setToolTip(i18nc("@info:tooltip", "Double-click any word to insert it into translation"));


    int i = m_actions.size();
    while (--i >= 0) {
        connect(m_actions.at(i), &QAction::triggered, this, [this, i] { slotUseSuggestion(i); });
    }
    connect(m_browser, &QTextBrowser::selectionChanged, this, &WebQueryView::slotSelectionChanged);

}

void WebQueryView::slotSelectionChanged()
{
    //NOTE works fine only for dbl-click word selection
    //(actually, quick word insertion is exactly the purpose of this slot:)
    QString sel(m_browser->textCursor().selectedText());
    if (!sel.isEmpty()) {
        Q_EMIT textInsertRequested(sel);
    }
}

//TODO text may be dragged
// void WebQueryView::dragEnterEvent(QDragEnterEvent* event)
// {
//     /*    if(event->mimeData()->hasUrls() && event->mimeData()->urls().first().path().endsWith(".po"))
//         {
//             //qCWarning(LOKALIZE_LOG) << " " <<;
//             event->acceptProposedAction();
//         };*/
// }
//
// void WebQueryView::dropEvent(QDropEvent *event)
// {
//     /*    Q_EMIT mergeOpenRequested(event->mimeData()->urls().first());
//         event->acceptProposedAction();*/
// }

void WebQueryView::slotNewEntryDisplayed(const DocPosition& pos)
{
    //m_flowLayout->clearWebQueryResult();
    m_browser->clear();
    m_suggestions.clear();

    ui_queryControl->actionzView->data.msg = m_catalog->msgid(pos);
    //TODO pass DocPosition also, as tmview does

    if (ui_queryControl->autoQuery->isChecked())
        ui_queryControl->actionzView->triggerSelectedActions();
}

void WebQueryView::slotUseSuggestion(int i)
{
    if (i >= m_suggestions.size())
        return;
    Q_EMIT textInsertRequested(m_suggestions.at(i));
}


void WebQueryView::addWebQueryResult(const QString& name, const QString& str)
{
    QString html(str);
    html.replace('<', "&lt;");
    html.replace('>', "&gt;");
    html.append(QString("<br><br>"));
    html.prepend(QString("[%2] /%1/ ").arg(name).arg(
                     (m_suggestions.size() < m_actions.size()) ?
                     m_actions.at(m_suggestions.size())->shortcut().toString() :
                     " - "));

    m_browser->insertHtml(html);
    //m_flowLayout->addWebQueryResult(str);

    m_suggestions.append(str);
}

