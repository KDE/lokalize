/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2007-2015 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2019 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2024-2025 Finley Watson <fin-w@tutanota.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "projectwidget.h"
#include "catalog.h"
#include "headerviewmenu.h"
#include "lokalize_debug.h"
#include "project.h"

#include <KColorScheme>
#include <KDirLister>
#include <KDirSortFilterProxyModel>
#include <KStringHandler>

#include <QApplication>
#include <QCollator>
#include <QHeaderView>
#include <QItemDelegate>
#include <QKeyEvent>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QtAssert>

class PoItemDelegate : public QStyledItemDelegate
{
public:
    explicit PoItemDelegate(QObject *parent = nullptr);
    ~PoItemDelegate() override = default;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QString displayText(const QVariant &value, const QLocale &locale) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    KColorScheme m_colorScheme{QPalette::Normal};
};

PoItemDelegate::PoItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize PoItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString text = index.data().toString();
    int lineCount = 1;
    int nPos = text.indexOf(QLatin1Char('\n'));
    if (nPos == -1)
        nPos = text.size();
    else
        lineCount += text.count(QLatin1Char('\n'));
    static QFontMetrics metrics(option.font);
    return QSize(metrics.averageCharWidth() * nPos, metrics.height() * lineCount);
}

void PoItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (static_cast<ProjectModel::ProjectModelColumns>(index.column()) != ProjectModel::ProjectModelColumns::Graph)
        return QStyledItemDelegate::paint(painter, option, index);

    QVariant graphData = index.data(Qt::DisplayRole);
    if (Q_UNLIKELY(!graphData.isValid())) {
        painter->fillRect(option.rect, Qt::transparent);
        return;
    }

    QRect rect = graphData.toRect();
    int translated = rect.left();
    int untranslated = rect.top();
    int fuzzy = rect.width();
    int total = translated + untranslated + fuzzy;

    if (total > 0) {
        QBrush brush;
        painter->setPen(Qt::white);
        QRect myRect(option.rect);

        myRect.setWidth(option.rect.width() * translated / total);
        if (translated) {
            brush = m_colorScheme.foreground(KColorScheme::PositiveText);
            painter->fillRect(myRect, brush);
        }

        myRect.setLeft(myRect.left() + myRect.width());
        myRect.setWidth(option.rect.width() * fuzzy / total);
        if (fuzzy) {
            brush = m_colorScheme.foreground(KColorScheme::NeutralText);
            painter->fillRect(myRect, brush);
        }

        myRect.setLeft(myRect.left() + myRect.width());
        myRect.setWidth(option.rect.width() - myRect.left() + option.rect.left());
        if (untranslated)
            brush = m_colorScheme.foreground(KColorScheme::NegativeText);
        // esle: paint what is left with the last brush used - blank, positive or neutral

        painter->fillRect(myRect, brush);
    } else if (total == -1)
        painter->fillRect(option.rect, Qt::transparent);
    else if (total == 0)
        painter->fillRect(option.rect, QBrush(Qt::gray));
}

// Temporary workaround for Qt bug https://bugreports.qt.io/browse/QTBUG-78094
// to ensure that large numbers are formatted using a thousands separator
QString PoItemDelegate::displayText(const QVariant &value, [[maybe_unused]] const QLocale &locale) const
{
    return QStyledItemDelegate::displayText(value, QLocale::system());
}

void ProjectOverviewSortFilterProxyModel::toggleTranslatedFiles()
{
    m_hideTranslatedFiles = !m_hideTranslatedFiles;
    invalidateFilter();
}

bool ProjectOverviewSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    bool result = false;
    const QAbstractItemModel *model = sourceModel();
    QModelIndex item = model->index(source_row, 0, source_parent);

    if (item.data(ProjectModel::DirectoryRole) == 1 && item.data(ProjectModel::TotalRole) == 0)
        return false; // Hide rows with no translations if they are folders.

    if (item.data(ProjectModel::FuzzyUntrCountAllRole) == 0 && m_hideTranslatedFiles)
        return false; // Hide rows with no untranslated items if the filter is enabled.

    if (!filterRegularExpression().isValid())
        return false;

    QString itemPathRelativeToProject = Project::instance()->relativePath(Project::instance()->model()->itemForIndex(item).localPath());
    // True if the search string matches the item's relative path.
    result = filterRegularExpression().match(itemPathRelativeToProject).hasMatch();

    // Without this no items are visible.
    int i = model->rowCount(item);
    while (--i >= 0 && !result)
        result = filterAcceptsRow(i, item);

    return result;
}

bool ProjectOverviewSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    static QCollator collator;
    ProjectModel *projectModel = static_cast<ProjectModel *>(sourceModel());
    const KFileItem leftFileItem = projectModel->itemForIndex(left);
    const KFileItem rightFileItem = projectModel->itemForIndex(right);

    // Code taken from KDirSortFilterProxyModel, as it is not compatible with our model.
    // TODO: make KDirSortFilterProxyModel::subSortLessThan not cast model to KDirModel, but use data() with FileItemRole instead.

    // Directories and hidden files should always be on the top, independent
    // from the sort order.
    const bool isLessThan = (sortOrder() == Qt::AscendingOrder);

    if (leftFileItem.isNull() || rightFileItem.isNull()) {
        qCWarning(LOKALIZE_LOG) << ".isNull()";
        return false;
    }

    // On our priority, folders go above regular files.
    if (leftFileItem.isDir() && !rightFileItem.isDir()) {
        return isLessThan;
    } else if (!leftFileItem.isDir() && rightFileItem.isDir()) {
        return !isLessThan;
    }

    // Hidden elements go before visible ones, if they both are
    // folders or files.
    if (leftFileItem.isHidden() && !rightFileItem.isHidden()) {
        return isLessThan;
    } else if (!leftFileItem.isHidden() && rightFileItem.isHidden()) {
        return !isLessThan;
    }

    // Hidden elements go before visible ones, if they both are
    // folders or files.
    if (leftFileItem.isHidden() && !rightFileItem.isHidden()) {
        return true;
    } else if (!leftFileItem.isHidden() && rightFileItem.isHidden()) {
        return false;
    }

    switch (static_cast<ProjectModel::ProjectModelColumns>(left.column())) {
    case ProjectModel::ProjectModelColumns::FileName:
        return collator.compare(leftFileItem.name(), rightFileItem.name()) < 0;
    case ProjectModel::ProjectModelColumns::Graph: {
        QRect leftRect(left.data(Qt::DisplayRole).toRect());
        QRect rightRect(right.data(Qt::DisplayRole).toRect());

        int leftAll = leftRect.left() + leftRect.top() + leftRect.width();
        int rightAll = rightRect.left() + rightRect.top() + rightRect.width();

        if (!leftAll || !rightAll)
            return false;

        float leftVal = (float)leftRect.left() / leftAll;
        float rightVal = (float)rightRect.left() / rightAll;

        if (leftVal < rightVal)
            return true;
        if (leftVal > rightVal)
            return false;

        leftVal = (float)leftRect.top() / leftAll;
        rightVal = (float)rightRect.top() / rightAll;

        if (leftVal < rightVal)
            return true;
        if (leftVal > rightVal)
            return false;

        leftVal = (float)leftRect.width() / leftAll;
        rightVal = (float)rightRect.width() / rightAll;

        if (leftVal < rightVal)
            return true;
        return false;
    }
    case ProjectModel::ProjectModelColumns::LastTranslator:
    case ProjectModel::ProjectModelColumns::SourceDate:
    case ProjectModel::ProjectModelColumns::TranslationDate:
    case ProjectModel::ProjectModelColumns::Comment:
        return collator.compare(projectModel->data(left).toString(), projectModel->data(right).toString()) < 0;
    case ProjectModel::ProjectModelColumns::TotalCount:
    case ProjectModel::ProjectModelColumns::TranslatedCount:
    case ProjectModel::ProjectModelColumns::UntranslatedCount:
    case ProjectModel::ProjectModelColumns::IncompleteCount:
    case ProjectModel::ProjectModelColumns::FuzzyCount:
        return projectModel->data(left).toInt() < projectModel->data(right).toInt();
    default:
        return false;
    }
}

ProjectWidget::ProjectWidget(QWidget *parent)
    : QTreeView(parent)
    , m_proxyModel(new ProjectOverviewSortFilterProxyModel(this))
{
    PoItemDelegate *delegate = new PoItemDelegate(this);
    setItemDelegate(delegate);

    connect(this, &ProjectWidget::activated, this, &ProjectWidget::slotItemActivated);

    m_proxyModel->setSourceModel(Project::instance()->model());
    setModel(m_proxyModel);
    connect(Project::instance()->model(), &ProjectModel::loadingAboutToStart, this, &ProjectWidget::modelAboutToReload);
    connect(Project::instance()->model(), &ProjectModel::loadingFinished, this, &ProjectWidget::modelReloaded, Qt::QueuedConnection);

    setUniformRowHeights(true);
    setAllColumnsShowFocus(true);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);

    new HeaderViewMenuHandler(header());

    KConfig config;
    KConfigGroup stateGroup(&config, QStringLiteral("ProjectWindow"));
    const bool stateRestoreIsSuccessful = (stateGroup.exists() && stateGroup.hasKey("ListHeaderState")
                                           && header()->restoreState(QByteArray::fromBase64(stateGroup.readEntry("ListHeaderState", QByteArray()))));
    if (!stateRestoreIsSuccessful) {
        // FileName, Graph, TotalCount, TranslatedCount, FuzzyCount, UntranslatedCount, IncompleteCount, Comment, SourceDate, TranslationDate, LastTranslator
        static const int widthDefaults[] = {3, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2};
        int i = sizeof(widthDefaults) / sizeof(int);
        Q_ASSERT(i == header()->count());
        int baseWidth = columnWidth(0);

        header()->resizeSections(QHeaderView::ResizeToContents);
        while (--i >= 0)
            if (widthDefaults[i])
                setColumnWidth(i, baseWidth * widthDefaults[i]);
    }
}

ProjectWidget::~ProjectWidget()
{
    KConfig config;
    KConfigGroup stateGroup(&config, QStringLiteral("ProjectWindow"));
    stateGroup.writeEntry("ListHeaderState", header()->saveState().toBase64());
}

void ProjectWidget::modelAboutToReload()
{
    m_currentItemPathBeforeReload = currentItem();
}

void ProjectWidget::modelReloaded()
{
    int i = 10;
    while (--i >= 0) {
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers | QEventLoop::WaitForMoreEvents, 100);
        if (setCurrentItem(m_currentItemPathBeforeReload))
            break;
    }
    if (proxyModel()->filterRegularExpression().pattern().size() > 2)
        expandItems();
}

bool ProjectWidget::setCurrentItem(const QString &u)
{
    if (u.isEmpty())
        return true;
    QModelIndex index = m_proxyModel->mapFromSource(Project::instance()->model()->indexForUrl(QUrl::fromLocalFile(u)));
    if (index.isValid())
        setCurrentIndex(index);
    return index.isValid();
}

QString ProjectWidget::currentItem() const
{
    if (!currentIndex().isValid())
        return QString();
    return Project::instance()->model()->itemForIndex(m_proxyModel->mapToSource(currentIndex())).localPath();
}

bool ProjectWidget::currentIsTranslationFile() const
{
    // remember 'bout empty state
    return Catalog::extIsSupported(currentItem());
}

void ProjectWidget::slotItemActivated(const QModelIndex &index)
{
    if (currentIsTranslationFile()) {
        ProjectModel *srcModel = static_cast<ProjectModel *>(static_cast<QSortFilterProxyModel *>(m_proxyModel)->sourceModel());
        QModelIndex srcIndex = static_cast<QSortFilterProxyModel *>(m_proxyModel)->mapToSource(index);
        QUrl fileUrl = srcModel->beginEditing(srcIndex);

        Q_EMIT fileOpenRequested(fileUrl.toLocalFile(), !(QApplication::keyboardModifiers() & Qt::ControlModifier));
    }
}

void ProjectWidget::recursiveAdd(QStringList &list, const QModelIndex &idx) const
{
    if (!m_proxyModel->filterAcceptsRow(idx.row(), idx.parent())) {
        return;
    }
    ProjectModel &model = *(Project::instance()->model());
    const KFileItem &item(model.itemForIndex(idx));
    if (item.isDir()) {
        int j = model.rowCount(idx);
        while (--j >= 0) {
            const KFileItem &childItem(model.itemForIndex(model.index(j, 0, idx)));

            if (childItem.isDir())
                recursiveAdd(list, model.index(j, 0, idx));
            else if (m_proxyModel->filterAcceptsRow(j, idx))
                list.prepend(childItem.localPath());
        }
    } else // if (!list.contains(u))
        list.prepend(item.localPath());
}

QStringList ProjectWidget::selectedItems() const
{
    QStringList list;
    const auto items = selectedIndexes();
    for (const QModelIndex &item : items) {
        if (item.column() == 0)
            recursiveAdd(list, m_proxyModel->mapToSource(item));
    }

    return list;
}

void ProjectWidget::expandItems(const QModelIndex &parent)
{
    const QAbstractItemModel *m = model();
    expand(parent);

    int i = m->rowCount(parent);
    while (--i >= 0)
        expandItems(m->index(i, 0, parent));
}

bool ProjectWidget::gotoIndexCheck(const QModelIndex &currentIndex, ProjectModel::AdditionalRoles role)
{
    // Check if role is found for this index
    if (currentIndex.isValid()) {
        ProjectModel *srcModel = static_cast<ProjectModel *>(static_cast<QSortFilterProxyModel *>(m_proxyModel)->sourceModel());
        QModelIndex srcIndex = static_cast<QSortFilterProxyModel *>(m_proxyModel)->mapToSource(currentIndex);
        QVariant result = srcModel->data(srcIndex, role);
        return result.isValid() && result.toInt() > 0;
    }
    return false;
}

QModelIndex ProjectWidget::gotoIndexPrevNext(const QModelIndex &currentIndex, int direction) const
{
    QModelIndex index = currentIndex;
    QModelIndex sibling;

    // Unless first or last sibling reached, continue with previous or next
    // sibling, otherwise continue with previous or next parent
    while (index.isValid()) {
        sibling = index.sibling(index.row() + direction, index.column());
        if (sibling.isValid())
            return sibling;
        index = index.parent();
    }
    return index;
}

ProjectWidget::gotoIndexResult ProjectWidget::gotoIndexFind(const QModelIndex &currentIndex, ProjectModel::AdditionalRoles role, int direction)
{
    QModelIndex index = currentIndex;

    while (index.isValid()) {
        // Set current index and show it if role is found for this index
        if (gotoIndexCheck(index, role)) {
            clearSelection();
            setCurrentIndex(index);
            scrollTo(index);
            return gotoIndex_found;
        }

        // Handle child recursively if index is not a leaf
        QModelIndex child = index.model()->index((direction == 1) ? 0 : (m_proxyModel->rowCount(index) - 1), index.column(), index);
        if (child.isValid()) {
            ProjectWidget::gotoIndexResult result = gotoIndexFind(child, role, direction);
            if (result != gotoIndex_notfound)
                return result;
        }

        // Go to previous or next item
        index = gotoIndexPrevNext(index, direction);
    }
    if (index.parent().isValid())
        return gotoIndex_notfound;
    else
        return gotoIndex_end;
}

ProjectWidget::gotoIndexResult ProjectWidget::gotoIndex(const QModelIndex &currentIndex, ProjectModel::AdditionalRoles role, int direction)
{
    QModelIndex index = currentIndex;

    // Check if current index already found, and if so go to previous or next item
    if (gotoIndexCheck(index, role))
        index = gotoIndexPrevNext(index, direction);

    return gotoIndexFind(index, role, direction);
}

void ProjectWidget::gotoPrevFuzzyUntr()
{
    gotoIndex(currentIndex(), ProjectModel::FuzzyUntrCountRole, -1);
}
void ProjectWidget::gotoNextFuzzyUntr()
{
    gotoIndex(currentIndex(), ProjectModel::FuzzyUntrCountRole, +1);
}
void ProjectWidget::gotoPrevFuzzy()
{
    gotoIndex(currentIndex(), ProjectModel::FuzzyCountRole, -1);
}
void ProjectWidget::gotoNextFuzzy()
{
    gotoIndex(currentIndex(), ProjectModel::FuzzyCountRole, +1);
}
void ProjectWidget::gotoPrevUntranslated()
{
    gotoIndex(currentIndex(), ProjectModel::UntransCountRole, -1);
}
void ProjectWidget::gotoNextUntranslated()
{
    gotoIndex(currentIndex(), ProjectModel::UntransCountRole, +1);
}
void ProjectWidget::gotoPrevTemplateOnly()
{
    gotoIndex(currentIndex(), ProjectModel::TemplateOnlyRole, -1);
}
void ProjectWidget::gotoNextTemplateOnly()
{
    gotoIndex(currentIndex(), ProjectModel::TemplateOnlyRole, +1);
}
void ProjectWidget::gotoPrevTransOnly()
{
    gotoIndex(currentIndex(), ProjectModel::TransOnlyRole, -1);
}
void ProjectWidget::gotoNextTransOnly()
{
    gotoIndex(currentIndex(), ProjectModel::TransOnlyRole, +1);
}
void ProjectWidget::toggleTranslatedFiles()
{
    m_proxyModel->toggleTranslatedFiles();
}

QSortFilterProxyModel *ProjectWidget::proxyModel()
{
    return m_proxyModel;
}

#include "moc_projectwidget.cpp"
