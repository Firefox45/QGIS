/***************************************************************************
                    QgsPluginRegistry.cpp  -  Singleton class for
                    tracking registering plugins.
                             -------------------
    begin                : Fri Feb 7 2004
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QMessageBox>

#include "qgssettings.h"
#include "qgis.h"
#include "qgsapplication.h"
#include "qgisinterface.h"
#include "qgspluginregistry.h"
#include "qgspluginmetadata.h"
#include "qgisplugin.h"
#include "qgisapp.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"

#ifdef WITH_BINDINGS
#include "qgspythonutils.h"
#endif

/* typedefs for plugins */
typedef QgisPlugin *create_ui( QgisInterface *qI );
typedef QString name_t();
typedef QString description_t();
typedef QString category_t();
typedef int type_t();


QgsPluginRegistry *QgsPluginRegistry::sInstance = nullptr;
QgsPluginRegistry *QgsPluginRegistry::instance()
{
  if ( !sInstance )
  {
    sInstance = new QgsPluginRegistry();
  }
  return sInstance;
}

void QgsPluginRegistry::setQgisInterface( QgisInterface *iface )
{
  mQgisInterface = iface;
}

void QgsPluginRegistry::setPythonUtils( QgsPythonUtils *pythonUtils )
{
  mPythonUtils = pythonUtils;
}

bool QgsPluginRegistry::isLoaded( const QString &key ) const
{
  QMap<QString, QgsPluginMetadata>::const_iterator it = mPlugins.find( key );
  if ( it != mPlugins.end() ) // found a c++ plugin?
    return true;

#ifdef WITH_BINDINGS
  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    return mPythonUtils->isPluginLoaded( key );
  }
#endif

  return false;
}

QString QgsPluginRegistry::library( const QString &key )
{
  QMap<QString, QgsPluginMetadata>::const_iterator it = mPlugins.constFind( key );
  if ( it != mPlugins.constEnd() )
    return it->library();

#ifdef WITH_BINDINGS
  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    if ( mPythonUtils->isPluginLoaded( key ) )
      return key;
  }
#endif

  return QString();
}

QgisPlugin *QgsPluginRegistry::plugin( const QString &key )
{
  QMap<QString, QgsPluginMetadata>::iterator it = mPlugins.find( key );
  if ( it == mPlugins.end() )
    return nullptr;

  // note: not used by python plugins

  return it->plugin();
}

bool QgsPluginRegistry::isPythonPlugin( const QString &key ) const
{
#ifdef WITH_BINDINGS
  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    if ( mPythonUtils->isPluginLoaded( key ) )
      return true;
  }
#endif
  return false;
}

void QgsPluginRegistry::addPlugin( const QString &key, const QgsPluginMetadata &metadata )
{
  mPlugins.insert( key, metadata );
}

void QgsPluginRegistry::dump()
{
  QgsDebugMsg( "PLUGINS IN REGISTRY: key -> (name, library)" );
  for ( QMap<QString, QgsPluginMetadata>::const_iterator it = mPlugins.constBegin();
        it != mPlugins.constEnd();
        ++it )
  {
    QgsDebugMsg( QString( "PLUGIN: %1 -> (%2, %3)" )
                 .arg( it.key(),
                       it->name(),
                       it->library() ) );
  }

#ifdef WITH_BINDINGS
  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    QgsDebugMsg( "PYTHON PLUGINS IN REGISTRY:" );
    Q_FOREACH ( const QString &pluginName, mPythonUtils->listActivePlugins() )
    {
      Q_UNUSED( pluginName );
      QgsDebugMsg( pluginName );
    }
  }
#endif
}


void QgsPluginRegistry::removePlugin( const QString &key )
{
  QgsDebugMsg( "removing plugin: " + key );
  QMap<QString, QgsPluginMetadata>::iterator it = mPlugins.find( key );
  if ( it != mPlugins.end() )
  {
    mPlugins.erase( it );
  }

  // python plugins are removed when unloaded
}

void QgsPluginRegistry::unloadAll()
{
  for ( QMap<QString, QgsPluginMetadata>::iterator it = mPlugins.begin();
        it != mPlugins.end();
        ++it )
  {
    if ( it->plugin() )
    {
      it->plugin()->unload();
    }
    else
    {
      QgsDebugMsg( "warning: plugin is NULL:" + it.key() );
    }
  }

#ifdef WITH_BINDINGS
  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    Q_FOREACH ( const QString &pluginName, mPythonUtils->listActivePlugins() )
    {
      mPythonUtils->unloadPlugin( pluginName );
    }
  }
#endif
}


bool QgsPluginRegistry::checkQgisVersion( const QString &minVersion, const QString &maxVersion ) const
{
  // Parse qgisMinVersion. Must be in form x.y.z or just x.y
  QStringList minVersionParts = minVersion.split( '.' );
  if ( minVersionParts.count() != 2 && minVersionParts.count() != 3 )
    return false;

  int minVerMajor, minVerMinor, minVerBugfix = 0;
  bool ok;
  minVerMajor = minVersionParts.at( 0 ).toInt( &ok );
  if ( !ok )
    return false;
  minVerMinor = minVersionParts.at( 1 ).toInt( &ok );
  if ( !ok )
    return false;
  if ( minVersionParts.count() == 3 )
  {
    minVerBugfix = minVersionParts.at( 2 ).toInt( &ok );
    if ( !ok )
      return false;
  }

  // Parse qgisMaxVersion. Must be in form x.y.z or just x.y
  int maxVerMajor, maxVerMinor, maxVerBugfix = 99;
  if ( maxVersion.isEmpty() || maxVersion == QLatin1String( "__error__" ) )
  {
    maxVerMajor = minVerMajor;
    maxVerMinor = 99;
  }
  else
  {
    QStringList maxVersionParts = maxVersion.split( '.' );
    if ( maxVersionParts.count() != 2 && maxVersionParts.count() != 3 )
      return false;

    bool ok;
    maxVerMajor = maxVersionParts.at( 0 ).toInt( &ok );
    if ( !ok )
      return false;
    maxVerMinor = maxVersionParts.at( 1 ).toInt( &ok );
    if ( !ok )
      return false;
    if ( maxVersionParts.count() == 3 )
    {
      maxVerBugfix = maxVersionParts.at( 2 ).toInt( &ok );
      if ( !ok )
        return false;
    }
  }

  // our qgis version - cut release name after version number
  QString qgisVersion = Qgis::QGIS_VERSION.section( '-', 0, 0 );

  QStringList qgisVersionParts = qgisVersion.split( '.' );

  int qgisMajor = qgisVersionParts.at( 0 ).toInt();
  int qgisMinor = qgisVersionParts.at( 1 ).toInt();
  int qgisBugfix = qgisVersionParts.at( 2 ).toInt();

  // build XxYyZz strings with trailing zeroes if needed
  QString minVer = QStringLiteral( "%1%2%3" ).arg( minVerMajor, 2, 10, QChar( '0' ) )
                   .arg( minVerMinor, 2, 10, QChar( '0' ) )
                   .arg( minVerBugfix, 2, 10, QChar( '0' ) );
  QString maxVer = QStringLiteral( "%1%2%3" ).arg( maxVerMajor, 2, 10, QChar( '0' ) )
                   .arg( maxVerMinor, 2, 10, QChar( '0' ) )
                   .arg( maxVerBugfix, 2, 10, QChar( '0' ) );
  QString curVer = QStringLiteral( "%1%2%3" ).arg( qgisMajor, 2, 10, QChar( '0' ) )
                   .arg( qgisMinor, 2, 10, QChar( '0' ) )
                   .arg( qgisBugfix, 2, 10, QChar( '0' ) );

  // compare
  return ( minVer <= curVer && maxVer >= curVer );
}


void QgsPluginRegistry::loadPythonPlugin( const QString &packageName )
{
#ifdef WITH_BINDINGS
  if ( !mPythonUtils || !mPythonUtils->isEnabled() )
  {
    QgsMessageLog::logMessage( QObject::tr( "Python is not enabled in QGIS." ), QObject::tr( "Plugins" ) );
    return;
  }

  QgsSettings settings;

  // is loaded already?
  if ( ! isLoaded( packageName ) )
  {
    // if plugin is not compatible, disable it
    if ( ! isPythonPluginCompatible( packageName ) )
    {
      QgsMessageLog::logMessage( QObject::tr( "Plugin \"%1\" is not compatible with this version of QGIS.\nIt will be disabled." ).arg( packageName ),
                                 QObject::tr( "Plugins" ) );
      settings.setValue( "/PythonPlugins/" + packageName, false );
      return;
    }

    mPythonUtils->loadPlugin( packageName );
    mPythonUtils->startPlugin( packageName );

    // TODO: test success

    QString pluginName = mPythonUtils->getPluginMetadata( packageName, QStringLiteral( "name" ) );

    // add to settings
    settings.setValue( "/PythonPlugins/" + packageName, true );
    QgsMessageLog::logMessage( QObject::tr( "Loaded %1 (package: %2)" ).arg( pluginName, packageName ), QObject::tr( "Plugins" ), QgsMessageLog::INFO );

    settings.remove( "/PythonPlugins/watchDog/" + packageName );
  }
#endif
}


void QgsPluginRegistry::loadCppPlugin( const QString &fullPathName )
{
  QgsSettings settings;

  QString baseName = QFileInfo( fullPathName ).baseName();

  // first check to see if its already loaded
  if ( isLoaded( baseName ) )
  {
    // plugin is loaded
    // QMessageBox::warning(this, "Already Loaded", description + " is already loaded");
    return;
  }

  QLibrary myLib( fullPathName );

  QString myError; //we will only show detailed diagnostics if something went wrong
  myError += QObject::tr( "Library name is %1\n" ).arg( myLib.fileName() );

  bool loaded = myLib.load();
  if ( !loaded )
  {
    QgsMessageLog::logMessage( QObject::tr( "Failed to load %1 (Reason: %2)" ).arg( myLib.fileName(), myLib.errorString() ), QObject::tr( "Plugins" ) );
    return;
  }

  myError += QObject::tr( "Attempting to resolve the classFactory function\n" );

  type_t *pType = ( type_t * ) cast_to_fptr( myLib.resolve( "type" ) );
  name_t *pName = ( name_t * ) cast_to_fptr( myLib.resolve( "name" ) );

  switch ( pType() )
  {
    case QgisPlugin::Renderer:
    case QgisPlugin::UI:
    {
      // UI only -- doesn't use mapcanvas
      create_ui *cf = ( create_ui * ) cast_to_fptr( myLib.resolve( "classFactory" ) );
      if ( cf )
      {
        QgisPlugin *pl = cf( mQgisInterface );
        if ( pl )
        {
          pl->initGui();
          // add it to the plugin registry
          addPlugin( baseName, QgsPluginMetadata( myLib.fileName(), pName(), pl ) );
          //add it to the qsettings file [ts]
          settings.setValue( "/Plugins/" + baseName, true );
          QgsMessageLog::logMessage( QObject::tr( "Loaded %1 (Path: %2)" ).arg( pName(), myLib.fileName() ), QObject::tr( "Plugins" ), QgsMessageLog::INFO );

          QObject *o = dynamic_cast<QObject *>( pl );
          if ( o )
          {
            QgsDebugMsg( QString( "plugin object name: %1" ).arg( o->objectName() ) );
            if ( o->objectName().isEmpty() )
            {
#ifndef Q_OS_WIN
              baseName = baseName.mid( 3 );
#endif
              QgsDebugMsg( QString( "object name to %1" ).arg( baseName ) );
              o->setObjectName( QStringLiteral( "qgis_plugin_%1" ).arg( baseName ) );
              QgsDebugMsg( QString( "plugin object name now: %1" ).arg( o->objectName() ) );
            }

            if ( !o->parent() )
            {
              QgsDebugMsg( QString( "setting plugin parent" ) );
              o->setParent( QgisApp::instance() );
            }
            else
            {
              QgsDebugMsg( QString( "plugin parent already set" ) );
            }
          }

          settings.remove( "/Plugins/watchDog/" + baseName );
        }
        else
        {
          // something went wrong
          QMessageBox::warning( mQgisInterface->mainWindow(), QObject::tr( "Error Loading Plugin" ),
                                QObject::tr( "There was an error loading a plugin. "
                                             "The following diagnostic information may help the QGIS developers resolve the issue:\n%1." )
                                .arg( myError ) );
          //disable it to the qsettings file [ts]
          settings.setValue( "/Plugins/" + baseName, false );
        }
      }
      else
      {
        QgsMessageLog::logMessage( QObject::tr( "Unable to find the class factory for %1." ).arg( fullPathName ), QObject::tr( "Plugins" ) );
      }

    }
    break;
    default:
      // type is unknown
      QgsMessageLog::logMessage( QObject::tr( "Plugin %1 did not return a valid type and cannot be loaded" ).arg( fullPathName ), QObject::tr( "Plugins" ) );
      break;
  }
}


void QgsPluginRegistry::unloadPythonPlugin( const QString &packageName )
{
#ifdef WITH_BINDINGS
  if ( !mPythonUtils || !mPythonUtils->isEnabled() )
  {
    QgsMessageLog::logMessage( QObject::tr( "Python is not enabled in QGIS." ), QObject::tr( "Plugins" ) );
    return;
  }

  if ( isLoaded( packageName ) )
  {
    mPythonUtils->unloadPlugin( packageName );
    QgsDebugMsg( "Python plugin successfully unloaded: " + packageName );
  }

  // disable the plugin no matter if successfully loaded or not
  QgsSettings settings;
  settings.setValue( "/PythonPlugins/" + packageName, false );
#endif
}


void QgsPluginRegistry::unloadCppPlugin( const QString &fullPathName )
{
  QgsSettings settings;
  QString baseName = QFileInfo( fullPathName ).baseName();
  settings.setValue( "/Plugins/" + baseName, false );
  if ( isLoaded( baseName ) )
  {
    QgisPlugin *pluginInstance = plugin( baseName );
    if ( pluginInstance )
    {
      pluginInstance->unload();
    }
    // remove the plugin from the registry
    removePlugin( baseName );
    QgsDebugMsg( "Cpp plugin successfully unloaded: " + baseName );
  }
}


//overloaded version of the next method that will load from multiple directories not just one
void QgsPluginRegistry::restoreSessionPlugins( const QStringList &pluginDirList )
{
  QStringListIterator myIterator( pluginDirList );
  while ( myIterator.hasNext() )
  {
    restoreSessionPlugins( myIterator.next() );
  }
}

void QgsPluginRegistry::restoreSessionPlugins( const QString &pluginDirString )
{
  QgsSettings mySettings;

#if defined(Q_OS_WIN) || defined(__CYGWIN__)
  QString pluginExt = "*.dll";
#elif ANDROID
  QString pluginExt = "*plugin.so";
#else
  QString pluginExt = QStringLiteral( "*.so*" );
#endif

  // check all libs in the current plugin directory and get name and descriptions
  QDir myPluginDir( pluginDirString, pluginExt, QDir::Name | QDir::IgnoreCase, QDir::Files | QDir::NoSymLinks );

  for ( uint i = 0; i < myPluginDir.count(); i++ )
  {
    QString myFullPath = pluginDirString + '/' + myPluginDir[i];
    if ( checkCppPlugin( myFullPath ) )
    {
      // check if the plugin was active on last session

      QString baseName = QFileInfo( myFullPath ).baseName();
      if ( mySettings.value( QStringLiteral( "Plugins/watchDog/%1" ).arg( baseName ) ).isValid() )
      {
        mQgisInterface->messageBar()->pushWarning( QObject::tr( "Plugin %1" ).arg( baseName ), QObject::tr( "The plugin will be disabled because it crashed QGIS during last startup. Please report an issue and re-enable the plugin when the problem has been solved." ) );
        mySettings.setValue( "/Plugins/" + baseName, false );
      }
      if ( mySettings.value( "/Plugins/" + baseName ).toBool() )
      {
        mySettings.setValue( QStringLiteral( "Plugins/watchDog/%1" ).arg( baseName ), true );
        loadCppPlugin( myFullPath );
        mySettings.remove( QStringLiteral( "/Plugins/watchDog/%1" ).arg( baseName ) );
      }
    }
  }

#ifdef WITH_BINDINGS
  if ( mPythonUtils && mPythonUtils->isEnabled() )
  {
    // check for python plugins system-wide
    QStringList pluginList = mPythonUtils->pluginList();
    QgsDebugMsg( "Loading python plugins" );

    QStringList corePlugins = QStringList();
    corePlugins << QStringLiteral( "GdalTools" );
    corePlugins << QStringLiteral( "db_manager" );
    corePlugins << QStringLiteral( "processing" );
    corePlugins << QStringLiteral( "MetaSearch" );

    // make the required core plugins enabled by default:
    Q_FOREACH ( const QString &corePlugin, corePlugins )
    {
      if ( !mySettings.contains( "/PythonPlugins/" + corePlugin ) )
      {
        mySettings.setValue( "/PythonPlugins/" + corePlugin, true );
      }
    }

    Q_FOREACH ( const QString &packageName, pluginList )
    {
      // TODO: apply better solution for #5879
      // start - temporary fix for issue #5879
      if ( QgsApplication::isRunningFromBuildDir() )
      {
        if ( corePlugins.contains( packageName ) )
        {
          QgsApplication::setPkgDataPath( QString() );
        }
        else
        {
          QgsApplication::setPkgDataPath( QgsApplication::buildSourcePath() );
        }
      }
      // end - temporary fix for issue #5879, more below


      if ( mySettings.value( "/PythonPlugins/watchDog/" + packageName ).isValid() )
      {
        mQgisInterface->messageBar()->pushWarning( QObject::tr( "Plugin %1" ).arg( packageName ), QObject::tr( "The plugin will be disabled because it crashed QGIS during last startup. Please report an issue and re-enable the plugin when the problem has been solved." ) );
        mySettings.setValue( "/PythonPlugins/" + packageName, false );
      }
      // check if the plugin was active on last session
      if ( mySettings.value( "/PythonPlugins/" + packageName ).toBool() )
      {
        mySettings.setValue( "/PythonPlugins/watchDog/" + packageName, true );
        if ( checkPythonPlugin( packageName ) )
        {
          loadPythonPlugin( packageName );
        }
        mySettings.remove( "/PythonPlugins/watchDog/" + packageName );

      }
    }
    // start - temporary fix for issue #5879, more above
    if ( QgsApplication::isRunningFromBuildDir() )
    {
      QgsApplication::setPkgDataPath( QgsApplication::buildSourcePath() );
    }
    // end - temporary fix for issue #5879
  }
#endif

  QgsDebugMsg( "Plugin loading completed" );
}


bool QgsPluginRegistry::checkCppPlugin( const QString &pluginFullPath )
{
  QLibrary myLib( pluginFullPath );
  bool loaded = myLib.load();
  if ( ! loaded )
  {
    QgsMessageLog::logMessage( QObject::tr( "Failed to load %1 (Reason: %2)" ).arg( myLib.fileName(), myLib.errorString() ), QObject::tr( "Plugins" ) );
    return false;
  }

  name_t *myName = ( name_t * ) cast_to_fptr( myLib.resolve( "name" ) );
  description_t   *myDescription = ( description_t * )  cast_to_fptr( myLib.resolve( "description" ) );
  category_t   *myCategory = ( category_t * )  cast_to_fptr( myLib.resolve( "category" ) );
  version_t   *myVersion = ( version_t * ) cast_to_fptr( myLib.resolve( "version" ) );

  if ( myName && myDescription && myVersion  && myCategory )
    return true;

  QgsDebugMsg( "Failed to get name, description, category or type for " + myLib.fileName() );
  return false;
}


bool QgsPluginRegistry::checkPythonPlugin( const QString &packageName )
{
#ifdef WITH_BINDINGS
  QString pluginName, description, /*category,*/ version;

  // get information from the plugin
  // if there are some problems, don't continue with metadata retreival
  pluginName  = mPythonUtils->getPluginMetadata( packageName, QStringLiteral( "name" ) );
  description = mPythonUtils->getPluginMetadata( packageName, QStringLiteral( "description" ) );
  version     = mPythonUtils->getPluginMetadata( packageName, QStringLiteral( "version" ) );
  // for Python plugins category still optional, by default used "Plugins" category
  //category = mPythonUtils->getPluginMetadata( packageName, "category" );

  if ( pluginName == QLatin1String( "__error__" ) || description == QLatin1String( "__error__" ) || version == QLatin1String( "__error__" ) )
  {
    QgsMessageLog::logMessage( QObject::tr( "Error when reading metadata of plugin %1" ).arg( packageName ),
                               QObject::tr( "Plugins" ) );
    return false;
  }

  return true;
#else
  return false;
#endif
}

bool QgsPluginRegistry::isPythonPluginCompatible( const QString &packageName ) const
{
#ifdef WITH_BINDINGS
  QString minVersion = mPythonUtils->getPluginMetadata( packageName, QStringLiteral( "qgisMinimumVersion" ) );
  // try to read qgisMaximumVersion. Note checkQgisVersion can cope with "__error__" value.
  QString maxVersion = mPythonUtils->getPluginMetadata( packageName, QStringLiteral( "qgisMaximumVersion" ) );
  return minVersion != QLatin1String( "__error__" ) && checkQgisVersion( minVersion, maxVersion );
#else
  return false;
#endif
}

QList<QgsPluginMetadata *> QgsPluginRegistry::pluginData()
{
  QList<QgsPluginMetadata *> resultList;
  QMap<QString, QgsPluginMetadata>::iterator it = mPlugins.begin();
  for ( ; it != mPlugins.end(); ++it )
  {
    resultList.push_back( &( it.value() ) );
  }
  return resultList;
}
