#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <mythscreentype.h>

class ProgressDialog : public MythScreenType
{
    Q_OBJECT

public:

    ProgressDialog(MythScreenStack *parentStack, const char* name, const QString& titleText = "", const QString& statusText = "");
    ~ProgressDialog();

    bool keyPressEvent(QKeyEvent *event);

public slots:

    void setTitleText(const QString& text);
    void setStatusText(const QString& text);
    void setProgress(int progress);
    void setTotal(int total);

signals:

    void cancelClicked();

private:

    void updateProgress();

    int total_;
    int progress_;

    MythUIText* titleText_;
    MythUIText* statusText_;
    MythUIButton* cancelButton_;
    MythUIProgressBar* progressBar_;

};

#endif // PROGRESSDIALOG_H
