#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <mythscreentype.h>

#include "ProgressDialog.h"
#include "FavoritesStore.h"

class MythGenericTree;
class MythUIButtonTree;
class MythUIBusyDialog;
class MythConfirmationDialog;

class ProgramListCache;
class EpisodeListBuilder;
class Program;

class MainWindow : public MythScreenType
{
    Q_OBJECT

public:
    MainWindow(MythScreenStack *parentStack);
    ~MainWindow();

    void beginProgramDownload(bool refreshCache = false);

    bool keyPressEvent(QKeyEvent *event);

public slots:
    void populateTree();

    void onListButtonClicked(MythUIButtonListItem *item);
    void onListButtonSelected(MythUIButtonListItem *item);
    void onImageReady(MythUIImage*);

    void abortProgramsDownload();
    void onNumberOfProgramsFound(int count);
    void onNumberOfProgramsComplete(int count);
    void onProgramComplete(const QString& title);

    void onRefreshDialogResult(bool);

private:
    MainWindow();

    MythGenericTree* createAlphabeticTree(const QList<Program*>& programs);
    MythGenericTree* createCategoryTree(const QList<Program*>& programs);
    MythGenericTree* createFavoritesTree(const QList<Program*>& programs);

    MythUIButtonTree* programTree_;
    MythGenericTree* programTreeData_;

    MythUIImage* programLogoImage_;
    MythUIText* programTitleText_;
    MythUIText* programDescriptionText_;

    QList<Program*> programList_;

    MythUIBusyDialog* busyDialog_;
    ProgressDialog* progressDialog_;
    MythConfirmationDialog* confirmCacheRefreshDialog_;

    ProgramListCache* programListCache_;

    FavoritesStore favoritesStore_;
};

#endif // MAINWINDOW_H
