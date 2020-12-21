#pragma once

#include <string>

extern const char * const ENV_INTERNAL_PATH;
extern const char * const ENV_DEPLOY_PREFIX;
extern const char * const ENV_DB_PATH_BERLIN;
extern const char * const ENV_DB_PATH_GERMANY;
extern const char * const ENV_LOG_PATH;

bool useInternalPaths();
const std::string &deployPrefix();
