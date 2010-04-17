#include "ProgramWindow.h"

#include "Program.h"
#include "ProgressDialog.h"
#include "EpisodeListBuilder.h"

#include <iostream>

#include <QCoreApplication>

#include <mythmainwindow.h>
#include <mythdialogbox.h>

#include <mythtv/mythcontext.h>

#include <mythtv/libmythui/mythsystem.h>
#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythprogressdialog.h>
#include <mythtv/libmythui/mythuibuttontree.h>

ProgramWindow::ProgramWindow(MythScreenStack *parentStack, Program* program)
    : MythScreenType(parentStack, "ProgramWindow"),
      program_(program),
      savedListPosition_(0),
      progressDialog_(NULL),
      mediaPlayer_(NULL)
{
    if (!LoadWindowFromXML("svtplay-ui.xml", "program-view", this))
    {
        throw "Could not load svtplay-ui.xml";
    }

    bool err = false;
    UIUtilE::Assign(this, episodeList_, "episode-list", &err);
    UIUtilE::Assign(this, episodeTypeList_, "episode-type-list", &err);

    UIUtilE::Assign(this, episodeTitleText_, "episode-title", &err);
    UIUtilE::Assign(this, episodeDescriptionText_, "episode-description", &err);
    UIUtilE::Assign(this, episodeAvailableToDateText_, "episode-available-to-date", &err);
    UIUtilE::Assign(this, episodePreviewImage_, "episode-preview-image", &err);

    UIUtilE::Assign(this, programTitleText_, "program-title", &err);
    UIUtilE::Assign(this, programDescriptionText_, "program-description", &err);
    UIUtilE::Assign(this, programLogoImage_, "program-logo-image", &err);

    QObject::connect(episodeList_, SIGNAL(itemClicked(MythUIButtonListItem*)),
                     this, SLOT(onEpisodeClicked(MythUIButtonListItem*)));
    QObject::connect(episodeList_, SIGNAL(itemSelected(MythUIButtonListItem*)),
                     this, SLOT(onEpisodeSelected(MythUIButtonListItem*)));

    QObject::connect(episodeTypeList_, SIGNAL(itemSelected(MythUIButtonListItem*)),
                     this, SLOT(onEpisodeTypeSelected(MythUIButtonListItem*)));

    QObject::connect(&imageLoader_, SIGNAL(imageReady(MythUIImage*)),
                     this, SLOT(onImageReady(MythUIImage*)));

    programTitleText_->SetText(program->title);
    programDescriptionText_->SetText(program->description);

    programLogoImage_->SetFilename(program->logoFilepath);

    imageLoader_.loadImage(programLogoImage_);
    imageLoader_.start();

    episodeTypeList_->Reset();

    for (int i = 0; i < program_->episodeTypeLinks.size(); ++i)
    {
        MythUIButtonListItem* item = new MythUIButtonListItem(
                episodeTypeList_,
                program_->episodeTypeLinks.at(i).first);
    }

    episodeBuilders_.clear();

    for (int i = 0; i < program->episodeTypeLinks.size(); ++i)
    {
        QString link = program->link + program->episodeTypeLinks.at(i).second;
        EpisodeListBuilder* builder = new EpisodeListBuilder(
                                            program->episodeTypeLinks.at(i).first,
                                            QUrl("http://svtplay.se" + link));

        episodeBuilders_[program->episodeTypeLinks.at(i).first] = builder;

        QObject::connect(builder, SIGNAL(episodesReady(QString)),
                         this, SLOT(onEpisodesReady(QString)));

        builder->buildEpisodeList();
    }

    SetFocusWidget(episodeTypeList_);
}

ProgramWindow::~ProgramWindow()
{
    for (int i = 0; i < episodeBuilders_.values().count(); ++i)
    {
        episodeBuilders_.values().at(i)->abort();
        episodeBuilders_.values().at(i)->deleteLater();
    }
    episodeBuilders_.clear();
    program_->episodesByType.clear();

    imageLoader_.terminate();
    imageLoader_.wait();
}

void ProgramWindow::onEpisodesReady(const QString& episodeType)
{
    QList<Episode*> episodes = episodeBuilders_[episodeType]->episodeList();

    program_->episodesByType[episodeType] = episodes;

    populateEpisodeList();
}

void ProgramWindow::populateEpisodeList()
{
    episodeList_->Reset();

    QList<Episode*> episodes = program_->episodesByType[selectedEpisodeType_];

    for (int i = 0; i < episodes.size(); ++i)
    {
        MythUIButtonListItem* item = new MythUIButtonListItem(
                episodeList_,
                episodes.at(i)->title,
                QVariant::fromValue(episodes.at(i)));
    }

    if (episodes.size() > 0)
    {
        if (episodeBuilders_[selectedEpisodeType_]->moreEpisodesAvailable())
        {
            MythUIButtonListItem* item = new MythUIButtonListItem(
                    episodeList_,
                    QString::fromUtf8("Hämta fler ..."),
                    QVariant(selectedEpisodeType_));
        }

        episodeList_->SetItemCurrent(savedListPosition_);
    }
    else if (episodeBuilders_[selectedEpisodeType_] != NULL)
    {
        if (episodeBuilders_[selectedEpisodeType_]->isBusy())
        {
            MythUIButtonListItem* item = new MythUIButtonListItem(
                    episodeList_,
                    QString::fromUtf8("Hämtar ..."));
        }
    }
}

void ProgramWindow::setupMediaPlayer(Episode* episode)
{
    mediaPlayer_ = new MediaPlayer();

    QObject::connect(mediaPlayer_, SIGNAL(cacheFilledPercent(int)),
                     this, SLOT(onCacheFilledPercentChange(int)));
    QObject::connect(mediaPlayer_, SIGNAL(cacheFilled()),
                     this, SLOT(onCacheFilled()));
    QObject::connect(mediaPlayer_, SIGNAL(destroyed()),
                     this, SLOT(onMediaPlayerDestroyed()));

    mediaPlayer_->loadEpisode(episode);
}

void ProgramWindow::stopMediaPlayer()
{
    if (mediaPlayer_ != NULL)
    {
        mediaPlayer_->quit();
        mediaPlayer_ = NULL;
    }
}

void ProgramWindow::onEpisodeClicked(MythUIButtonListItem* item)
{
    if (item->GetText() == QString::fromUtf8("Hämta fler ..."))
    {
        episodeBuilders_[selectedEpisodeType_]->buildEpisodeList();

        savedListPosition_ = episodeList_->GetCurrentPos();

        item->SetText(QString::fromUtf8("Hämtar ..."));

        return;
    }
    else if (item->GetText() == QString::fromUtf8("Hämtar ..."))
        return;

    QVariant itemData = item->GetData();

    Episode* episode = itemData.value<Episode*>();

    if (episode->mediaUrl.isEmpty())
    {
        noStreamFoundDialog_ = new MythConfirmationDialog(GetScreenStack(), "Sorry, no suitable stream found.", false);
        noStreamFoundDialog_->Create();
        GetScreenStack()->AddScreen(noStreamFoundDialog_);
        return;
    }

    progressDialog_ = new ProgressDialog(GetScreenStack(), "cache-dialog", "Filling stream buffer ...", "0%");

    QObject::connect(progressDialog_, SIGNAL(cancelClicked()),
                     this, SLOT(onCancelClicked()));

    GetScreenStack()->AddScreen(progressDialog_);

    setupMediaPlayer(episode);
}

void ProgramWindow::onEpisodeSelected(MythUIButtonListItem* item)
{
    if (item->GetText() == QString::fromUtf8("Hämta fler ...") ||
        item->GetText() == QString::fromUtf8("Hämtar ..."))
    {
        episodePreviewImage_->Reset();
        episodeDescriptionText_->SetText("");
        episodeTitleText_->SetText("");
        episodeAvailableToDateText_->SetText("");

        Refresh();

        return;
    }

    QVariant itemData = item->GetData();

    Episode* episode = itemData.value<Episode*>();

    episodePreviewImage_->SetFilename(episode->episodeImageFilepath);
    imageLoader_.loadImage(episodePreviewImage_);

    episodeDescriptionText_->SetText(episode->description);
    episodeTitleText_->SetText(episode->title);
    episodeAvailableToDateText_->SetText(episode->availableUntilDate);

    Refresh();
}

void ProgramWindow::onEpisodeTypeSelected(MythUIButtonListItem* item)
{
    selectedEpisodeType_ = item->GetText();

    populateEpisodeList();
}

void ProgramWindow::onCancelClicked()
{
    if (progressDialog_)
    {
        progressDialog_->Close();
        progressDialog_ = NULL;
    }

    stopMediaPlayer();
}

void ProgramWindow::onImageReady(MythUIImage* image)
{
    image->Load();
}

void ProgramWindow::onCacheFilledPercentChange(int percent)
{
    if (progressDialog_)
    {
        progressDialog_->setProgress(percent);
        progressDialog_->setStatusText(QString::number(percent) + "%");
    }
}

void ProgramWindow::onCacheFilled()
{
    if (progressDialog_)
    {
        progressDialog_->Close();
        progressDialog_ = NULL;
    }
}

void ProgramWindow::onMediaPlayerDestroyed()
{
    mediaPlayer_ = NULL;
}

bool ProgramWindow::keyPressEvent(QKeyEvent *event)
{
    if (mediaPlayer_ != NULL && mediaPlayer_->isRunning())
        return true;

    if (GetFocusWidget() && GetFocusWidget()->keyPressEvent(event))
        return true;

    bool handled = false;
    QStringList actions;
    handled = GetMythMainWindow()->TranslateKeyPress("ProgramWindow", event, actions);

    for (int i = 0; i < actions.size() && !handled; i++)
    {
        QString action = actions[i];
        handled = true;

        if (action == "ESCAPE")
        {
            // Hopefully temporary fix to prevent crash when closing
            // the window precisely when signals are recieved in the downloader.
            bool allowEscape = true;
            for (int j = 0; j < episodeBuilders_.values().size(); ++j)
            {
                if (episodeBuilders_.values().at(j)->isBusy())
                {
                    allowEscape = false;
                }
            }

            if (allowEscape)
            {
                if (progressDialog_ != NULL && progressDialog_->IsVisible())
                {
                    stopMediaPlayer();
                    progressDialog_->Close();
                    progressDialog_ = NULL;
                }
                else
                {
                    Close();
                }
            }
        }
        else if (action == "UP")
        {
            savedListPosition_ = 0;
            SetFocusWidget(episodeTypeList_);
        }
        else if (action == "DOWN")
        {
            SetFocusWidget(episodeList_);
        }
        else if (action == "LEFT")
        {
            SetFocusWidget(episodeTypeList_);
        }
        else if (action == "RIGHT")
        {
            SetFocusWidget(episodeTypeList_);
        }
        else
            handled = false;
    }

    if (!handled && MythScreenType::keyPressEvent(event))
        handled = true;

    return handled;
}
