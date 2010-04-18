#ifndef PROGRAMWINDOW_H
#define PROGRAMWINDOW_H

#include <mythscreentype.h>

#include "ImageLoader.h"

class MythUIButtonTree;
class MythUIButtonListItem;
class MythUIBusyDialog;
class MythUIProgressDialog;
class MythConfirmationDialog;
class MythUIImage;
class MythUIText;

class ProgressDialog;
class Program;
class EpisodeListBuilder;
class IMediaPlayer;
class Episode;

class ProgramWindow : public MythScreenType
{
    Q_OBJECT

public:
    ProgramWindow(MythScreenStack *parentStack,
                  Program* program);
    ~ProgramWindow();

    bool keyPressEvent(QKeyEvent *event);

public slots:
    void onEpisodeSelected(MythUIButtonListItem*);
    void onEpisodeClicked(MythUIButtonListItem*);

    void onEpisodeTypeSelected(MythUIButtonListItem*);

    void onEpisodesReady(const QString& episodeType);

    void onCancelClicked();

    void onCacheFilledPercentChange(int);
    void onCacheFilled();
    void onMediaPlayerDestroyed();
    void onConnectionFailed();

    void onImageReady(MythUIImage*);

private:
    void populateEpisodeList();

    void setupMediaPlayer(Episode*);
    void stopMediaPlayer();

    Program* program_;
    QMap<QString, EpisodeListBuilder*> episodeBuilders_;

    int savedListPosition_;

    QString selectedEpisodeType_;

    MythUIButtonList* episodeList_;
    MythUIButtonList* episodeTypeList_;

    MythUIImage* programLogoImage_;
    MythUIImage* episodePreviewImage_;

    MythUIText* programTitleText_;
    MythUIText* programDescriptionText_;

    MythUIText* episodeTitleText_;
    MythUIText* episodeDescriptionText_;

    MythUIText* episodePublishedDateText_;
    MythUIText* episodeAvailableToDateText_;

    MythConfirmationDialog* noStreamFoundDialog_;
    ProgressDialog* progressDialog_;

    IMediaPlayer* mediaPlayer_;
    ImageLoader imageLoader_;
};

#endif // PROGRAMWINDOW_H
