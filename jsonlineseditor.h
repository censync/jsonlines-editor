#ifndef JSONLINESEDITOR_H
#define JSONLINESEDITOR_H

#include <QMainWindow>
#include "core/appcache.h"
#include <QCloseEvent>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui { class JsonLinesEditor; }
QT_END_NAMESPACE

class JsonLinesEditor : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(bool isFileChanged READ isFileChanged WRITE setIsFileChanged NOTIFY isFileChangedUpdated)
    Q_PROPERTY(bool isItemChanged READ isItemChanged WRITE setIsItemChanged NOTIFY isItemChangedUpdated)
    Q_PROPERTY(QString openedFile READ openedFile WRITE setOpenedFile NOTIFY openedFileUpdated)


public:
    JsonLinesEditor(QWidget *parent = nullptr);
    ~JsonLinesEditor();

protected:
    void closeEvent(QCloseEvent *event) override {
        if (checkForExit()) {
            saveDailyJournal();
            event->accept();
        } else {
            event->ignore();
        }
    }

private slots:
    void on_actionExit_triggered();

    bool checkForExit();
    void checkForCloseFile();
    void selectFileAndOpen();
    bool loadEditableFile(const QString &filePath);
    void journalMessage(const QString& message);
    void saveDailyJournal();
    void openedFileChanged(const QString &filePath);

    void on_actionAbout_triggered();

    void on_actionClearCache_triggered();

    void on_actionOpen_triggered();

    void on_tableWidgetFile_itemSelectionChanged();

    void on_toolButton_TermSearchGoogle_clicked();

    void on_toolButton_TermOrigSearchGooglech_clicked();

    void on_lineEditTerm_textChanged();

    void on_lineEditTermOrig_textChanged();

    void on_plainTextDefinition_textChanged();

    void on_plainTextEditDefinitionOrig_textChanged();

    void on_lineEditSource_textChanged();

    void on_toolButtonSaveItem_clicked();

    void on_actionSave_triggered();

    void on_actionCloseFile_triggered();

    void on_actionSaveAs_triggered();

    void on_actionCreate_triggered();

signals:
    void isFileChangedUpdated(bool);
    void isItemChangedUpdated(bool);
    void openedFileUpdated(QString);
//    void newJournalMessage(const QString& message);

private:
    Ui::JsonLinesEditor *ui;
    const QString defaultFileUnsaved = "unsaved";
    AppCache *appCache = new AppCache();
    int rowsUpdated = 0;
    int rowsInserted = 0;
    QString lastPath = "";


    bool isFileChangedVal = false;
    bool isItemChangedVal = false;
    QString openedFileVal = "";

    bool isFileChanged() const { return isFileChangedVal; }
    void setIsFileChanged(bool val) {
        if (isFileChangedVal != val) {
            isFileChangedVal = val;
            emit isFileChangedUpdated(val);
        }
    }

    bool isItemChanged() const { return isItemChangedVal; }
    void setIsItemChanged(bool val) {
        if (isItemChangedVal != val) {
            isItemChangedVal = val;
            emit isItemChangedUpdated(val);
        }
    }

    QString openedFile() const { return openedFileVal; }
    void setOpenedFile(QString val) {
        if (openedFileVal != val) {
            openedFileVal = val;
            emit openedFileUpdated(val);
        }
    }

    bool initDataDirs();
    bool checkItemChanged();
    void checkItemChanges();
    void enableEditor();
    void disableEditor();
    bool saveFile(bool saveAs = false);
    bool createFileBackup(const QString &filePath);
};
#endif // JSONLINESEDITOR_H
