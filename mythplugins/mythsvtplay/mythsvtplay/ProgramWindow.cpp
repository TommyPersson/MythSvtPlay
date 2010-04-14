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
      progressDialog_(NULL)
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

    QObject::connect(&mediaPlayer_, SIGNAL(cacheFilledPercent(int)),
                     this, SLOT(onCacheFilledPercentChange(int)));
    QObject::connect(&mediaPlayer_, SIGNAL(cacheFilled()),
                     this, SLOT(onCacheFilled()));

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
        delete episodeBuilders_.values().at(i);
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

    for (int i = episodes.size() - 1; i >= 0; --i)
    {
        MythUIButtonListItem* item = new MythUIButtonListItem(
                episodeList_,
                episodes.at(i)->title,
                QVariant::fromValue(episodes.at(i)));
    }

    if (episodes.size() > 0)
    {
        onEpisodeSelected(episodeList_->GetItemCurrent());
    }
}

void ProgramWindow::onEpisodeClicked(MythUIButtonListItem* item)
{
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

    mediaPlayer_.loadEpisode(episode);
}

void ProgramWindow::onEpisodeSelected(MythUIButtonListItem* item)
{
    QVariant itemData = item->GetData();

    Episode* episode = itemData.value<Episode*>();

    episodePreviewImage_->SetFilename(episode->episodeImageFilepath);
    imageLoader_.loadImage(episodePreviewImage_);

    episodeDescriptionText_->SetText(episode->description);
    episodeTitleText_->SetText(episode->title);
    episodeAvailableToDateText_->SetText(episode->availableUntilDate);

    this->Refresh();
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

    mediaPlayer_.quit();
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

bool ProgramWindow::keyPressEvent(QKeyEvent *event)
{
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
            if (progressDialog_ != NULL && progressDialog_->IsVisible())
            {
                mediaPlayer_.quit();
                progressDialog_->Close();
                progressDialog_ = NULL;
            }
            else
            {
                Close();
            }
        }
        else if (action == "UP")
        {
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
            SetFocusWidget(episodeList_);
        }
        else
            handled = false;
    }

    if (!handled && MythScreenType::keyPressEvent(event))
        handled = true;

    return handled;
}
