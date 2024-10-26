/*
  This file is part of KAider
  This file contains parts of KBabel code

  SPDX-FileCopyrightText: 2002-2003 Stanislav Visnovsky
                                <visnovsky@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH LicenseRef-Qt-Commercial-exception-1.0*/
#ifndef CATALOGFILEPLUGIN_H
#define CATALOGFILEPLUGIN_H

#include <QList>
class QIODevice;
class QString;

namespace GettextCatalog
{

class GettextStorage;
class CatalogItem;
class CatalogImportPluginPrivate;
class CatalogExportPluginPrivate;

/**
 * Result of the conversion
 */
enum ConversionStatus {
    OK = 0,
    NOT_IMPLEMENTED,
    NO_FILE,
    NO_PERMISSIONS,
    PARSE_ERROR,
    RECOVERED_PARSE_ERROR,
    OS_ERROR,
    NO_PLUGIN,
    UNSUPPORTED_TYPE,
    RECOVERED_HEADER_ERROR, ///< Header error that could be recovered @since 1.11.2 (KDE 3.5.2)
    STOPPED,
    BUSY,
    NO_ENTRY_ERROR ///< The loaded catalog has not any entry! @since 1.11.2 (KDE 3.5.2)
};

/**
 * HISTORY: this was a base class for Catalog import plugins in KBabel,
 * but this architecture isn't not suitable for XML-based files
 * (when whole DOM-tree is stored in memory to prevent file clashes)
 *
 * This class is the base for import plugins for catalogs.
 * It provides "transactional behavior", so the changes are stored in
 * catalog only if the import process finishes successfully.
 *
 * To use it, just subclass and redefine load() and id() methods.
 * When importing, you can use the protected methods for setting
 * the catalog. New catalog items can be added using appendCatalogItem.
 *
 * @short Base class for GettextCatalog import plugins
 * @author Stanislav Visnovsky <visnovsky@kde.org>
 */
class CatalogImportPlugin
{
public:
    CatalogImportPlugin();
    virtual ~CatalogImportPlugin();

    Q_DISABLE_COPY(CatalogImportPlugin)
    /**
     * Load the file and fill the corresponding catalog. The file
     * is considered to be of @p mimetype MIME type.
     *
     * @param device   local file name to be opened
     * @param catalog  the catalog to be filled
     * @param errorLine the number of erroneous line
     * @return result of the operation
     */
    ConversionStatus open(QIODevice *, GettextStorage *catalog, int *errorLine);

    /**
     * Reimplement this method to load the local file passed as an argument.
     * Throughout the run, you can use the protected methods for setting
     * the contents of the resulting catalog.
     * This method must call \see setMimeTypes to setup correct MIME types
     * for the loaded file. Also, it should use \see isStopped to
     * abort loading and the signals for providing user feedback.
     * @param device file to be loaded
     */
    virtual ConversionStatus load(QIODevice *) = 0;

protected:
    /** Append a new catalog item, either as normal or as an obsolete one
     *  @param item the new item
     *  @param obsolete flag that the item is obsolete
     */
    void appendCatalogItem(const CatalogItem &item, const bool obsolete = false);

    /** set flag that the file is generated from DocBook */
    void setGeneratedFromDocbook(const bool fromDocbook);
    /** set the list of parse error indexes */
    void setErrorIndex(const QList<int> &errors);

    /** set extra data for the catalog, which can't be stored in
     *  @ref CatalogItem. The format can be arbitrary */
    void setCatalogExtraData(const QStringList &data);
    /** set the header catalog item */
    void setHeader(const CatalogItem &header);

    /** Set the character encoding used in the catalog file. */
    void setCodec(const QByteArray &codec);

    /** start a new transaction. You should never call this method. */
    void startTransaction();
    /** commit the data in the current transaction. You should never call this method. */
    void commitTransaction();

    short _maxLineLength{0};
    int _errorLine{0};

private:
    CatalogImportPluginPrivate *d;
};

}

#endif
