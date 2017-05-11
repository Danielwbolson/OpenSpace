/*****************************************************************************************
*                                                                                       *
* OpenSpace                                                                             *
*                                                                                       *
* Copyright (c) 2014-2017                                                               *
*                                                                                       *
* Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
* software and associated documentation files (the "Software"), to deal in the Software *
* without restriction, including without limitation the rights to use, copy, modify,    *
* merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
* permit persons to whom the Software is furnished to do so, subject to the following   *
* conditions:                                                                           *
*                                                                                       *
* The above copyright notice and this permission notice shall be included in all copies *
* or substantial portions of the Software.                                              *
*                                                                                       *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
* INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
* PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
* CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
* OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
****************************************************************************************/

#include <modules/globebrowsing/models/roverpathfilereader.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/filesystem/filesystem.h>

#include <fstream>
#include <gdal_priv.h>
#include "ogrsf_frmts.h"

namespace {
	const std::string _loggerCat		= "RoverPathFileReader";
	const char* keyRoverLocationPath	= "RoverLocationPath";
	const char* keyModelPath			= "ModelPath";
}

namespace openspace {
namespace globebrowsing {
	
std::vector<Subsite> RoverPathFileReader::extractAllSubsites(const ghoul::Dictionary dictionary) {
	std::string roverLocationFilePath;
	if (!dictionary.getValue(keyRoverLocationPath, roverLocationFilePath))
		throw ghoul::RuntimeError(std::string(keyRoverLocationPath) + " must be specified!");

	std::fstream in(roverLocationFilePath.c_str());

	if (!in.is_open())
		throw ghoul::FileNotFoundError(roverLocationFilePath);

	GDALDataset *poDS;
	poDS = (GDALDataset*)GDALOpenEx(roverLocationFilePath.c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
	if (poDS == NULL) {
		LERROR("Could not open .shp file");
	}

	OGRLayer *poLayer = poDS->GetLayerByName("rover_locations");

	OGRFeature *poFeature;
	poLayer->ResetReading();

	int currentSite = 0;
	double siteLat;
	double siteLon;
	std::vector<Subsite> subsites;

	while ((poFeature = poLayer->GetNextFeature()) != NULL) {

		// Extract coordinates from OGR
		std::string frame = poFeature->GetFieldAsString("frame");
		std::string site = poFeature->GetFieldAsString("site");
		std::string drive = poFeature->GetFieldAsString("drive");
		double lat = poFeature->GetFieldAsDouble("plcl");
		double lon = poFeature->GetFieldAsDouble("longitude");

		// Saves all coordinates for rendering the path and only site coordinates for rendering sites.
		// GetFieldAsDouble resturns zero (0) if field is empty.
		if (lat != 0 && lon != 0) {
			if (frame == "SITE") {
				siteLat = lat;
				siteLon = lon;
			}

			std::string type = "site";
			Subsite subsite;
			subsite.site = convertString(site, type);
			type = "drive";
			subsite.drive = convertString(drive, type);
			subsite.lat = lat;
			subsite.lon = lon;
			subsite.frame = frame;
			subsite.siteLat = siteLat;
			subsite.siteLon = siteLon;

			// All features with the the frame is "Site" will have "Drive" that is -1. 
			// The feature right after each site frame has the same coordinates as the site frame.
			// E.g. feature 1: "Frame = SITE, Site = 6, Drive = -1, Lat = -4.7000, Lon = 137.4000"
			//		feature 2: "Frame = ROVER, Site = 6, Drive = 0, Lat = -4.7000, Lon = 137.4000"
			if(drive != "-1")
				subsites.push_back(subsite);
		}
		OGRFeature::DestroyFeature(poFeature);
	}
	GDALClose(poDS);

	return subsites;
}

std::vector<Subsite> RoverPathFileReader::extractSubsitesWithModels(const ghoul::Dictionary dictionary) {

	// Make sure the dictionary includes the necessary keys
	std::string roverLocationFilePath;
	if (!dictionary.getValue(keyRoverLocationPath, roverLocationFilePath))
		throw ghoul::RuntimeError(std::string(keyRoverLocationPath) + " must be specified!");

	std::string surfaceModelFilePath;
	if (!dictionary.getValue(keyModelPath, surfaceModelFilePath))
		throw ghoul::RuntimeError(std::string(keyModelPath) + " must be specified!");

	// Extract all subsites in the data set given the path to the file
	ghoul::Dictionary tempDictionary;
	tempDictionary.setValue(keyRoverLocationPath, roverLocationFilePath);
	std::vector<Subsite> allSubsites = extractAllSubsites(tempDictionary);

	std::vector<Subsite> subsitesWithModels;
	for (auto subsite : allSubsites) {
		std::string pathToDriveFolder;

		// Convert the site and drive string to match the folder structure
		std::string site = convertString(subsite.site, "site");
		std::string drive = convertString(subsite.drive, "drive");
		pathToDriveFolder = surfaceModelFilePath + "/level1/" + "site" + site + "/" + "drive" + drive;

		// If the folder exists it means there are models for this subsite, then check if that
		// specific site/drive combination has already been added. If the models haven't already been 
		// added, loop through the text file with file names and add those to the subsite.
		bool pathExists = FileSys.directoryExists(pathToDriveFolder);
		bool modelExists = false;
		if(pathExists) {
			for (auto controlSubsite : subsitesWithModels) {
				if (subsite.site == controlSubsite.site && subsite.drive == controlSubsite.drive) {
					modelExists = true;
					break;
				}
			}
			if(!modelExists) {
				std::string pathToFilenamesTextFile = pathToDriveFolder + "/filenames.txt";
				std::vector<std::string> fileNames = extractFileNames(pathToFilenamesTextFile);

				subsite.fileNames = fileNames;
				subsitesWithModels.push_back(subsite);
			}
		}
	}
	return subsitesWithModels;
}

std::string RoverPathFileReader::convertString(const std::string sitenr, const std::string type) {
	int k = std::stoi(sitenr);

	std::string temp;
	if (type == "site") {
		if (k < 10) {
			temp = "00" + std::to_string(k);
		}
		else if (k < 100) {
			temp = "0" + std::to_string(k);
		}
	}
	else if (type == "drive") {
		if (k < 10) {
			temp = "000" + std::to_string(k);
		}
		else if (k < 100) {
			temp = "00" + std::to_string(k);
		}
		else if (k < 1000) {
			temp = "0" + std::to_string(k);
		}
		else {
			temp = std::to_string(k);
		}
	}
	return temp;
}

std::vector<std::string> RoverPathFileReader::extractFileNames(const std::string filePath) {
	std::string path = absPath(filePath);
	std::ifstream myfile(path);

	std::string fileName;
	std::vector<std::string> fileNames;
	if (myfile.is_open()) {
		while (std::getline(myfile, fileName)) {
			fileNames.push_back(fileName);
		}
		myfile.close();
	}
	return fileNames;
}

} // namespace globebrowsing
} // namespace openspace
