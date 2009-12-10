#include "MainWindow.h"

#include <QVariant>

#include <mythtv/mythcontext.h>
#include <mythtv/mythverbose.h>
#include <mythtv/libmythui/mythsystem.h>
#include <mythtv/libmythui/mythprogressdialog.h>
#include <mythtv/libmythui/mythuibuttontree.h>
#include <mythtv/libmythui/mythdialogbox.h>

#include "Episode.h"

#include "ShowTreeBuilder.h"
#include "EpisodeListBuilder.h"

MainWindow::MainWindow(MythScreenStack *parentStack)
    : MythScreenType(parentStack, "MainWindow"),
      treeBuilder_(new ShowTreeBuilder()),
      episodeListBuilder_(new EpisodeListBuilder())
{
}

MainWindow::~MainWindow()
{
    delete treeBuilder_;
}

bool MainWindow::Create()
{
    if (!LoadWindowFromXML("svtplay-ui.xml", "main", this))
    {
        std::cerr << "Omg failed to load svtplay-ui.xml" << std::endl;
        return false;
    }

    bool err = false;
    UIUtilE::Assign(this, programTree_, "program-tree", &err);

    QObject::connect(treeBuilder_, SIGNAL(treeBuilt(MythGenericTree*)),
                     this, SLOT(populateTree(MythGenericTree*)));

    QObject::connect(programTree_, SIGNAL(itemClicked(MythUIButtonListItem*)),
                     this, SLOT(onListButtonClicked(MythUIButtonListItem*)));

    QObject::connect(episodeListBuilder_, SIGNAL(episodesLoaded(QList<Episode*>)),
                     this, SLOT(onReceiveEpisodes(QList<Episode*>)));
    QObject::connect(episodeListBuilder_, SIGNAL(noEpisodesFound()),
                     this, SLOT(onNoEpisodesReceived()));

    busyDialog_ = ShowBusyPopup("Downloading available shows ...");

    treeBuilder_->buildTree();

    BuildFocusList();

    return true;
}

void MainWindow::populateTree(MythGenericTree* tree)
{
    if (!tree)
    {
        std::cerr << "Null tree :(" << std::endl;
    }
    else
    {
        this->programTree_->AssignTree(tree);
    }

    if (busyDialog_)
        busyDialog_->Close();
}

void MainWindow::onListButtonClicked(MythUIButtonListItem *item)
{
    MythGenericTree* node = item->GetData().value<MythGenericTree*>();

    QVariant itemData = node->GetData();

    if (itemData.type() == QVariant::String)
    {
        QString data = itemData.toString();

        std::cerr << data.toStdString() << std::endl;

        QUrl url("http://svtplay.se" + data);

        busyDialog_ = ShowBusyPopup("Downloading episode data ...");

        episodeListBuilder_->buildEpisodeListFromUrl(url);
    }

}

void MainWindow::onReceiveEpisodes(QList<Episode*> episodes)
{
    if (busyDialog_)
        busyDialog_->Close();

    for (int i = 0; i < episodes.count(); ++i)
    {
        std::cerr << episodes.at(i)->asxUrl.toString().toStdString() << std::endl;


    }

    QString url = episodes.at(0)->asxUrl.toString();

    gContext->sendPlaybackStart();
    myth_system("mplayer -playlist " + url);
    gContext->sendPlaybackEnd();
}

void MainWindow::onNoEpisodesReceived()
{
    if (busyDialog_)
        busyDialog_->Close();
}
