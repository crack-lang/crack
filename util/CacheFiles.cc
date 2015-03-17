// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>
// Copyright 2012 Google Inc.
//
//   This Source Code Form is subject to the terms of the Mozilla Public
//   License, v. 2.0. If a copy of the MPL was not distributed with this
//   file, You can obtain one at http://mozilla.org/MPL/2.0/.
//

#include "CacheFiles.h"

#include "builder/BuilderOptions.h"
#include "spug/StringFmt.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <libgen.h>

using namespace model;
using namespace std;
using namespace builder;

namespace crack { namespace util {

namespace {
    // recursive mkdir - i.e. ensure the path exists and is readable, and if
    // necessary create the path and all parent paths
    // returns true if the path can be used successfully
    // originally based on idea from
    // http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
    bool r_mkdir(const char *path) {

            char opath[PATH_MAX];
            char *p;
            size_t len;

            if (access(path, R_OK|W_OK) == 0)
                return true;

            strncpy(opath, path, sizeof(opath));
            len = strlen(opath);
            if (opath[len - 1] == '/')
                    opath[len - 1] = '\0';
            p = opath;
            if (*p == '/')
                p++;
            for (; *p; p++) {
                if (*p == '/') {
                    *p = '\0';
                    if (access(opath, F_OK)) {
                        if (mkdir(opath, S_IRWXU))
                            return false;
                    };
                    *p = '/';
                }
            }
            if (access(opath, F_OK))
                mkdir(opath, S_IRWXU);

            return (access(opath, R_OK|W_OK) == 0);

    }
}

bool initCacheDirectory(BuilderOptions *options, Construct &construct) {

    string path;

    // if the user didn't specify a cache directory, use the default
    BuilderOptions::StringMap::const_iterator i =
            options->optionMap.find("cachePath");
    if (i != options->optionMap.end()) {
        // explicit path
        path = i->second;
    }

    if (path.empty()) {
        // default path is home dir
        // XXX maybe pull out ModulePath from construct for use?
        path = getenv("HOME");
        if (path.empty()) {
            if (options->verbosity)
                cerr << "unable to set default cache path, caching disabled\n";
            construct.cacheMode = false;
            return false;
        }
        if (path.at(path.size()-1) != '/')
            path.push_back('/');
        path.append(".crack/cache/");
    }

    // at this point we have some string they specified on the command line
    // OR the value of the HOME env variable
    if (path.at(path.size()-1) != '/')
        path.push_back('/');

    if (r_mkdir(path.c_str()))
        options->optionMap["cachePath"] = path;
    else {
        if (options->verbosity)
            cerr << "no access to create/use cache path, caching disabled: " <<
                    path << "\n";
        construct.cacheMode = false;
        return false;
    }

    return true;
}

string getCacheFilePath(BuilderOptions* options,
                        Construct &construct,
                        const std::string &canonicalName,
                        const std::string &destExt
                        ) {

    // at this point, initCacheDirectory should have ensured we have a path
    BuilderOptions::StringMap::const_iterator i =
            options->optionMap.find("cachePath");

    // if we didn't find the cache path, try initializing the cache directory
    // before failing.
    if (i == options->optionMap.end()) {
        if (!initCacheDirectory(options, construct))
            return string();
        else
            i = options->optionMap.find("cachePath");
    }

    return SPUG_FSTR(i->second << "/" << canonicalName << '.' << destExt);

#if 0
    // get the canonicalized path - if that doesn't work, we have to assume
    // that this is a path to a nonexistent file.
    char rpath[PATH_MAX + 1];
    if (!realpath(path.c_str(), rpath))
        strcpy(rpath, path.c_str());
    assert(*rpath == '/' && "not absolute");

    char *rpathd = strdup(rpath);
    char *rpathf = strdup(rpath);
    assert(rpathd); assert(rpathf);
    char *rpathdir = dirname(rpathd);
    char *rpathfile = basename(rpathf);

    finalPath.append(rpathdir+1);
    finalPath.push_back('/');

    if (!r_mkdir(finalPath.c_str())) {
        if (options->verbosity)
            cerr << "unable to create cache path for: " <<
                    finalPath << "\n";
        return NULL;
    }

    finalPath.append(rpathfile);
    finalPath.push_back('.');
    finalPath.append(destExt);

    free(rpathd);
    free(rpathf);

    // XXX add in opt level?

    return finalPath;
#endif
}

bool move(const std::string &src, const std::string &dst) {
    unlink(dst.c_str());
    int result = link(src.c_str(), dst.c_str());
    unlink(src.c_str());
    return result ? false : true;
}

}} // namespace crack::util
