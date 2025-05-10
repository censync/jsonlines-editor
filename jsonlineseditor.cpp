#include "jsonlineseditor.h"
#include "ui_jsonlineseditor.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QTableWidgetItem>
#include <QCryptographicHash>
#include <QDesktopServices>
#include <QPushButton>
#include <QWidget>
#include <QLayout>

JsonLinesEditor::JsonLinesEditor(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::JsonLinesEditor)
{
    QCoreApplication::setApplicationName("JsonLinesEditor");
    QCoreApplication::setApplicationVersion(APP_VERSION);
    QCoreApplication::setOrganizationName("CenSync");
    QCoreApplication::setOrganizationDomain("censync.com");

    ui->setupUi(this);

    this->journalMessage(QString("Starting app: v%1").arg(APP_VERSION));

    this->setWindowTitle(QString("%1 v%2").arg(
                             QCoreApplication::applicationName(),
                             QCoreApplication::applicationVersion()));

    ui->centralwidget->setLayout(ui->verticalMainLayout);
    ui->tabsMainWidget->setParent(ui->centralwidget);

    ui->tabEditor->setLayout(ui->verticaEditorlLayout);
    ui->tabJournal->setLayout(ui->verticalLayoutJournal);
    ui->tabsMainWidget->setCurrentIndex(0);


    // connect(this, &JsonLinesEditor::newJournalMessage, this, &JsonLinesEditor::journalMessage);


    QObject::connect(this, &JsonLinesEditor::isFileChangedUpdated, [this](bool isFileChanged) {

        QAction* actionSave = findChild<QAction*>("actionSave");
        actionSave->setEnabled(isFileChanged);

        QAction* actionSaveAs = findChild<QAction*>("actionSaveAs");
        actionSaveAs->setEnabled(isFileChanged);

    });

    QObject::connect(this, &JsonLinesEditor::isItemChangedUpdated, [this](bool isItemChanged) {
        ui->toolButtonSaveItem->setEnabled(isItemChanged);
    });

    QObject::connect(this, &JsonLinesEditor::openedFileUpdated, [this](QString openedFile) {

        QAction* actionCloseFile = findChild<QAction*>("actionCloseFile");
        actionCloseFile->setEnabled(!openedFile.isEmpty());
        ui->toolButton_AddRow->setEnabled(!openedFile.isEmpty());


        this->openedFileChanged(openedFile);
    });



    if (!appCache->init())
    {
        QApplication::exit();
    }

    if (!this->initDataDirs()) {
        QApplication::exit();
    }

    ui->statusbar->showMessage(QString("Cache loaded: %1").arg(this->appCache->getCacheFilepath()));

    this->lastPath = this->appCache->getLastPath();


}

JsonLinesEditor::~JsonLinesEditor()
{
    delete this->appCache;
    delete ui;
}


void JsonLinesEditor::selectFileAndOpen() {
   QString fileName;

   if (this->lastPath.isEmpty()) {
       this->lastPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
   }

   fileName = QFileDialog::getOpenFileName(this, "Open file" , this->lastPath, "All files (*.*);;JSON Lines (*.jsonl);;JSON (*.json);;Text files (*.txt)");

   if (fileName.isEmpty()) {
       return;
   }

   QFileInfo fileInfo(fileName);
   this->lastPath = fileInfo.absoluteDir().path();
   this->appCache->setLastPath(this->lastPath);

   this->journalMessage(QString("Try to open file: %1").arg(fileName));

   this->loadEditableFile(fileName);
   return;
}

bool JsonLinesEditor::loadEditableFile(const QString &filePath)
{
    QFile jsonLinesFile(filePath);

    if (!jsonLinesFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this,
                              "Cannot open file",
                              QString("Cannot open file for edit. Error: %1").
                              arg(jsonLinesFile.errorString()),
                              QMessageBox::Ok);
        return false;
    }

    this->rowsInserted = 0;
    this->rowsUpdated = 0;

    ui->tableWidgetFile->setRowCount(0);
    ui->tableWidgetFile->resizeRowsToContents();
    ui->tableWidgetFile->updateGeometry();

    QTextStream in(&jsonLinesFile);
    in.setEncoding(QStringConverter::Utf8);

    int lineNumber = 0;

    while (!in.atEnd()) {
        lineNumber++;
          QString line = in.readLine().trimmed();


          if (line.isEmpty()) {
              continue;
          }

          QJsonParseError parseError;
          QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &parseError);

          if (parseError.error != QJsonParseError::NoError) {
              QString error = QString("Cannot parse file: on line %1. Error:%2. File: %3").arg(lineNumber).arg(parseError.errorString(), filePath);
              this->journalMessage(error);
              ui->statusbar->showMessage(error);

              jsonLinesFile.close();

              ui->tableWidgetFile->setRowCount(0);
              ui->tableWidgetFile->resizeRowsToContents();
              ui->tableWidgetFile->updateGeometry();

              QMessageBox::critical(this,
                                    "Cannot parse file",
                                    error + "\n\n" +line,
                                    QMessageBox::Abort);

              jsonLinesFile.close();

              return false;
          }

          if (!doc.isObject()) {
              QString error = QString("Cannot parse file: on line %1.Error: Not JSON object. File: %3").arg(lineNumber).arg(filePath);
              this->journalMessage(error);
              ui->statusbar->showMessage(error);

              jsonLinesFile.close();

              ui->tableWidgetFile->setRowCount(0);
              ui->tableWidgetFile->resizeRowsToContents();
              ui->tableWidgetFile->updateGeometry();

              QMessageBox::critical(this,
                                    "Cannot parse file",
                                    error + "\n\n" +line,
                                    QMessageBox::Abort);

              jsonLinesFile.close();

              return false;
          }

          QJsonObject entryTerm = doc.object();
          int rowCount = ui->tableWidgetFile->rowCount();
          ui->tableWidgetFile->insertRow(rowCount);

          QTableWidgetItem *itemTerm = new QTableWidgetItem();
          QTableWidgetItem *itemTermOrigin = new QTableWidgetItem();
          QTableWidgetItem *itemDefinition = new QTableWidgetItem();
          QTableWidgetItem *itemDefinitionOrigin = new QTableWidgetItem();
          QTableWidgetItem *itemSource = new QTableWidgetItem();

          itemTerm->setText(entryTerm["term"].toString());
          itemTermOrigin->setText(entryTerm["original_term"].toString());
          itemDefinition->setText(entryTerm["definition"].toString());
          itemDefinitionOrigin->setText(entryTerm["original_definition"].toString());
          itemSource->setText(entryTerm["source"].toString());

          ui->tableWidgetFile->setItem(rowCount, 0,itemTerm);
          ui->tableWidgetFile->setItem(rowCount, 1,itemTermOrigin);
          ui->tableWidgetFile->setItem(rowCount, 2,itemDefinition);
          ui->tableWidgetFile->setItem(rowCount, 3,itemDefinitionOrigin);
          ui->tableWidgetFile->setItem(rowCount, 4,itemSource);

          QCoreApplication::processEvents();

    }

    jsonLinesFile.close();

    this->setOpenedFile(filePath);

    return true;
}

void JsonLinesEditor::openedFileChanged(const QString &filePath)
{
    if (filePath.isEmpty()) {
        this->setWindowTitle(QString("%1 v%2 - file not selected").arg(
                                 QCoreApplication::applicationName(),
                                 QCoreApplication::applicationVersion()));

        ui->tableWidgetFile->setRowCount(0);
        ui->tableWidgetFile->resizeRowsToContents();

        ui->tableWidgetFile->resizeColumnToContents(0);
        ui->tableWidgetFile->resizeColumnToContents(1);
        ui->tableWidgetFile->horizontalHeader()->setStretchLastSection(true);


        this->disableEditor();
        ui->tableWidgetFile->setEnabled(false);
    } else {
        this->rowsInserted = 0;
        this->rowsUpdated = 0;

        this->setWindowTitle(QString("%1 v%2 - %3").arg(
                                 QCoreApplication::applicationName(),
                                 QCoreApplication::applicationVersion(),
                                 filePath));

        this->journalMessage(QString("Opened %1").arg(filePath));

        ui->tableWidgetFile->resizeColumnToContents(0);
        ui->tableWidgetFile->resizeColumnToContents(1);
        ui->tableWidgetFile->horizontalHeader()->setStretchLastSection(true);


        ui->tableWidgetFile->setEnabled(true);
        this->setIsFileChanged(false);
    }
}

bool JsonLinesEditor::checkForExit()
{
    if (this->isFileChanged() || this->isItemChanged()) {
        QMessageBox::StandardButton confirmExit  = QMessageBox::question(this,
                                            "Exit confirmation",
                                            "File contains unsaved changes. Do you want really save file whithout saving?",
                                            QMessageBox::No|QMessageBox::Yes);
        if (confirmExit == QMessageBox::No)
        {
            this->journalMessage(QString("Program exit without saving: %1").arg(this->openedFile()));
            return true;
        }

        this->journalMessage("Cancel exiting program without saving data");
        return false;
    }
    return true;
}

void JsonLinesEditor::checkForCloseFile()
{
    if (this->isFileChanged() || this->isItemChanged()) {
        QMessageBox::StandardButton confirmCloseFile  = QMessageBox::question(this,
                                            "Close file confirmation",
                                            "There are unsaved changes",
                                            QMessageBox::Save|QMessageBox::Cancel|QMessageBox::Close);
        if (confirmCloseFile == QMessageBox::Save)
        {
            this->saveFile();
        } else if (confirmCloseFile == QMessageBox::Cancel)  {
            this->journalMessage(QString("Cancel closing file: %1").arg(this->openedFile()));
            return;
        } else if (confirmCloseFile == QMessageBox::Close){
            this->journalMessage(QString("Close file without saving: %1").arg(this->openedFile()));
            this->setOpenedFile("");
           // close
        }
    } else {
        this->journalMessage(QString("Close file unchanged: %1").arg(this->openedFile()));
        this->setOpenedFile("");
    }

    // ui->tableWidgetFile->setRowCount(0);
    // ui->tableWidgetFile->resizeRowsToContents();

    // ui->tableWidgetFile->setEnabled(false);
    // this->disableEditor();
    this->setIsFileChanged(false);


    return;
}

void JsonLinesEditor::on_actionExit_triggered()
{
    QApplication::quit();
}


void JsonLinesEditor::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About",
                       QString("%1 by %2: v%3").arg(
                           QCoreApplication::applicationName(),
                           QCoreApplication::organizationName(),
                           QCoreApplication::applicationVersion()));
}


void JsonLinesEditor::on_actionClearCache_triggered()
{
    this->appCache->cleanCache();
    QApplication::quit();
}


void JsonLinesEditor::on_actionOpen_triggered()
{
    this->selectFileAndOpen();
}


void JsonLinesEditor::on_tableWidgetFile_itemSelectionChanged()
{
    QModelIndex index = ui->tableWidgetFile->selectionModel()->currentIndex();
    if(ui->tableWidgetFile->item(index.row(), 0)) {
        QList<QTableWidgetItem*> items = ui->tableWidgetFile->selectedItems();

        ui->lineEditTerm->setText(items.at(0)->text());
        ui->lineEditTermOrig->setText(items.at(1)->text());
        ui->plainTextDefinition->setPlainText(items.at(2)->text());
        ui->plainTextEditDefinitionOrig->setPlainText(items.at(3)->text());
        ui->lineEditSource->setText(items.at(4)->text());

        ui->toolButton_RemoveRow->setEnabled(true);
        this->enableEditor();
    } else {
        ui->toolButton_RemoveRow->setEnabled(false);
        this->disableEditor();
    }
}


void JsonLinesEditor::on_toolButton_TermSearchGoogle_clicked()
{
    if (!ui->lineEditTerm->text().trimmed().isEmpty()) {
        QDesktopServices::openUrl(QUrl(QString("https://google.com/search?q=\"%1\"&lr=lang_ru").arg(ui->lineEditTerm->text())));
    }
}

void JsonLinesEditor::on_toolButton_TermOrigSearchGooglech_clicked()
{
    if (!ui->lineEditTermOrig->text().trimmed().isEmpty()) {
        QDesktopServices::openUrl(QUrl(QString("https://google.com/search?q=\"%1\"&lr=lang_en").arg(ui->lineEditTerm->text())));

    }
}

bool JsonLinesEditor::checkItemChanged()
{
    QList<QTableWidgetItem*> items = ui->tableWidgetFile->selectedItems();
    if (items.length() != 0) {
        if (ui->lineEditTerm->text() != items.at(0)->text()) {
            return true;
        }
        if (ui->lineEditTermOrig->text() != items.at(1)->text()) {
            return true;
        }
        if (ui->plainTextDefinition->toPlainText() != items.at(2)->text()) {
            return true;
        }
        if (ui->plainTextEditDefinitionOrig->toPlainText() != items.at(3)->text()) {
            return true;
        }
        if (ui->lineEditSource->text() != items.at(4)->text()) {
            return true;
        }
    } else {
        // Insert
        return !ui->lineEditTerm->text().isEmpty() ||
               !ui->lineEditTermOrig->text().isEmpty() ||
               !ui->plainTextDefinition->toPlainText().isEmpty() ||
               !ui->plainTextEditDefinitionOrig->toPlainText().isEmpty() ||
               !ui->lineEditTermOrig->text().isEmpty();
    }


    return false;

}

void JsonLinesEditor::checkItemChanges()
{
    if (this->checkItemChanged()) {
        this->setIsItemChanged(true);
    } else {
        this->setIsItemChanged(false);
    }
}



void JsonLinesEditor::on_lineEditTerm_textChanged()
{
    this->checkItemChanges();
}


void JsonLinesEditor::on_lineEditTermOrig_textChanged()
{
    this->checkItemChanges();
}


void JsonLinesEditor::on_plainTextDefinition_textChanged()
{
    this->checkItemChanges();
}


void JsonLinesEditor::on_plainTextEditDefinitionOrig_textChanged()
{
    this->checkItemChanges();
}


void JsonLinesEditor::on_lineEditSource_textChanged()
{
    this->checkItemChanges();
}

void JsonLinesEditor::enableEditor() {
    ui->lineEditTerm->setEnabled(true);
    ui->lineEditTermOrig->setEnabled(true);
    ui->plainTextDefinition->setEnabled(true);
    ui->plainTextEditDefinitionOrig->setEnabled(true);
    ui->lineEditSource->setEnabled(true);

    ui->toolButton_TermSearchGoogle->setEnabled(true);
    ui->toolButton_TermOrigSearchGooglech->setEnabled(true);
}

void JsonLinesEditor::disableEditor() {
    ui->lineEditTerm->clear();
    ui->lineEditTermOrig->clear();
    ui->plainTextDefinition->clear();
    ui->plainTextEditDefinitionOrig->clear();
    ui->lineEditSource->clear();

    ui->lineEditTerm->setEnabled(false);
    ui->lineEditTermOrig->setEnabled(false);
    ui->plainTextDefinition->setEnabled(false);
    ui->plainTextEditDefinitionOrig->setEnabled(false);
    ui->lineEditSource->setEnabled(false);

    ui->toolButton_TermSearchGoogle->setEnabled(false);
    ui->toolButton_TermOrigSearchGooglech->setEnabled(false);
}


void JsonLinesEditor::on_toolButtonSaveItem_clicked()
{
    QString strTerm = ui->lineEditTerm->text().trimmed();
    QString strTermOrig = ui->lineEditTermOrig->text().trimmed();
    QString strDefinition = ui->plainTextDefinition->toPlainText().trimmed();
    QString strDefinitionOrig = ui->plainTextEditDefinitionOrig->toPlainText().trimmed();
    QString strSource = ui->lineEditSource->text().trimmed();


    if (strTerm.isEmpty()) {
        QMessageBox::warning(this,
                              "Cannot save item",
                              "Empty term",
                              QMessageBox::Ok);
        return;
    }

    if (strTermOrig.isEmpty()) {
        QMessageBox::warning(this,
                              "Cannot save item",
                              "Empty original term",
                              QMessageBox::Ok);
        return;
    }

    if (strDefinition.isEmpty()) {
        QMessageBox::warning(this,
                              "Cannot save item",
                              "Empty definition",
                              QMessageBox::Ok);
        return;
    }

    if (strDefinitionOrig.isEmpty()) {
        QMessageBox::warning(this,
                              "Cannot save item",
                              "Empty original definition",
                              QMessageBox::Ok);
        return;
    }

    if (strSource.isEmpty()) {
        QMessageBox::warning(this,
                              "Cannot save item",
                              "Empty source",
                              QMessageBox::Ok);
        return;
    }

    QList<QTableWidgetItem*> items = ui->tableWidgetFile->selectedItems();
    if (items.length() != 0) {
        items.at(0)->setText(strTerm);
        items.at(1)->setText(strTermOrig);
        items.at(2)->setText(strDefinition);
        items.at(3)->setText(strDefinitionOrig);
        items.at(4)->setText(strSource);

        this->rowsUpdated++;
        this->journalMessage(QString("Updated row: \"%1\" / \"%2\"").arg(strTerm, strTermOrig));
        ui->tableWidgetFile->scrollToItem(items.at(0));
    } else {
        // Insert
        int rowCount = ui->tableWidgetFile->rowCount();
        ui->tableWidgetFile->insertRow(rowCount);

        QTableWidgetItem *itemTerm = new QTableWidgetItem();
        QTableWidgetItem *itemTermOrigin = new QTableWidgetItem();
        QTableWidgetItem *itemDefinition = new QTableWidgetItem();
        QTableWidgetItem *itemDefinitionOrigin = new QTableWidgetItem();
        QTableWidgetItem *itemSource = new QTableWidgetItem();

        itemTerm->setText(strTerm);
        itemTermOrigin->setText(strTermOrig);
        itemDefinition->setText(strDefinition);
        itemDefinitionOrigin->setText(strDefinitionOrig);
        itemSource->setText(strSource);

        ui->tableWidgetFile->setItem(rowCount, 0,itemTerm);
        ui->tableWidgetFile->setItem(rowCount, 1,itemTermOrigin);
        ui->tableWidgetFile->setItem(rowCount, 2,itemDefinition);
        ui->tableWidgetFile->setItem(rowCount, 3,itemDefinitionOrigin);
        ui->tableWidgetFile->setItem(rowCount, 4,itemSource);

        this->rowsInserted++;
        this->journalMessage(QString("Insert row: \"%1\" / \"%2\"").arg(strTerm, strTermOrig));

        ui->tableWidgetFile->scrollToItem(itemTerm);
    }

    ui->tableWidgetFile->resizeColumnToContents(0);
    ui->tableWidgetFile->resizeColumnToContents(1);
    ui->tableWidgetFile->horizontalHeader()->setStretchLastSection(true);

    this->setIsFileChanged(true);

    this->setIsItemChanged(false);


    ui->toolButtonSaveItem->setEnabled(false);
}

bool JsonLinesEditor::saveFile(bool saveAs)
{
    if (ui->tableWidgetFile->rowCount() == 0) {
        QMessageBox::critical(this,
                              "Cannot save file",
                              "Nothing to save: data is empty",
                              QMessageBox::Ok);
        return false;
    }

    QString filePath = this->openedFile();

    if (saveAs || filePath.isEmpty() || filePath == defaultFileUnsaved) {
        filePath = QFileDialog::getSaveFileName(this, "Open file" , this->lastPath, "All files (*.*);;JSON Lines (*.jsonl);;JSON (*.json);;Text files (*.txt)");

        if (filePath.isEmpty()) {
            QMessageBox::critical(this,
                                  "Cannot save file",
                                  "File is not selected",
                                  QMessageBox::Ok);
            return false;
        }
    }
    QFileInfo fileInfo(filePath);

    if (fileInfo.exists()) {
        if (!fileInfo.isWritable()) {
            QMessageBox::critical(this,
                                  "Cannot save file",
                                  QString("File is not writable:\n%1").arg("%1", filePath),
                                  QMessageBox::Ok);
            return false;
        }

        if (!this->createFileBackup(filePath))
        {
            QMessageBox::warning(this,
                                  "Cannot create backup for file",
                                  QString("Cannot create backup for file"),
                                  QMessageBox::Ok);
        }
    }

    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this,
                              "Cannot save file",
                              QString("Path is not writable:\n%1").arg("%1", filePath),
                              QMessageBox::Ok);
        return false;
    }


    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    int rows = ui->tableWidgetFile->rowCount();

    for (int row = 0; row < rows; row++) {
        QJsonObject obj;

        QString strTerm = ui->tableWidgetFile->item(row, 0)->text().trimmed();
        QString strTermOrig = ui->tableWidgetFile->item(row, 1)->text().trimmed();
        QString strDefinition = ui->tableWidgetFile->item(row, 2)->text().trimmed();
        QString strDefinitionOrig = ui->tableWidgetFile->item(row, 3)->text().trimmed();
        QString strSource = ui->tableWidgetFile->item(row, 4)->text().trimmed();


        // Skip empty
        if (strTerm.isEmpty() &&
                strTermOrig.isEmpty() &&
                strDefinition.isEmpty() &&
                strDefinitionOrig.isEmpty() &&
                strSource.isEmpty()) {
            continue;
        }

        obj.insert("term", strTerm);
        obj.insert("original_term", strTermOrig);
        obj.insert("definition", strDefinition);
        obj.insert("original_definition", strDefinitionOrig);
        obj.insert("source", strSource);

        QJsonDocument doc(obj);

        out << doc.toJson(QJsonDocument::Compact) << "\n";

    }
    file.close();

    this->journalMessage(QString("Saved file: %1 inserts: %2 updates: %3").
                         arg(filePath).
                         arg(this->rowsInserted).
                         arg(this->rowsUpdated));

    this->setIsFileChanged(false);
    this->setOpenedFile(filePath);


    return true;
}

bool JsonLinesEditor::createFileBackup(const QString &filePath)
{
    QFileInfo fileInfo(filePath);

    if (!fileInfo.exists()) {
        this->journalMessage(QString("Cannot create file backup. File not exist: %1").arg(filePath));
        return false;
    }


    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();
    QString dirPath = fileInfo.absolutePath();
    QString backupBaseName = baseName + "." + suffix + ".bak";

    int counter = 1;
    QString backupPath = QDir(this->appCache->getCacheDir() + "/backups/").filePath(backupBaseName);

    while (QFileInfo::exists(backupPath)) {
        backupPath = QDir(this->appCache->getCacheDir() + "/backups/").filePath(
            QString("%1.%2.bak.%3").arg(baseName).arg(suffix).arg(counter++));
    }


    if (!QFile::copy(filePath, backupPath)) {
        this->journalMessage(QString("Failed to create backup for: %1, backup path: %2").arg(filePath, backupPath));
        return false;
    }

    this->journalMessage(QString("Backup saved: %1").arg(backupPath));

    return true;
}

void JsonLinesEditor::on_actionSave_triggered()
{
    this->saveFile();
}


void JsonLinesEditor::on_actionCloseFile_triggered()
{
    this->checkForCloseFile();
}

void JsonLinesEditor::journalMessage(const QString &message)
{
    QString tm= QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    ui->plainTextJournal->appendPlainText(tm + message);

    QTextCursor cursor = ui->plainTextJournal->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->plainTextJournal->setTextCursor(cursor);
}

void JsonLinesEditor::saveDailyJournal()
{
    QString fileName = this->appCache->getCacheDir() + "/logs/" +(QDateTime::currentDateTime().toString("yyyy-MM-dd") + ".log");

    QFile file(fileName);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);

        QString tm = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
        stream << QString("\n======================%1======================\n").arg(tm);

        stream << ui->plainTextJournal->toPlainText() << "\n";

        file.close();
    } else {
        QMessageBox::critical(this,
                              "Cannot save log file",
                              QString("Cannot save log file: %1").arg(file.errorString()),
                              QMessageBox::Ok);
    }
}

bool JsonLinesEditor::initDataDirs()
{
    QString appCacheDir = this->appCache->getCacheDir();

    if (!QDir(appCacheDir).exists()) {
        if (!QDir().mkpath(appCacheDir)) {
            QMessageBox::critical(this,
                                  "Cannot create directory",
                                  QString("Cannot create app cache directory:\n%1").arg("%1", appCacheDir),
                                  QMessageBox::Abort);
            return false;
        }
    }

    if (!QDir(appCacheDir+"/logs/").exists()) {
        if (!QDir().mkpath(appCacheDir+"/logs/")) {
            QMessageBox::critical(this,
                                  "Cannot create directory",
                                  QString("Cannot create app logs directory:\n%1").arg("%1", appCacheDir+"/logs/"),
                                  QMessageBox::Abort);
            return false;
        }
    }

    if (!QDir(appCacheDir+"/backups/").exists()) {
        if (!QDir().mkpath(appCacheDir+"/backups/")) {
            QMessageBox::critical(this,
                                  "Cannot create directory",
                                  QString("Cannot create app backups directory:\n%1").arg("%1", appCacheDir+"/backups/"),
                                  QMessageBox::Abort);
            return false;
        }
    }

    return true;
}

void JsonLinesEditor::on_actionSaveAs_triggered()
{
    this->saveFile(true);
}


void JsonLinesEditor::on_actionCreate_triggered()
{
    this->setOpenedFile(defaultFileUnsaved);
}


void JsonLinesEditor::on_toolButton_AddRow_clicked()
{
    int rowCount = ui->tableWidgetFile->rowCount();
    ui->tableWidgetFile->insertRow(rowCount);

    QTableWidgetItem *itemTerm = new QTableWidgetItem();
    QTableWidgetItem *itemTermOrigin = new QTableWidgetItem();
    QTableWidgetItem *itemDefinition = new QTableWidgetItem();
    QTableWidgetItem *itemDefinitionOrigin = new QTableWidgetItem();
    QTableWidgetItem *itemSource = new QTableWidgetItem();


    ui->tableWidgetFile->setItem(rowCount, 0,itemTerm);
    ui->tableWidgetFile->setItem(rowCount, 1,itemTermOrigin);
    ui->tableWidgetFile->setItem(rowCount, 2,itemDefinition);
    ui->tableWidgetFile->setItem(rowCount, 3,itemDefinitionOrigin);
    ui->tableWidgetFile->setItem(rowCount, 4,itemSource);
    this->journalMessage(QString("Added row"));
    ui->tableWidgetFile->scrollToItem(itemTerm);

}



void JsonLinesEditor::on_toolButton_RemoveRow_clicked()
{
    QModelIndex index = ui->tableWidgetFile->selectionModel()->currentIndex();
    if(ui->tableWidgetFile->item(index.row(), 0)) {
        this->journalMessage(QString("Removed row: %1").arg(ui->tableWidgetFile->item(index.row(), 0)->text()));
        ui->tableWidgetFile->removeRow(index.row());
        this->setIsFileChanged(true);
    }
}

