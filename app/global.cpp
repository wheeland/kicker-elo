#include "global.hpp"

#include <QByteArray>
#include <QDebug>

const char * const ENV_INTERNAL_PATH = "ELO_USE_INTERNAL_PATHS";
const char * const ENV_DEPLOY_PREFIX = "ELO_DEPLOY_PREFIX";
const char * const ENV_DB_PATH = "ELO_DB_PATH";
const char * const ENV_LOG_PATH = "ELO_LOG_PATH";

static bool checkUseInternalPaths()
{
    const QByteArray dbPath = qgetenv(ENV_INTERNAL_PATH);

    // default to false
    if (dbPath.isEmpty())
        return false;

    if (dbPath != "0" && dbPath != "1") {
        qCritical() << "Invalid value for" << ENV_INTERNAL_PATH;
        return false;
    }

    return (dbPath == "1");
}

bool useInternalPaths()
{
    static bool use = checkUseInternalPaths();
    return use;
}

const std::string &deployPrefix()
{
    static const std::string s_deployPrefix(qgetenv(ENV_DEPLOY_PREFIX).data());
    static const std::string s_empty;
    return useInternalPaths() ? s_empty : s_deployPrefix;
}
