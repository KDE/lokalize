/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2011-2012 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef QAMODEL_H
#define QAMODEL_H

#include "rule.h"
#include <QAbstractListModel>
#include <QDomDocument>

class QaModel : public QAbstractListModel
{
    // Q_OBJECT
public:
    enum Columns {
        // ID = 0,
        Source = 0,
        FalseFriend,
        ColumnCount,
    };

    explicit QaModel(QObject *parent = nullptr /*, Glossary* glossary*/);
    ~QaModel() override;

    bool loadRules(const QString &filename);
    bool saveRules(QString filename = QString());

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return ColumnCount;
        Q_UNUSED(parent)
    }
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant data(const QModelIndex &, int role = Qt::DisplayRole) const override;

    QVector<Rule> toVector() const;

    QModelIndex appendRow();
    void removeRow(const QModelIndex &);

    static QaModel *instance();
    static bool isInstantiated();

private:
    static QaModel *_instance;
    static void cleanupQaModel();

private:
    QDomDocument m_doc;
    QDomNodeList m_entries;
    QString m_filename;
};

#endif // QAMODEL_H
