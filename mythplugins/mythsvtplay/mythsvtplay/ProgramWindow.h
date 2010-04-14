#ifndef PROGRAMWINDOW_H
#define PROGRAMWINDOW_H

#include <mythscreentype.h>

#include "MediaPlayer.h"
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

    void onImageReady(MythUIImage*);

private:
    void populateEpisodeList();

    Program* program_;
    QMap<QString, EpisodeListBuilder*> episodeBuilders_;

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

    MythUIBusyDialog* busyDialog_;
    MythConfirmationDialog* noStreamFoundDialog_;
    ProgressDialog* progressDialog_;

    MediaPlayer mediaPlayer_;
    ImageLoader imageLoader_;

    QString selectedEpisodeType_;
};

#endif // PROGRAMWINDOW_H
