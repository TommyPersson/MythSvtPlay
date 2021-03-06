#include "ProgramWindow.h"

#include "Program.h"
#include "ProgressDialog.h"
#include "EpisodeListBuilder.h"
#include "PlainMediaPlayer.h"
#include "RtmpMediaPlayer.h"

#include <iostream>
#include <typeinfo>

#include <QCoreApplication>

#include <mythmainwindow.h>
#include <mythdialogbox.h>

#include <mythtv/mythcontext.h>

#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythprogressdialog.h>
#include <mythtv/libmythui/mythuibuttontree.h>

ProgramWindow::ProgramWindow(MythScreenStack *parentStack, Program* program, bool disposeProgramOnExit)
    : MythScreenType(parentStack, "ProgramWindow"),
      program_(program),
      progressDialog_(NULL),
      mediaPlayer_(NULL),
      disposeProgramOnExit_(disposeProgramOnExit)
{
    if (!LoadWindowFromXML("mythsvtplay/svtplay-ui.xml", "program-view", this))
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

    QObject::connect(&imageLoader_, SIGNAL(imageReady()),
                     this, SLOT(onImageReady()));

    programTitleText_->SetText(program->title);
    programDescriptionText_->SetText(program->description);

    programLogoImage_->SetFilename(program->logoFilepath);
    programLogoImage_->Load();

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
        QString episodeType = program->episodeTypeLinks.at(i).first;

        EpisodeListBuilder* builder = new EpisodeListBuilder(
                                            episodeType,
                                            QUrl("http://svtplay.se" + link));

        episodeBuilders_[episodeType] = builder;

        QObject::connect(builder, SIGNAL(episodesReady(QString)),
                         this, SLOT(onEpisodesReady(QString)));

        builder->buildEpisodeList();

        addBusyImage(episodeType);
    }

    BuildFocusList();
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

    if (disposeProgramOnExit_)
    {
        delete program_;
    }
}

void ProgramWindow::onEpisodesReady(const QString& episodeType)
{
    QList<IProgramItem*> episodes = episodeBuilders_[episodeType]->episodeList();

    removeBusyImage(episodeType);

    program_->episodesByType[episodeType] = episodes;

    if (episodeTypeList_->GetItemCurrent()->GetText() == episodeType)
    {
       populateEpisodeList();
    }
}


void ProgramWindow::onEpisodeClicked(MythUIButtonListItem* item)
{
    if (item->GetText() == QString::fromUtf8("Hämta fler ..."))
    {
        episodeBuilders_[selectedEpisodeType_]->buildEpisodeList();

        item->SetText(QString::fromUtf8("Hämtar ..."));

        addBusyImage(selectedEpisodeType_);

        return;
    }
    else if (item->GetText() == QString::fromUtf8("Hämtar ..."))
    {
        return;
    }

    QVariant itemData = item->GetData();

    IProgramItem* programItem = itemData.value<IProgramItem*>();

    handleProgramItem(programItem);
}

void ProgramWindow::handleProgramItem(IProgramItem* item)
{
    if (Episode* episode = dynamic_cast<Episode*>(item)) {
        handleEpisode(episode);
    }
    else if (EpisodeDirectory* episodeDirectory = dynamic_cast<EpisodeDirectory*>(item)) {
        handleEpisodeDirectory(episodeDirectory);
    }
    else
    {
        std::cerr << "Program item was neither an episode or directory. Should not happen." << std::endl;
    }
}

void ProgramWindow::handleEpisode(Episode* episode)
{
    setupMediaPlayer(episode);
}

void ProgramWindow::handleEpisodeDirectory(EpisodeDirectory* dir)
{
    ProgramWindow* window = new ProgramWindow(GetScreenStack(), dir->createProgram(*program_), true);
    GetScreenStack()->AddScreen(window);
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

    IProgramItem* programItem = itemData.value<IProgramItem*>();

    episodePreviewImage_->Reset();
    episodePreviewImage_->SetFilename(programItem->episodeImageFilepath);
    imageLoader_.loadImage(episodePreviewImage_->GetFilename());

    episodeDescriptionText_->SetText(programItem->description);
    episodeTitleText_->SetText(programItem->title);

    if (Episode* episode = dynamic_cast<Episode*>(programItem))
    {
        episodeAvailableToDateText_->SetText(episode->availableUntilDate);
    }
    else
    {
        episodeAvailableToDateText_->SetText("");
    }

    Refresh();
}

void ProgramWindow::onEpisodeTypeSelected(MythUIButtonListItem* item)
{
    selectedEpisodeType_ = item->GetText();

    populateEpisodeList();
}

void ProgramWindow::onCancelClicked()
{
    closeProgressDialog();

    stopMediaPlayer();
}

void ProgramWindow::onImageReady()
{
    episodePreviewImage_->Load();
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
    closeProgressDialog();
}

void ProgramWindow::onMediaPlayerDestroyed()
{
    mediaPlayer_ = NULL;
}

void ProgramWindow::onConnectionFailed()
{
    closeProgressDialog();

    MythConfirmationDialog* dialog = new MythConfirmationDialog(GetScreenStack(), "Connection failed, please try again.", false);
    dialog->Create();
    GetScreenStack()->AddScreen(dialog);
    return;
}

void ProgramWindow::populateEpisodeList()
{
    int previousPosition = episodeList_->GetCurrentPos();

    episodeList_->Reset();

    QList<IProgramItem*> episodes = program_->episodesByType[selectedEpisodeType_];

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

        episodeList_->SetItemCurrent(previousPosition);
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
    if (RtmpMediaPlayer::canPlay(episode))
    {
        mediaPlayer_ = new RtmpMediaPlayer();
    }
    else if (PlainMediaPlayer::canPlay(episode))
    {
        mediaPlayer_ = new PlainMediaPlayer();
    }
    else
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

    QObject::connect(mediaPlayer_, SIGNAL(cacheFilledPercent(int)),
                     this, SLOT(onCacheFilledPercentChange(int)));
    QObject::connect(mediaPlayer_, SIGNAL(cacheFilled()),
                     this, SLOT(onCacheFilled()));
    QObject::connect(mediaPlayer_, SIGNAL(connectionFailed()),
                     this, SLOT(onConnectionFailed()));
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

void ProgramWindow::closeProgressDialog()
{
    if (progressDialog_)
    {
        progressDialog_->Close();
        progressDialog_ = NULL;
    }
}

void ProgramWindow::addBusyImage(const QString& episodeType)
{
    // Another thing missing from the MythTV API: MythUIButtonList::GetItemByText()

    QString currentType = selectedEpisodeType_;
    int currentPosition = episodeList_->GetCurrentPos();

    episodeTypeList_->MoveToNamedPosition(episodeType);
    episodeTypeList_->GetItemCurrent()->SetImage("busyimages/%1.png", "busy-animation-image", true);

    selectedEpisodeType_ = currentType;

    episodeTypeList_->MoveToNamedPosition(currentType);
    episodeList_->SetItemCurrent(currentPosition);
}

void ProgramWindow::removeBusyImage(const QString& episodeType)
{
    QString currentType = selectedEpisodeType_;
    int currentPosition = episodeList_->GetCurrentPos();

    episodeTypeList_->MoveToNamedPosition(episodeType);
    episodeTypeList_->GetItemCurrent()->SetImage("mythsvtplay/images/invisible%1.png", "busy-animation-image", true);

    selectedEpisodeType_ = currentType;

    episodeTypeList_->MoveToNamedPosition(currentType);
    episodeList_->SetItemCurrent(currentPosition);
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
            doClose();
        }
        else
            handled = false;
    }

    if (!handled && MythScreenType::keyPressEvent(event))
        handled = true;

    return handled;
}

void ProgramWindow::doClose()
{
    // Hopefully temporary fix to prevent crash when closing
    // the window precisely when signals are received in the downloader.
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
