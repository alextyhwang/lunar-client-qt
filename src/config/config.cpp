//
// Created by nils on 12/1/21.
//

#include "config.h"

#include <QStandardPaths>
#include <QJsonObject>
#include <QProcess>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QApplication>

static QString resolvePortablePath(const QString& path) {
#ifdef ATW_TEST_PORTABLE
    if (path.isEmpty() || QDir::isAbsolutePath(path))
        return path;

    return QDir(QApplication::applicationDirPath()).filePath(path);
#else
    return path;
#endif
}

static QString storePortablePath(const QString& path) {
#ifdef ATW_TEST_PORTABLE
    if (path.isEmpty() || !QDir::isAbsolutePath(path))
        return path;

    QDir appDir(QApplication::applicationDirPath());
    const QString relativePath = appDir.relativeFilePath(path);
    if (!relativePath.startsWith("..") && !QDir::isAbsolutePath(relativePath))
        return QDir::toNativeSeparators(relativePath);
#endif

    return path;
}

#ifdef ATW_TEST_PORTABLE
static const QString defaultJvmArgs = QString();
#else
static const QString defaultJvmArgs = QStringLiteral(
    "-Xverify:none "
    "-Xss2M "
    "-Xmn1G "
    "-XX:+UnlockExperimentalVMOptions "
    "-XX:+UseG1GC "
    "-XX:MaxGCPauseMillis=40 "
    "-XX:+AlwaysActAsServerClassMachine "
    "-XX:MaxTenuringThreshold=1 "
    "-XX:SurvivorRatio=32 "
    "-XX:G1HeapRegionSize=8M "
    "-XX:G1MixedGCCountTarget=4 "
    "-XX:G1MixedGCLiveThresholdPercent=90 "
    "-XX:-UsePerfData "
    "-XX:+PerfDisableSharedMem "
    "-XX:+UseLargePages "
    "-XX:+AlwaysPreTouch "
    "-XX:+UseFastStosb "
    "-XX:+EliminateLocks "
    "-XX:+EnableJVMCIProduct "
    "-XX:+EnableJVMCI "
    "-XX:+UseJVMCICompiler "
    "-XX:+EagerJVMCI"
);
#endif

void Config::save() {
    QJsonObject saveObj;

    saveObj["version"] = gameVersion;
    saveObj["modLoader"] = modLoader;

    saveObj["keepMemorySame"] = keepMemorySame;
    saveObj["initialMemory"] = initialMemory;
    saveObj["maxMemory"] = maximumMemory;

    saveObj["useCustomJre"] = useCustomJre;
    saveObj["customJrePath"] = storePortablePath(customJrePath);

    saveObj["closeOnLaunch"] = closeOnLaunch;
    saveObj["autoLaunchOnOpen"] = autoLaunchOnOpen;

    saveObj["jvmArgs"] = jvmArgs;
    saveObj["javaOptimizationProfile"] = javaOptimizationProfile;
    saveObj["useLargePages"] = useLargePages;
    saveObj["showGpuReminder"] = showGpuReminder;

    saveObj["useCustomMinecraftDir"] = useCustomMinecraftDir;
    saveObj["customMinecraftDir"] = storePortablePath(customMinecraftDir);

    saveObj["joinServerOnLaunch"] = joinServerOnLaunch;
    saveObj["serverIp"] = serverIp;

    saveObj["useWeave"] = useWeave;

    saveObj["windowWidth"] = windowWidth;
    saveObj["windowHeight"] = windowHeight;

    QJsonArray arr;
    for(const Agent& agent : agents){
        QJsonObject agentObj;
        agentObj["path"] = storePortablePath(agent.path);
        agentObj["option"] = agent.option;
        agentObj["enabled"] = agent.enabled;

        arr.append(agentObj);
    }

    saveObj["agents"] = arr;

    QJsonArray arr2;
    foreach(const QString& str, helpers) {
        arr2.append(storePortablePath(str));
    }

    saveObj["helpers"] = arr2;
    saveJsonToConfig(saveObj);
}

Config Config::load() {
    QJsonObject jsonObj = loadJsonFromConfig();

    QJsonArray arr = jsonObj["agents"].toArray();

    QList<Agent> agents;

    for(auto val : arr){
        if(val.isObject()){
            QJsonObject obj = val.toObject();

            QString path = resolvePortablePath(obj["path"].toString());
            QString option = obj["option"].toString({});
            bool enabled = obj["enabled"].toBool(true);

            if(QFile::exists(path)){
                agents.append({path, option, enabled});
            }
        }else{
            QString path = resolvePortablePath(val.toString());
            agents.append({path, {}});
        }
    }

    arr.empty();
    arr = jsonObj["helpers"].toArray();

    QStringList helpers;

    foreach(const QJsonValue& val, arr) {
        QString path = resolvePortablePath(val.toString());
        if (QFile::exists(path)) {
            helpers.append(path);
        }
    }

    QDir modsDir(FS::getWeaveModsDirectory());
    if (!modsDir.exists())
        modsDir.mkpath(modsDir.path());

    QFileInfoList list = modsDir.entryInfoList({ "*.jar", "*.jar.disabled" }, QDir::Files, QDir::Name);

    QList<Mod> mods = QList<Mod>();
    for (const QFileInfo& file : list) {
        mods.append(Mod(file.fileName().remove(".jar.disabled").remove(".jar"), !file.completeSuffix().endsWith("jar.disabled")));
    }

    QString jvmArgs = jsonObj["jvmArgs"].toString(defaultJvmArgs).trimmed();
    if (jvmArgs.isEmpty())
        jvmArgs = defaultJvmArgs;

    return {
        jsonObj["version"].toString("1.8.9"),
        jsonObj["modLoader"].toString("Optifine"),
        jsonObj["keepMemorySame"].toBool(true),
        jsonObj["initialMemory"].toInt(3072),
        jsonObj["maxMemory"].toInt(3072),
        jsonObj["useCustomJre"].toBool(false),
        resolvePortablePath(jsonObj["customJrePath"].toString()),
        jvmArgs,
        jsonObj["javaOptimizationProfile"].toString(QStringLiteral("stable-g1")),
        jsonObj["useLargePages"].toBool(false),
        jsonObj["showGpuReminder"].toBool(true),
        jsonObj["closeOnLaunch"].toBool(false),
        jsonObj["autoLaunchOnOpen"].toBool(true),
        jsonObj["useCustomMinecraftDir"].toBool(false),
        resolvePortablePath(jsonObj["customMinecraftDir"].toString()),
        jsonObj["joinServerOnLaunch"].toBool(false),
        jsonObj["serverIp"].toString(),
        jsonObj["windowWidth"].toInt(640),
        jsonObj["windowHeight"].toInt(480),
        jsonObj["useWeave"].toBool(false),
        agents,
        helpers,
        mods
    };
}

void Config::saveJsonToConfig(const QJsonObject &jsonObject) {
    const QString filePath = configFilePath();
    QString path = QFileInfo(filePath).absolutePath();
    QDir dir;
    if(!dir.exists(path)){
        dir.mkpath(path);
    }

    QFile configFile(filePath);

    configFile.open(QIODevice::WriteOnly);

    configFile.write(QJsonDocument(jsonObject).toJson());

    configFile.close();
}

QJsonObject Config::loadJsonFromConfig() {
    QFile configFile(configFilePath());
    configFile.open(QIODevice::ReadOnly | QIODevice::Text);

    QJsonObject jsonObj = QJsonDocument::fromJson(configFile.readAll()).object();

    configFile.close();

    return jsonObj;
}

QString Config::configFilePath() {
#ifdef ATW_TEST_PORTABLE
    return FS::combinePaths(QApplication::applicationDirPath(), QStringLiteral("config"), QStringLiteral("settings.json"));
#else
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QStringLiteral("/atw-client/settings.json");
#endif
}
