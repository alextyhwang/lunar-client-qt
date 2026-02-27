//
// Game settings - memory, JRE, Minecraft dir, JVM args.
//

#include "gamepage.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QGroupBox>
#include <QRadioButton>

#include "gui/widgets/filechooser.h"
#include "gui/widgets/widgetutils.h"
#include <QDesktopServices>
#include <QUrl>
#include <QStandardPaths>

#include "util/fs.h"

#ifndef _WIN32
#include <unistd.h>
unsigned long long getSystemMemory() {
	long pages = sysconf(_SC_PHYS_PAGES);
	long pageSize = sysconf(_SC_PAGE_SIZE);
	return pages * pageSize;
}
#else
#include <windows.h>
unsigned long long getSystemMemory() {
	MEMORYSTATUSEX status;
	status.dwLength = sizeof(status);
	GlobalMemoryStatusEx(&status);
	return status.ullTotalPhys;
}
#endif

GamePage::GamePage(Config& config, QWidget *parent) : ConfigurationPage(config, parent) {
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setSpacing(40);

    unsigned long long systemMemory = getSystemMemory();
    size_t mibMemory = (size_t)(systemMemory / 1024 / 1024);
    size_t pageStep = (size_t)(mibMemory / 16);

    QLabel* memoryLabel = new QLabel();
    memorySlider = new QSlider(Qt::Horizontal);
    memorySlider->setMinimum(1024);
    memorySlider->setMaximum(mibMemory);
    memorySlider->setPageStep(pageStep);

    connect(memorySlider, &QSlider::valueChanged, [memoryLabel](int val){memoryLabel->setText(QStringLiteral("Memory: ") + QString::number(val) + QStringLiteral(" MiB"));});

    QVBoxLayout* memorySliderContainer = new QVBoxLayout();
    memorySliderContainer->setSpacing(6);
    memorySliderContainer->addWidget(memoryLabel, 0, Qt::AlignHCenter);
    memorySliderContainer->addWidget(memorySlider);

    QVBoxLayout* jreContainer = new QVBoxLayout();
    jreContainer->setSpacing(6);
    jreContainer->addWidget(new QLabel(QStringLiteral("Custom JRE path (leave empty for default)")), 0, Qt::AlignHCenter);
    jrePath = new FileChooser(QFileDialog::ExistingFile);
    jreContainer->addWidget(jrePath);

    QVBoxLayout* minecraftContainer = new QVBoxLayout();
    minecraftContainer->setSpacing(6);
    minecraftContainer->addWidget(new QLabel(QStringLiteral("Minecraft directory (leave empty for default)")), 0, Qt::AlignHCenter);
    minecraftPathChooser = new FileChooser(QFileDialog::Directory);
    minecraftContainer->addWidget(minecraftPathChooser);

    QVBoxLayout* jvmArgsGroup = new QVBoxLayout();
    jvmArgsGroup->setSpacing(6);
    jvmArgs = new QPlainTextEdit();
    jvmArgsGroup->addWidget(new QLabel(QStringLiteral("JVM Arguments")), 0, Qt::AlignHCenter);
    jvmArgsGroup->addWidget(jvmArgs);

    QGroupBox* groupBox = new QGroupBox(QStringLiteral("After Launch"));
    QRadioButton* stayOpen = new QRadioButton(QStringLiteral("Keep Launcher Open"));
    closeOnLaunch = new QRadioButton(QStringLiteral("Close Launcher"));
    stayOpen->setChecked(true);
    QVBoxLayout* radioLayout = new QVBoxLayout();
    radioLayout->setSpacing(6);
    radioLayout->addWidget(stayOpen);
    radioLayout->addWidget(closeOnLaunch);
    groupBox->setLayout(radioLayout);

    openDataFolder = new QPushButton(QStringLiteral("Open Configuration Folder"));
    connect(openDataFolder, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/atw-client/"));
    });

    mainLayout->addLayout(memorySliderContainer);
    mainLayout->addLayout(jreContainer);
    mainLayout->addLayout(minecraftContainer);
    mainLayout->addLayout(jvmArgsGroup, 1);
    mainLayout->addWidget(groupBox);
    mainLayout->addWidget(openDataFolder, 0, Qt::AlignHCenter);

    setLayout(mainLayout);
}

QString GamePage::title() {
    return QStringLiteral("Game");
}

QIcon GamePage::icon() {
    return QIcon(":/res/icons/minecraft.svg");
}

void GamePage::apply() {
    config.keepMemorySame = true;
    int mem = memorySlider->value();
    config.initialMemory = mem;
    config.maximumMemory = mem;

    QString jrePathStr = jrePath->getPath().trimmed();
    config.useCustomJre = !jrePathStr.isEmpty();
    config.customJrePath = jrePathStr;

    QString mcPath = minecraftPathChooser->getPath().trimmed();
    config.useCustomMinecraftDir = !mcPath.isEmpty();
    config.customMinecraftDir = mcPath;

    config.jvmArgs = jvmArgs->toPlainText();
    config.closeOnLaunch = closeOnLaunch->isChecked();
}

void GamePage::load() {
    memorySlider->setValue(config.maximumMemory);
    jrePath->setPath(config.customJrePath);
    minecraftPathChooser->setPath(config.customMinecraftDir);
    jvmArgs->setPlainText(config.jvmArgs);
    closeOnLaunch->setChecked(config.closeOnLaunch);
}

QString GamePage::description() {
    return QStringLiteral("Memory, Java, and game directory settings.");
}
