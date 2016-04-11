#ifndef DFILEMANAGERWINDOW_H
#define DFILEMANAGERWINDOW_H

#include "dmovablemainwindow.h"
#include <dtitlebar.h>

#define DEFAULT_WINDOWS_WIDTH 950
#define DEFAULT_WINDOWS_HEIGHT 600
#define LEFTSIDEBAR_MAX_WIDTH 200
#define LEFTSIDEBAR_MIN_WIDTH 60
#define TITLE_FIXED_HEIGHT 40

class DTitleBar;
class DLeftSideBar;
class DToolBar;
class DFileView;
class DDetailView;
class QStatusBar;
class QFrame;
class QHBoxLayout;
class QVBoxLayout;
class QSplitter;
class QResizeEvent;
class DSplitter;

DWIDGET_USE_NAMESPACE

class DFileManagerWindow : public DMovableMainWindow
{
    Q_OBJECT
public:
    explicit DFileManagerWindow(QWidget *parent = 0);
    ~DFileManagerWindow();

    static const int MinimumWidth;

    void initData();
    void initUI();
    void initTitleBar();
    void initSplitter();

    void initLeftSideBar();

    void initRightView();
    void initToolBar();
    void initFileView();
    void initDetailView();

    void initCentralWidget();
    void initStatusBar();
    void initConnect();
signals:
    void aboutToClose();

public slots:
    void showMinimized();

protected:
    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent *event);
    void closeEvent(QCloseEvent* event);

private:
    QFrame* m_centralWidget;
    DTitlebar* m_titleBar = NULL;
    DLeftSideBar* m_leftSideBar = NULL;
    QFrame* m_rightView = NULL;
    DToolBar* m_toolbar = NULL;
    DFileView* m_fileView = NULL;
    DDetailView* m_detailView = NULL;
    QStatusBar* m_statusBar = NULL;
    QVBoxLayout* m_mainLayout;
    QVBoxLayout* m_viewLayout;
    DSplitter* m_splitter;
    QFrame * m_titleFrame = NULL;
};

#endif // DFILEMANAGERWINDOW_H
