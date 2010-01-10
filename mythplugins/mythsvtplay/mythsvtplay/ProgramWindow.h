#ifndef PROGRAMWINDOW_H
#define PROGRAMWINDOW_H

#include <mythscreentype.h>

#include "MediaPlayer.h"
#include "ImageLoader.h"

class MythUIButtonTree;
class MythUIButtonListItem;
class MythUIBusyDialog;
class MythUIProgressDialog;
class MythUIImage;
class MythUIText;

class Program;

class ProgramWindow : public MythScreenType
{
    Q_OBJECT

public:
    ProgramWindow(MythScreenStack *parentStack,
                  Program* program);
    ~ProgramWindow();

public slots:
    void onEpisodeSelected(MythUIButtonListItem *item);
    void onEpisodeClicked(MythUIButtonListItem *item);

    void onCacheFilledPercentChange(int);
    void onCacheFilled();
    void onFinishedPlayback();

private:
    void populateEpisodeList();

    MythUIButtonTree* episodeList_;

    MythUIImage* programLogoImage_;
    MythUIImage* episodePreviewImage_;

    MythUIText* programTitleText_;
    MythUIText* programDescriptionText_;

    MythUIText* episodeTitleText_;
    MythUIText* episodeDescriptionText_;

    MythUIText* episodePublishedDateText_;
    MythUIText* episodeAvailableToDateText_;

    MythUIBusyDialog* busyDialog_;
    MythUIProgressDialog* progressDialog_;

    Program* program_;

    MediaPlayer mediaPlayer_;
    ImageLoader imageLoader_;

};

#endif // PROGRAMWINDOW_H
