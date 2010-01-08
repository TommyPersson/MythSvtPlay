#include "ProgramWindow.h"

#include <mythtv/libmythui/mythprogressdialog.h>
#include <mythtv/libmythui/mythuibuttontree.h>

ProgramWindow::ProgramWindow(MythScreenStack *parentStack, const Program& program)
    : MythScreenType(parentStack, "ProgramWindow"),
      program_(program)
{
    if (!LoadWindowFromXML("svtplay-ui.xml", "program-view", this))
    {
        throw "Could not load svtplay-ui.xml";
    }

    bool err = false;
    UIUtilE::Assign(this, episodeList_, "episode-tree", &err);

    QObject::connect(episodeList_, SIGNAL(itemClicked(MythUIButtonListItem*)),
                     this, SLOT(onEpisodeSelected(MythUIButtonListItem*)));

    QObject::connect(&mediaPlayer_, SIGNAL(finished()),
                     this, SLOT(onFinishedPlayback()));

    populateEpisodeList();

    SetFocusWidget(episodeList_);
}

ProgramWindow::~ProgramWindow()
{

}

void ProgramWindow::populateEpisodeList()
{
    MythGenericTree* list = new MythGenericTree("root-node");

    for (int i = 0; i < program_.episodes.size(); ++i)
    {
        MythGenericTree* episodeNode = new MythGenericTree(program_.episodes.at(i)->title);
        episodeNode->SetData(QVariant::fromValue(program_.episodes.at(i)));

        list->addNode(episodeNode);
    }

    episodeList_->AssignTree(list);
}

void ProgramWindow::onEpisodeSelected(MythUIButtonListItem *item)
{
    MythGenericTree* node = item->GetData().value<MythGenericTree*>();

    QVariant itemData = node->GetData();

    Episode* episode = itemData.value<Episode*>();

    busyDialog_ = ShowBusyPopup("Buffering episode stream ...");

    mediaPlayer_.loadEpisode(episode);
    mediaPlayer_.start();
}

void ProgramWindow::onFinishedPlayback()
{
    busyDialog_->Close();
}
