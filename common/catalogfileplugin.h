/* ****************************************************************************
  This file is part of KBabel

  Copyright (C) 2002-2003	 by Stanislav Visnovsky
                        	    <visnovsky@kde.org>

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
#ifndef IMPORTPLUGIN_H
#define IMPORTPLUGIN_H

#include <QObject>
#include <QFile>
#include <QList>
#include <QLinkedList>

#include <kdemacros.h>
//#include "kbabel_export.h"

class QString;

//namespace KBabel 
//{

class Catalog;
class CatalogItem;
class CatalogImportPluginPrivate;
class CatalogExportPluginPrivate;

// ### KDE4: force OK=0
/**
 * Result of the conversion
 */
enum ConversionStatus { 
    OK=0,
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
* This class is the base for import plugins for catalogs.
* It provides "transactional behavior", so the changes are stored in 
* catalog only if the import process finishes successfully. 
*
* To use it, just subclass and redefine load() and id() methods.
* When importing, you can use the protected methods for setting
* the catalog. New catalog items can be added using appendCatalogItem.
*
* @short Base class for Catalog import plugins
* @author Stanislav Visnovsky <visnovsky@kde.org>
*/    
class /*KBABELCOMMON_EXPORT*/ CatalogImportPlugin: public QObject
{
     Q_OBJECT

public:
    CatalogImportPlugin(QObject* parent, const char* name);
    virtual ~CatalogImportPlugin();
    
    /**
     * Load the file and fill the corresponding catalog. The file
     * is considered to be of @ref mimetype MIME type.
     * 
     * @param file     local file name to be opened
     * @param mimetype the MIME type is should be handled as
     * @param catalog  the catalog to be filled
     * @return result of the operation
     */
    ConversionStatus open(const QString& file, const QString& mimetype, Catalog* catalog);
    
    /**
    * Reimplement this method to load the local file passed as an argument.
    * Throughout the run, you can use the protected methods for setting
    * the contents of the resulting catalog.
    * This method must call \see setMimeTypes to setup correct MIME types
    * for the loaded file. Also, it should use \see isStopped to 
    * abort loading and the signals for providing user feedback.
    * @param file file to be loaded
    * @param mimetype the expected MIME type (the type used for plugin selection
    */
    virtual ConversionStatus load(const QString& file, const QString& mimetype) = 0;
    /**
    * Reimplement this method to return unique identification of your plugin
    */
    virtual const QString id() = 0;
    
    /** @return the list of all available MIME types for which there
     *  is a import plugin.
     */
    static QStringList availableImportMimeTypes();

public slots:
    /** stop the current operation */
    void stop();

protected:
    /** Append a new catalog item, either as normal or as an obsolete one
     *  @param item the new item
     *  @param obsolete flag that the item is obsolete
     */
    void appendCatalogItem( const CatalogItem& item, const bool obsolete = false );
    
    /** set flag that the file is generated from DocBook */
    void setGeneratedFromDocbook(const bool fromDocbook);
    /** set the list of parse error indexes */
    void setErrorIndex(const QList<uint>& errors);
    /** set the file codec */
    void setFileCodec(QTextCodec* codec);
    
    /** set extra data for the catalog, which can't be stored in
     *  @ref CatalogItem. The format can be arbitrary */
    void setCatalogExtraData( const QStringList& data );
    /** set the header catalog item */
    void setHeader( const CatalogItem& header );
    /** set the MIME types which can be used for this catalog */
    void setMimeTypes( const QString& catalog );

    /** start a new transaction. You should never call this method. */
    void startTransaction();
    /** commit the data in the current transaction. You should never call this method. */
    void commitTransaction(const QString& a=QString());
    
    /** Flag, whether the operation should be stopped immediately.*/
    bool isStopped() const;
    
    int _maxLineLength;
// signals:
//     /** Signal start of the operation */
//     void signalResetProgressBar(QString,int);
//     /** Signal end of the operation */
//     void signalClearProgressBar();

private:
    CatalogImportPluginPrivate* d;
};

/**
* This class is the base for export plugins for catalogs.
*
* To use it, just subclass and redefine the save() method.
*
* @short Base class for Catalog export plugins
* @author Stanislav Visnovsky <visnovsky@kde.org>
*/    
class /*KBABELCOMMON_EXPORT*/ CatalogExportPlugin: public QObject
{
    Q_OBJECT

public:
    CatalogExportPlugin(QObject* parent, const char* name);
    virtual ~CatalogExportPlugin();
    virtual ConversionStatus save(const QString& file, const QString& mimetype, const Catalog* catalog) = 0;

    static QStringList availableExportMimeTypes();

public slots:
    void stop();

protected:
    bool isStopped() const;
    
// signals:
//     void signalResetProgressBar(QString,int);
//     void signalProgress(int);
//     void signalClearProgressBar();

private:
    CatalogExportPluginPrivate* d;
};

//}


#endif
