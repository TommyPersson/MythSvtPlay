#include "ProgressDialog.h"

#include "mythverbose.h"

#include <mythtv/libmythui/mythuibutton.h>
#include <mythtv/libmythui/mythuiprogressbar.h>
#include <mythtv/libmythui/mythuitext.h>
#include <mythtv/libmythui/mythmainwindow.h>

ProgressDialog::ProgressDialog(MythScreenStack *parentStack, const char* name, const QString& titleText, const QString& statusText)
    : MythScreenType(parentStack, name),
    total_(100),
    progress_(0)
{
    if (!LoadWindowFromXML("svtplay-ui.xml", "cancellable-progress-dialog", this))
    {
        throw "Could not load svtplay-ui.xml";
    }

    bool err = false;
    UIUtilE::Assign(this, titleText_, "title-text", &err);
    UIUtilE::Assign(this, statusText_, "status-text", &err);
    UIUtilE::Assign(this, cancelButton_, "cancel-button", &err);
    UIUtilE::Assign(this, progressBar_, "progressbar", &err);

    if (err)
    {
        VERBOSE(VB_IMPORTANT, QString("Could not bind to all ui elements!"));
        throw "Blargh";
    }

    QObject::connect(cancelButton_, SIGNAL(Clicked()), this, SIGNAL(cancelClicked()));

    titleText_->SetText(titleText);

    statusText_->SetText(statusText);

    cancelButton_->SetText("Cancel");

    progressBar_->SetTotal(total_);
    progressBar_->SetStart(0);
    progressBar_->SetUsed(progress_);

    SetFocusWidget(cancelButton_);
}

ProgressDialog::~ProgressDialog()
{}

bool ProgressDialog::keyPressEvent(QKeyEvent *event)
{
    QStringList actions;
    bool handled = GetMythMainWindow()->TranslateKeyPress("qt", event, actions, false);

    for (int i = 0; i < actions.size() && !handled; i++)
    {
        QString action = actions[i];
        handled = true;

        if (action == "ESCAPE")
        {
            emit cancelClicked();
        }
        else
            handled = false;
    }

    if (!handled && MythScreenType::keyPressEvent(event))
        handled = true;

    return handled;
}

void ProgressDialog::setTitleText(const QString& text)
{
    titleText_->SetText(text);
}

void ProgressDialog::setStatusText(const QString& text)
{
    statusText_->SetText(text);
}

void ProgressDialog::setProgress(int progress)
{
    progress_ = progress;
    updateProgress();
}

void ProgressDialog::setTotal(int total)
{
    total_ = total;
    updateProgress();
}

void ProgressDialog::updateProgress()
{
    if (progress_ > total_)
    {
        VERBOSE(VB_IMPORTANT, QString("Progress count (%1) is higher "
                                      "than total (%2)")
                .arg(progress_)
                .arg(total_));
        return;
    }
    else
    {
        progressBar_->SetTotal(total_);
        progressBar_->SetUsed(progress_);
    }
}
