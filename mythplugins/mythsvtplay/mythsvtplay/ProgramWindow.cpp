#include "ProgramWindow.h"

#include "Program.h"

#include <iostream>

#include <QCoreApplication>

#include <mythtv/mythcontext.h>
#include <mythmainwindow.h>
#include <mythdialogbox.h>

#include <mythtv/libmythui/mythprogressdialog.h>
#include <mythtv/libmythui/mythuibuttontree.h>

ProgramWindow::ProgramWindow(MythScreenStack *parentStack, Program* program)
    : MythScreenType(parentStack, "ProgramWindow"),
      program_(program)
{
    if (!LoadWindowFromXML("svtplay-ui.xml", "program-view", this))
    {
        throw "Could not load svtplay-ui.xml";
    }

    bool err = false;
    UIUtilE::Assign(this, episodeList_, "episode-tree", &err);

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

    QObject::connect(&mediaPlayer_, SIGNAL(finished()),
                     this, SLOT(onFinishedPlayback()));

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

    populateEpisodeList();

    SetFocusWidget(episodeList_);
}

ProgramWindow::~ProgramWindow()
{
    imageLoader_.terminate();
    imageLoader_.wait();
}

void ProgramWindow::populateEpisodeList()
{
    MythGenericTree* list = new MythGenericTree("root-node");

    for (int i = 0; i < program_->episodes.size(); ++i)
    {
        MythGenericTree* episodeNode = new MythGenericTree(program_->episodes.at(i)->title);
        episodeNode->SetData(QVariant::fromValue(program_->episodes.at(i)));

        list->addNode(episodeNode);
    }

    episodeList_->AssignTree(list);

    onEpisodeSelected(episodeList_->GetItemCurrent());
}

void ProgramWindow::onEpisodeClicked(MythUIButtonListItem *item)
{
    MythGenericTree* node = item->GetData().value<MythGenericTree*>();

    QVariant itemData = node->GetData();

    Episode* episode = itemData.value<Episode*>();

    if (episode->mediaUrl.isEmpty())
    {
        noStreamFoundDialog_ = new MythConfirmationDialog(GetScreenStack(), "Sorry, no suitable stream found.", false);
        noStreamFoundDialog_->Create();
        GetScreenStack()->AddScreen(noStreamFoundDialog_);
        return;
    }

    progressDialog_ = new MythUIProgressDialog("Filling stream buffer ...", GetScreenStack(), "cache-dialog");
    progressDialog_->Create();
    progressDialog_->SetTotal(100);
    GetScreenStack()->AddScreen(progressDialog_);

    mediaPlayer_.playEpisode(episode);
}

void ProgramWindow::onEpisodeSelected(MythUIButtonListItem *item)
{
    MythGenericTree* node = item->GetData().value<MythGenericTree*>();

    QVariant itemData = node->GetData();

    Episode* episode = itemData.value<Episode*>();

    episodePreviewImage_->SetFilename(episode->episodeImageFilepath);
    imageLoader_.loadImage(episodePreviewImage_);

    episodeDescriptionText_->SetText(episode->description);
    episodeTitleText_->SetText(episode->title);
    episodeAvailableToDateText_->SetText(episode->availableUntilDate);

    this->Refresh();
}

void ProgramWindow::onImageReady(MythUIImage* image)
{
    image->Load();
}

void ProgramWindow::onCacheFilledPercentChange(int percent)
{
    progressDialog_->SetProgress(percent);
}

void ProgramWindow::onCacheFilled()
{
    progressDialog_->Close();
}

void ProgramWindow::onFinishedPlayback()
{
}
