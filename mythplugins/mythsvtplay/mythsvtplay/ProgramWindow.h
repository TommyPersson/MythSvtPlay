#ifndef PROGRAMWINDOW_H
#define PROGRAMWINDOW_H

#include <mythscreentype.h>

#include "Program.h"
#include "MediaPlayer.h"

class MythUIButtonTree;
class MythUIButtonListItem;
class MythUIBusyDialog;

class ProgramWindow : public MythScreenType
{
    Q_OBJECT

public:
    ProgramWindow(MythScreenStack *parentStack,
               const Program& program);
    ~ProgramWindow();

public slots:
    void onEpisodeSelected(MythUIButtonListItem *item);
    void onFinishedPlayback();

private:
    void populateEpisodeList();

    MythUIButtonTree* episodeList_;
    MythUIBusyDialog* busyDialog_;

    Program program_;
    MediaPlayer mediaPlayer_;

};

#endif // PROGRAMWINDOW_H
