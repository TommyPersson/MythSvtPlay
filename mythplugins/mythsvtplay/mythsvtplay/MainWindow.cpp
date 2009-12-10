#include "MainWindow.h"

#include <QVariant>

#include <mythtv/mythcontext.h>
#include <mythtv/mythverbose.h>
#include <mythtv/libmythui/mythsystem.h>
#include <mythtv/libmythui/mythprogressdialog.h>
#include <mythtv/libmythui/mythuibuttontree.h>

#include "ProgramTreeBuilder.h"

MainWindow::MainWindow(MythScreenStack *parentStack)
        : MythScreenType(parentStack, "MainWindow")
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

    treeBuilder_ = new ShowTreeBuilder();

    QObject::connect(treeBuilder_, SIGNAL(treeBuilt(MythGenericTree*)),
                     this, SLOT(populateTree(MythGenericTree*)));

    QObject::connect(programTree_, SIGNAL(itemClicked(MythUIButtonListItem*)),
                     this, SLOT(onListButtonClicked(MythUIButtonListItem*)));

    busyDialog_ = ShowBusyPopup("Loading program information...");

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
    }



}
