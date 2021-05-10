/* Copyright (C) 2021 chipmunk-sm <dannico@linuxmail.org> */

#ifndef FILESELECTORLOCATIONS_H
#define FILESELECTORLOCATIONS_H

#include "fileselectorlocationsenum.h"

#include <vector>

class FileSelectorLocations
{
public:
    static void getRootPathList(std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList, bool skipAppFolder);
    static QStringList AddRecentLocations(const QString & path, bool clear);
    static void ClearRecentLocations();
    static void GetRecentLocations (std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList);
private:
    static void GetAndroidLocations(eLocations locationId, std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList);
    static void GetLocations       (eLocations locationId, std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList, bool skipAppFolder);
    static void GetRootLocations   (eLocations locationId, std::vector<std::pair<eLocations, std::vector<QString>>> &locationsList);
};

#endif // FILESELECTORLOCATIONS_H
