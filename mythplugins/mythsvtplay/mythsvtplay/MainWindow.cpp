#include "MainWindow.h"

#include <QVariant>

#include <mythtv/mythcontext.h>
#include <mythtv/mythverbose.h>
#include <mythtv/libmythui/mythsystem.h>
#include <mythtv/libmythui/mythprogressdialog.h>
#include <mythtv/libmythui/mythuibuttontree.h>
#include <mythtv/libmythui/mythuibuttonlist.h>
#include <mythtv/libmythui/mythdialogbox.h>

#include "Episode.h"
#include "Program.h"

#include "ProgramListCache.h"
#include "EpisodeListBuilder.h"

#include "ProgramWindow.h"

MainWindow::MainWindow(MythScreenStack *parentStack)
    : MythScreenType(parentStack, "MainWindow"),
      progressDialog_(NULL),
      programListCache_(new ProgramListCache())
{
    if (!LoadWindowFromXML("mythsvtplay/svtplay-ui.xml", "main", this))
    {
        throw "Could not load svtplay-ui.xml";
    }

    bool err = false;
    UIUtilE::Assign(this, programTree_, "program-tree", &err);

    UIUtilE::Assign(this, programLogoImage_, "program-logo-image", &err);
    UIUtilE::Assign(this, programDescriptionText_ , "program-description", &err);
    UIUtilE::Assign(this, programTitleText_ , "program-title", &err);

    QObject::connect(programListCache_, SIGNAL(cacheFilled()),
                     this, SLOT(populateTree()));

    QObject::connect(programTree_, SIGNAL(itemClicked(MythUIButtonListItem*)),
                     this, SLOT(onListButtonClicked(MythUIButtonListItem*)));
    QObject::connect(programTree_, SIGNAL(itemSelected(MythUIButtonListItem*)),
                     this, SLOT(onListButtonSelected(MythUIButtonListItem*)));

    QObject::connect(&imageLoader_, SIGNAL(imageReady(MythUIImage*)),
                     this, SLOT(onImageReady(MythUIImage*)));

    BuildFocusList();
}

MainWindow::~MainWindow()
{
    programList_.clear();
    delete programListCache_;

    programTree_->DeleteAllChildren();
    delete programTree_;
}

void MainWindow::beginProgramDownload(bool refreshCache)
{
    if (programListCache_->isEmpty() || refreshCache)
    {
        progressDialog_ = new ProgressDialog(GetScreenStack(), "cache-dialog-1", "Downloading programs ...", "");

        QObject::connect(progressDialog_, SIGNAL(cancelClicked()),
                         this, SLOT(abortProgramsDownload()));
        QObject::connect(programListCache_, SIGNAL(numberOfProgramsFound(int)),
                         progressDialog_, SLOT(setTotal(int)));
        QObject::connect(programListCache_, SIGNAL(numberOfProgramsComplete(int)),
                         progressDialog_, SLOT(setProgress(int)));
        QObject::connect(programListCache_, SIGNAL(programComplete(QString)),
                         progressDialog_, SLOT(setStatusText(QString)));

        GetScreenStack()->AddScreen(progressDialog_);

        programListCache_->refresh();
    }
    else
    {
        populateTree();
    }
}

MythGenericTree* MainWindow::createAlphabeticTree(const QList<Program*>& programs)
{
    MythGenericTree* tree = new MythGenericTree(QString::fromUtf8("Program A-Ö"));

    QString firstLetter = "";
    MythGenericTree* currentNode = NULL;

    // List is assumed to be sorted correctly
    for (int i = 0; i < programs.count(); ++i)
    {
        if (programs[i]->firstLetter != firstLetter)
        {
            firstLetter = programs[i]->firstLetter;
            currentNode = new MythGenericTree(firstLetter);
            tree->addNode(currentNode);
        }

        MythGenericTree* programNode = new MythGenericTree(programs[i]->title);
        programNode->SetData(qVariantFromValue(programs[i]));

        currentNode->addNode(programNode);
    }

    return tree;
}

MythGenericTree* MainWindow::createCategoryTree(const QList<Program*>& programs)
{
    MythGenericTree* tree = new MythGenericTree(QString::fromUtf8("Kategorier"));

    QMultiMap<QString, Program*> categoryProgramMap;

    for (int i = 0; i < programs.count(); ++i)
    {
        categoryProgramMap.insert(programs.at(i)->category, programs.at(i));
    }

    for (int i = 0; i < categoryProgramMap.uniqueKeys().count(); ++i)
    {
        QString category = categoryProgramMap.uniqueKeys().at(i);
        QList<Program*> programsInCategory = categoryProgramMap.values(category);

        MythGenericTree* categoryNode = new MythGenericTree(category);

        for (int j = programsInCategory.count() - 1; j >= 0; --j)
        {
            Program* program = programsInCategory.at(j);
            MythGenericTree* programNode = new MythGenericTree(program->title);
            programNode->SetData(qVariantFromValue(program));

            categoryNode->addNode(programNode);
        }

        tree->addNode(categoryNode);
    }

    return tree;
}

MythGenericTree* MainWindow::createFavoritesTree(const QList<Program*>& programs)
{
    MythGenericTree* tree = new MythGenericTree(QString::fromUtf8("Favoriter"));

    for (int i = 0; i < programs.count(); ++i)
    {
        if (favoritesStore_.favorites().contains(programs.at(i)->title))
        {
            MythGenericTree* programNode = new MythGenericTree(programs.at(i)->title);
            programNode->SetData(qVariantFromValue(programs.at(i)));

            tree->addNode(programNode);
        }
    }

    return tree;
}

void MainWindow::populateTree()
{
    programList_ = programListCache_->programs();

    programTreeData_ = new MythGenericTree("Program A-Ö");

    programTreeData_->addNode(createAlphabeticTree(programList_));
    programTreeData_->addNode(createCategoryTree(programList_));
    programTreeData_->addNode(createFavoritesTree(programList_));

    programTree_->AssignTree(programTreeData_);

    if (progressDialog_)
    {
        progressDialog_->Close();
        progressDialog_ = NULL;
    }
}

void MainWindow::onListButtonClicked(MythUIButtonListItem *item)
{
    MythGenericTree* node = item->GetData().value<MythGenericTree*>();

    QVariant itemData = node->GetData();
    Program* program = itemData.value<Program*>();

    if (program != NULL)
    {
        ProgramWindow* window = new ProgramWindow(GetScreenStack(), program);
        GetScreenStack()->AddScreen(window);
    }

}

void MainWindow::onListButtonSelected(MythUIButtonListItem *item)
{
    MythGenericTree* node = item->GetData().value<MythGenericTree*>();

    QVariant itemData = node->GetData();
    Program* program = itemData.value<Program*>();

    if (program != NULL)
    {
        programTitleText_->SetText(program->title);
        programDescriptionText_->SetText(program->description);

        programLogoImage_->SetFilename(program->logoFilepath);
        programLogoImage_->Load();
    }
    else
    {
        programTitleText_->SetText("");
        programDescriptionText_->SetText("");
        programLogoImage_->Reset();
    }

    this->Refresh();
}

void MainWindow::onImageReady(MythUIImage* image)
{
    image->Load();
}

void MainWindow::abortProgramsDownload()
{
    if (progressDialog_)
    {
        progressDialog_->Close();
        progressDialog_ = NULL;
    }

    Close();
}

void MainWindow::onRefreshDialogResult(bool result)
{
    if (result)
    {
        beginProgramDownload(true);
    }
}

bool MainWindow::keyPressEvent(QKeyEvent *event)
{
    bool handled = false;

    if (!handled && GetFocusWidget() && GetFocusWidget()->keyPressEvent(event))
        return true;

    QStringList actions;
    handled = GetMythMainWindow()->TranslateKeyPress("MainWindow", event, actions);

    for (int i = 0; i < actions.size() && !handled; i++)
    {
        QString action = actions[i];
        handled = true;

        if (action == "ESCAPE" || action == "LEFT")
        {
            Close();
        }
        else if (action == "MENU")
        {
            confirmCacheRefreshDialog_ = new MythConfirmationDialog(GetScreenStack(), "Refresh program cache?");
            confirmCacheRefreshDialog_->Create();

            QObject::connect(confirmCacheRefreshDialog_, SIGNAL(haveResult(bool)),
                             this, SLOT(onRefreshDialogResult(bool)));

            GetScreenStack()->AddScreen(confirmCacheRefreshDialog_);
        }
        else if (action == "FAVORITE")
        {
            MythGenericTree* currentNode = programTree_->GetCurrentNode();

            QVariant itemData = currentNode->GetData();
            Program* program = itemData.value<Program*>();

            if (program != NULL)
            {
                MythGenericTree* favoritesNode = programTreeData_->getChildByName("Favoriter");

                if (favoritesStore_.favorites().contains(program->title))
                {
                    favoritesStore_.remove(program->title);

                    MythGenericTree* nodeToRemove = favoritesNode->getChildByName(program->title);

                    if (nodeToRemove == currentNode)
                    {
                        programTree_->RemoveCurrentItem(true);
                    }
                    else
                    {
                        favoritesNode->removeNode(nodeToRemove);
                    }
                }
                else
                {
                    favoritesStore_.add(program->title);

                    favoritesNode->deleteAllChildren();
                    programTreeData_->removeNode(favoritesNode);
                    programTreeData_->addNode(createFavoritesTree(programList_));
                }
            }
        }
        else
            handled = false;
    }

    if (!handled && MythScreenType::keyPressEvent(event))
        handled = true;

    return handled;
}
