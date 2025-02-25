#include "NetworkModAPI.h"

#include "ui/pages/modplatform/ModModel.h"

#include "Application.h"
#include "net/NetJob.h"

void NetworkModAPI::searchMods(CallerType* caller, SearchArgs&& args) const
{
    auto netJob = new NetJob(QString("%1::Search").arg(caller->debugName()), APPLICATION->network());
    auto searchUrl = getModSearchURL(args);

    auto response = new QByteArray();
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), response));

    QObject::connect(netJob, &NetJob::started, caller, [caller, netJob] { caller->setActiveJob(netJob); });
    QObject::connect(netJob, &NetJob::failed, caller, &CallerType::searchRequestFailed);
    QObject::connect(netJob, &NetJob::succeeded, caller, [caller, response] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << caller->debugName() << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        caller->searchRequestFinished(doc);
    });

    netJob->start();
}

void NetworkModAPI::getModInfo(CallerType* caller, ModPlatform::IndexedPack& pack)
{
    auto id_str = pack.addonId.toString();
    auto netJob = new NetJob(QString("%1::ModInfo").arg(id_str), APPLICATION->network());
    auto searchUrl = getModInfoURL(id_str);

    auto response = new QByteArray();
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), response));

    QObject::connect(netJob, &NetJob::succeeded, [response, &pack, caller] {
        QJsonParseError parse_error{};
        auto doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response for " << pack.name << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        caller->infoRequestFinished(doc, pack);
    });

    netJob->start();
}

void NetworkModAPI::getVersions(CallerType* caller, VersionSearchArgs&& args) const
{
    auto netJob = new NetJob(QString("%1::ModVersions(%2)").arg(caller->debugName()).arg(args.addonId), APPLICATION->network());
    auto response = new QByteArray();

    netJob->addNetAction(Net::Download::makeByteArray(getVersionsURL(args), response));

    QObject::connect(netJob, &NetJob::succeeded, caller, [response, caller, args] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from " << caller->debugName() << " at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            return;
        }

        caller->versionRequestSucceeded(doc, args.addonId);
    });

    QObject::connect(netJob, &NetJob::finished, caller, [response, netJob] {
        netJob->deleteLater();
        delete response;
    });

    netJob->start();
}
