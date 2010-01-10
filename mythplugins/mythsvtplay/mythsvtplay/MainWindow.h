#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <mythscreentype.h>

class MythGenericTree;
class MythUIButtonTree;
class MythUIBusyDialog;

class ShowTreeBuilder;
class EpisodeListBuilder;
class Program;


class MainWindow : public MythScreenType
{
    Q_OBJECT

public:
    MainWindow(MythScreenStack *parentStack);
    ~MainWindow();

public slots:
    void populateTree(MythGenericTree*);
    void onListButtonClicked(MythUIButtonListItem *item);
    void onReceiveEpisodes(Program*);
    void onNoEpisodesReceived();

private:
    MainWindow();

    MythUIButtonTree* programTree_;
    MythUIBusyDialog* busyDialog_;

    ShowTreeBuilder* treeBuilder_;
    EpisodeListBuilder* episodeListBuilder_;
};

#endif // MAINWINDOW_H
