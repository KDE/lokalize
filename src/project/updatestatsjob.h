/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2018 by Karl Ove Hufthammer <karl@huftis.org>
  Copyright (C) 2007-2014 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2009 by Viesturs Zarins <viesturs.zarins@mii.lu.lv>
  Copyright (C) 2018-2019 by Simon Depiets <sdepiets@gmail.com>
  Copyright (C) 2019 by Alexander Potashev <aspotashev@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************** */

#ifndef LOKALIZE_UPDATESTATSJOB_H
#define LOKALIZE_UPDATESTATSJOB_H

#include <QRunnable>

#include <KFileItem>

#include "metadata/filemetadata.h"

class UpdateStatsJob: public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit UpdateStatsJob(const QList<KFileItem> &files, QObject* owner = nullptr);
    ~UpdateStatsJob() override = default;

    int priority() const
    {
        return 35;   //SEE jobs.h
    }

    void setStatus(int status);

    QList<KFileItem> m_files;
    QList<FileMetaData> m_info;
    volatile int m_status; // 0 = running; -1 = cancel; -2 = abort

protected:
    void run() override;

Q_SIGNALS:
    void done(UpdateStatsJob*);
};

#endif //LOKALIZE_UPDATESTATSJOB_H
