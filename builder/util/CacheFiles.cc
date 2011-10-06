// Copyright 2011 Shannon Weyrick <weyrick@mozek.us>

#include "CacheFiles.h"
#include "builder/BuilderOptions.h"

using namespace model;
using namespace std;

namespace builder {

string getCacheFilePath(Context &context,
                        const BuilderOptions *options,
                        const string &canonicalName,
                        const string &ext) {

    string path;
/*
    // look for an explicit cache path
    BuilderOptions::StringMap::const_iterator i = options->optionMap.find("cachePath");
    if (i != options->optionMap.end()) {
        // explicit path
        path = i->second;
    }

*/

    // XXX maybe pull out ModulePath from construct for use

    // XXX add in opt level?

    path = "/tmp/"+canonicalName+"."+ext;
    return path;

}

}
