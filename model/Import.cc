#include "Import.h"

#include "Deserializer.h"
#include "Serializer.h"

using namespace model;
using namespace std;

void Import::serialize(Serializer &serializer) const {
    serializer.write(moduleName.size(), "#moduleName");
    for (std::vector<std::string>::const_iterator iter = moduleName.begin();
         iter != moduleName.end();
         ++iter
         )
        serializer.write(*iter, "module");

    serializer.write(syms.size(), "#importedSym");
    for (ImportedDefVec::const_iterator iter = syms.begin();
         iter != syms.end();
         ++iter
         )
        iter->serialize(serializer);

    serializer.write(0, "optional");
}

ImportPtr Import::deserialize(Deserializer &deser) {
    vector<string> moduleName;
    ImportedDefVec syms;

    int count = deser.readUInt("#moduleName");
    moduleName.reserve(count);
    while (count--)
        moduleName.push_back(deser.readString(Serializer::varNameSize,
                                              "moduleName"
                                              )
                             );

    count = deser.readUInt("#importedSym");
    while (count--)
        syms.push_back(ImportedDef::deserialize(deser));

    deser.readString(64, "optional");
    return new Import(moduleName, syms);
}
