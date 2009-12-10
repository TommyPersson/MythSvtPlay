#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <mythscreentype.h>

class MythGenericTree;
class MythUIButtonTree;
class MythUIBusyDialog;

class ShowTreeBuilder;

class MainWindow : public MythScreenType
{
    Q_OBJECT

public:
    MainWindow(MythScreenStack *parentStack);
    ~MainWindow();
    bool Create();

public slots:
    void populateTree(MythGenericTree*);
    void onListButtonClicked(MythUIButtonListItem *item);

private:
    MainWindow();

    MythUIButtonTree* programTree_;
    MythUIBusyDialog* busyDialog_;

    ShowTreeBuilder* treeBuilder_;
};

#endif // MAINWINDOW_H
