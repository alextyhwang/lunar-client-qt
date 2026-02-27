//
// Created by nils on 11/4/21.
//

#include "mainwindow.h"

#include <QCoreApplication>
#include <QGridLayout>
#include <QListWidgetItem>
#include <QComboBox>
#include <QStatusBar>
#include <QScrollArea>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QFileSystemModel>
#include <QIODevice>
#include <QLabel>

#include "pages/configurationpage.h"
#include "pages/gamepage.h"
#include "pages/logspage.h"
#include "launch/launcher.h"
#include "buildconfig.h"
#include "widgets/widgetutils.h"
#include "util/fs.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), config(Config::load()), offlineLauncher(config, false, NULL){
    setWindowTitle(QStringLiteral("ATW Client v") + BuildConfig::VERSION);
    static QString icon = FS::combinePaths(QCoreApplication::applicationDirPath(), QStringLiteral("icon.ico"));
    if (QFile::exists(icon))
        setWindowIcon(QIcon(icon));
    else {
        QString lcloc =
#if defined(Q_OS_WIN)
            QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/Programs/lunarclient/Lunar Client.exe");
#elif defined(Q_OS_DARWIN)
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/lunarclient/Lunar Client"; //Need location
#else
            QDir::homePath() + "/lunarclient/Lunar Client"; // Need location
#endif
        QFileInfo fin(lcloc);
        QFileSystemModel* model = new QFileSystemModel;
        QIcon ic = model->fileIcon(model->index(fin.filePath()));
        QPixmap pixmap = ic.pixmap(ic.actualSize(QSize(1028, 1028)));
        setWindowIcon(pixmap);
        QFile file(icon);
        file.open(QIODevice::WriteOnly);
        pixmap.save(&file, "ICO");
    }
    

    QWidget* centralWidget = new QWidget();

    QGridLayout* mainLayout = new QGridLayout();

    pageList = new QListWidget();
    pageStack = new QStackedWidget();

    pageStack->setContentsMargins(30, 10, 30, 10);

    pageList->setIconSize(QSize(32, 32));

    pages = {
        new GamePage(config),
        new AgentsPage(config),
        new ModsPage(config),
        new HelpersPage(config),
        new LogsPage(config)
    };
    logsPage = static_cast<LogsPage*>(pages.last());

    for(ConfigurationPage* page : pages){
        new QListWidgetItem(page->icon(), page->title(), pageList);
        pageStack->addWidget(page);
    }

    connect(pageList, &QListWidget::currentRowChanged, pageStack, &QStackedWidget::setCurrentIndex);

    pageList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pageList->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    pageList->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    QFont font;
    font.setPointSize(11);
    pageList->setFont(font);

    launchButton = new QPushButton();
    launchButton->setMinimumHeight(80);
    connect(launchButton, &QPushButton::clicked, this, &MainWindow::launchOrEndProcess);

    connect(&offlineLauncher, &OfflineLauncher::error, this, &MainWindow::errorCallback);
    connect(&offlineLauncher, &OfflineLauncher::processStarted, this, &MainWindow::updateLaunchButtonState);
    connect(&offlineLauncher, &OfflineLauncher::processFinished, this, &MainWindow::updateLaunchButtonState);

    processCheckTimer = new QTimer(this);
    connect(processCheckTimer, &QTimer::timeout, this, &MainWindow::updateLaunchButtonState);
    processCheckTimer->start(1000);

    resetLaunchButtons();

    mainLayout->addWidget(pageList);
    mainLayout->addWidget(launchButton, 2, 0);


    QFrame *frame = new QFrame;
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Sunken);

    QLabel *title = new QLabel;
    QFont titleFont;
    titleFont.setPointSize(13);
    title->setFont(titleFont);
    QLabel *description = new QLabel;

    QVBoxLayout *frameLayout = new QVBoxLayout(frame);

#ifdef INCLUDE_UPDATER
    QHBoxLayout *titleBtnLayout = new QHBoxLayout;
    QPushButton *checkForUpdates = new QPushButton("Check for updates");
    titleBtnLayout->addWidget(title, 1);
    titleBtnLayout->addWidget(checkForUpdates);

    frameLayout->addLayout(titleBtnLayout);

    connect(checkForUpdates, &QPushButton::clicked, [this]{updaterChecker.checkForUpdates(true);});
#else
    frameLayout->addWidget(title);
#endif
    frameLayout->addWidget(description);


    connect(pageList, &QListWidget::currentRowChanged, [this, title, description](int current) {
        if (current >= 0 && current < pages.size()) {
            title->setText(pages[current]->title());
            description->setText(pages[current]->description());
        }
    });

    pageList->setCurrentRow(0);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(frame);
    layout->addWidget(pageStack, 1);
    mainLayout->addLayout(layout, 0, 1, -1, 1);

    setCentralWidget(WidgetUtils::layoutToWidget(mainLayout));
    resize(800, 600);

    load();

#ifdef INCLUDE_UPDATER
    connect(&updaterChecker, &UpdateChecker::updateAvailable, this, &MainWindow::updateAvailable);
    connect(&updaterChecker, &UpdateChecker::noUpdatesAvailable, this, &MainWindow::noUpdatesAvailable);

    updaterChecker.checkForUpdates(false);
#endif
}

void MainWindow::resetLaunchButtons() {
    updateLaunchButtonState();
}

void MainWindow::updateLaunchButtonState() {
    if (offlineLauncher.isProcessRunning()) {
        launchButton->setText(QStringLiteral("End Process"));
        launchButton->setEnabled(true);
    } else {
        launchButton->setText(QStringLiteral("Launch"));
        launchButton->setEnabled(true);
    }
}

void MainWindow::launchOrEndProcess() {
    if (offlineLauncher.isProcessRunning()) {
        offlineLauncher.endProcess();
        if (logsPage)
            logsPage->stopPolling();
        return;
    }
    apply();
    if (offlineLauncher.launch()) {
        if (logsPage) {
            logsPage->startPolling();
            pageList->setCurrentRow(pages.indexOf(logsPage));
        }
        if (config.closeOnLaunch)
            close();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    apply();
    config.save();
    event->accept();
}

void MainWindow::apply() {
    for(ConfigurationPage *page : pages) {
        page->apply();
    }
    config.gameVersion = QStringLiteral("1.8.9");
    config.modLoader = QStringLiteral("Optifine");
}

void MainWindow::load() {
    for(ConfigurationPage* page : pages){
        page->load();
    }
}


void MainWindow::errorCallback(const QString &message) {
    statusBar()->showMessage(message, 10000);
    if (logsPage) {
        logsPage->stopPolling();
        pageList->setCurrentRow(pages.indexOf(logsPage));
        logsPage->appendToMainLog(QStringLiteral("[Launcher Error] ") + message);
    }
}

#ifdef INCLUDE_UPDATER

void MainWindow::updateAvailable(const QString &url) {
    statusBar()->showMessage(QStringLiteral("Update available: ") + url, 15000);
}

void MainWindow::noUpdatesAvailable() {
    statusBar()->showMessage(QStringLiteral("No updates available."), 3000);
}

#endif
