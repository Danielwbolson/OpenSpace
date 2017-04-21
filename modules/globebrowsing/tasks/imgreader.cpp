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

#include <modules/globebrowsing/tasks/imgreader.h>

#include <ghoul/glm.h>

#include <fstream>
#include <iterator>
#include <algorithm>
#include <sstream>
template<typename Out>
static void split(const std::string &s, char delim, Out result) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		*(result++) = item;
	}
}


static std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}

namespace openspace {
namespace globebrowsing {
	ImgReader::PointCloudInfo ImgReader::readBinaryHeader(const std::string filename) {
		std::ifstream header(filename);

		PointCloudInfo mInfo;
		char delimiter = '=';

		std::string line;
		std::string block = "";
		glm::dvec3 originOffset;

		//TODO: Use openinventor for this part
		//Reading header part of binary file
		while (std::getline(header, line)) {
			line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
			if (line.length() == 0 || line.find(delimiter) == std::string::npos) continue;
			if (line == "END") break; //End of "header"
			std::vector<std::string> s = split(line, delimiter);

			if (s.at(0) == "OBJECT" && s.at(1) == "IMAGE") block = "IMAGE";
			else if (s.at(0) == "END_OBJECT" && s.at(1) == "IMAGE") block = "";


			if (s.at(0) == "OBJECT" && s.at(1) == "IMAGE_HEADER") block = "IMAGE_HEADER";
			else if (s.at(0) == "END_OBJECT" && s.at(1) == "IMAGE_HEADER") block = "";


			if (s.at(0) == "GROUP" && s.at(1) == "ROVER_COORDINATE_SYSTEM") block = "ROVER_COORDINATE_SYSTEM";
			else if (s.at(0) == "END_GROUP" && s.at(1) == "ROVER_COORDINATE_SYSTEM") block = "";

			if (block == "IMAGE_HEADER") {
				if (s.at(0) == "BYTES") mInfo._bytes = std::stoi(s.at(1));
			}
			else if (block == "ROVER_COORDINATE_SYSTEM") {
				if (s.at(0) == "ORIGIN_ROTATION_QUATERNION") {
					std::vector<std::string> temp = split(s.at(1), ',');

					mInfo._roverOrigin.push_back(std::stod(split(temp.at(0), '(').at(1)));
					mInfo._roverOrigin.push_back(std::stod(temp.at(1)));
					mInfo._roverOrigin.push_back(std::stod(temp.at(2)));

					//This is because after like 1000 sols they 
					//start writing the fourth quaternion on new line...
					//All of this should probably be done in another way...
					if (temp.size() == 3) {
						mInfo._roverOrigin.push_back(std::stod(split(temp.at(3), ')').at(0)));
					}
					else {
						std::getline(header, line);
						line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
						line.erase(std::remove(line.begin(), line.end(), ','), line.end());
						//						mInfo._roverOrigin.push_back(std::stod(line));
					}


				}
			}
			else if (block == "IMAGE") {
				if (s.at(0) == "LINES") {
					mInfo._lines = std::stoi(s.at(1));
				}
				else if (s.at(0) == "LINE_SAMPLES") {
					mInfo._cols = std::stoi(s.at(1));
				}
				else if (s.at(0) == "BANDS") {
					mInfo._bands = std::stoi(s.at(1));
				}
			}
		}

		header.close();

		return mInfo;

	}

	void ImgReader::readBinaryData(const std::string filename, std::vector<std::vector<float>> &xyz, ImgReader::PointCloudInfo pci) {
		unsigned char bytes[4];
		FILE *fileID = fopen(filename.c_str(), "rb");
		bool firstIsFound = false;
		float f;

		// Reading the header until the image data is found
		while (!firstIsFound && fread((void*)(&f), sizeof(f), 1, fileID)) {
			float cf;
			char *floatToConvert = (char*)& f;
			char *floatConverted = (char*)& cf;

			// Read as big endian
			floatConverted[0] = floatToConvert[3];
			floatConverted[1] = floatToConvert[2];
			floatConverted[2] = floatToConvert[1];
			floatConverted[3] = floatToConvert[0];

			if (cf == 0.0) {
				// According to the SIS-pdf, the first data value is a zero
				firstIsFound = true;
			}
		}

		// Iterate over all bands
		for (int band = 0; band < pci._bands; ++band) {
			std::vector<float> lines;
			xyz.push_back(lines);
			// Iterate over all pixels
			for (int j = 0; j < pci._cols; ++j) {
				for (int k = 0; k < pci._lines; ++k) {
					float f;
					fread((void*)(&f), sizeof(f), 1, fileID);

					float cf;
					char *floatToConvert = (char*)& f;
					char *floatConverted = (char*)& cf;

					floatConverted[0] = floatToConvert[3];
					floatConverted[1] = floatToConvert[2];
					floatConverted[2] = floatToConvert[1];
					floatConverted[3] = floatToConvert[0];

					xyz.at(band).push_back(cf);
				}
			}
		}

		fclose(fileID);
	}
} //namespace globebrowsing
} //namespace openspace