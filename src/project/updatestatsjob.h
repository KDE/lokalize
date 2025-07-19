/*
  This file is part of Lokalize

  SPDX-FileCopyrightText: 2018 Karl Ove Hufthammer <karl@huftis.org>
  SPDX-FileCopyrightText: 2007-2014 Nick Shaforostoff <shafff@ukr.net>
  SPDX-FileCopyrightText: 2009 Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  SPDX-FileCopyrightText: 2018-2019 Simon Depiets <sdepiets@gmail.com>
  SPDX-FileCopyrightText: 2019 Alexander Potashev <aspotashev@gmail.com>

  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef LOKALIZE_UPDATESTATSJOB_H
#define LOKALIZE_UPDATESTATSJOB_H

#include "metadata/filemetadata.h"

#include <QObject>
#include <QRunnable>

class KFileItem;

class UpdateStatsJob : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit UpdateStatsJob(const QList<KFileItem> &files, QObject *owner = nullptr);
    ~UpdateStatsJob() override = default;

    int priority() const
    {
        return 35; // SEE jobs.h
    }

    void setStatus(int status);

    QList<KFileItem> m_files;
    QList<FileMetaData> m_info;
    volatile int m_status{0}; // 0 = running; -1 = cancel; -2 = abort

protected:
    void run() override;

Q_SIGNALS:
    void done(UpdateStatsJob *);
};

#endif // LOKALIZE_UPDATESTATSJOB_H
