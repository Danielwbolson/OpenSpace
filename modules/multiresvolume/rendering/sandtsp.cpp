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

#include <modules/multiresvolume/rendering/sandtsp.h>

// ghoul
#include <ghoul/filesystem/filesystem.h>
#include <ghoul/filesystem/cachemanager.h>
#include <ghoul/logging/logmanager.h>
#include <ghoul/glm.h>

// boost
#include <boost/iostreams/device/mapped_file.hpp>

// std
#include <algorithm>
#include <math.h>
#include <queue>

# include <chrono>

namespace {
    const std::string _loggerCat = "SandTSP";
}

namespace openspace {

SandTSP::SandTSP(const std::string& filename)
    : TSP(filename) { }

SandTSP::~SandTSP() {
    if (_file.is_open())
        _file.close();
}

bool SandTSP::construct() {
    LDEBUG("Constructing TSP...");
    return TSP::construct();
}

bool SandTSP::calculateSpatialError() {
    const unsigned int numBrickVals = paddedBrickDim_*paddedBrickDim_*paddedBrickDim_;

    boost::iostreams::mapped_file_source mfile;
    mfile.open(_filename);

    if (!mfile.is_open()) {
        return false;
    }

    const float * voxelData = (float *)mfile.data();
    const long long headerOffset = dataPosition() / sizeof(float);

    std::vector<float> averages(numTotalNodes_);
    std::vector<float> stdDevs(numTotalNodes_);

    // First pass: Calculate average color for each brick
    LDEBUG("Calculating spatial error, first pass");
    for (size_t brick = 0; brick<numTotalNodes_; ++brick) {
        // Offset in file
        const auto brickStart = headerOffset + static_cast<long long>(brick*numBrickVals);
        double average = 0.0;

        for (size_t i = 0; i < numBrickVals; i++) {
            average += voxelData[brickStart + i];
        }
        averages[brick] = average / static_cast<double>(numBrickVals);
    }

    // Spatial SNR stats
    float minError = 1e20f;
    float maxError = 0.f;
    std::vector<float> medianArray(numTotalNodes_);

    // Second pass: For each brick, compare the covered leaf voxels with
    // the brick average
    LDEBUG("Calculating spatial error, second pass");
    for (size_t brick = 0; brick<numTotalNodes_; ++brick) {

        // Fetch mean intensity 
        float brickAvg = averages[brick];

        // Sum  for std dev computation
        float stdDev = 0.f;

        // Get a list of leaf bricks that the current brick covers
        std::list<unsigned int> coveredLeafBricks =
            CoveredLeafBricks(brick);

        // If the brick is already a leaf, assign a negative error.
        // Ad hoc "hack" to distinguish leafs from other nodes that happens
        // to get a zero error due to rounding errors or other reasons.
        if (coveredLeafBricks.size() == 1) {
            stdDev = -0.1f;
        }
        else {

            // Calculate "standard deviation" corresponding to leaves
            for (auto lb = coveredLeafBricks.begin(); lb != coveredLeafBricks.end(); ++lb) {

                // Add to sum
                const auto leafStart = headerOffset + static_cast<long long>((*lb)*numBrickVals);

                for (size_t i = 0; i < numBrickVals; i++) {
                    stdDev += pow(voxelData[leafStart + i] - brickAvg, 2.f);
                }
            }

            // Finish calculation
            if (sizeof(float) != sizeof(int)) {
                LERROR("Float and int sizes don't match, can't reintepret");
                return false;
            }

            stdDev /= static_cast<float>(coveredLeafBricks.size()*numBrickVals);
            stdDev = sqrt(stdDev);

        } // if not leaf

        if (stdDev < minError) {
            minError = stdDev;
        }
        else if (stdDev > maxError) {
            maxError = stdDev;
        }

        stdDevs[brick] = stdDev;
        medianArray[brick] = stdDev;

    }

    // Close the memory map
    mfile.close();

    std::sort(medianArray.begin(), medianArray.end());

    // "Normalize" errors
    float minNorm = 1e20f;
    float maxNorm = 0.f;
    for (unsigned int i = 0; i<numTotalNodes_; ++i) {

        if (stdDevs[i] > 0.f) {
            stdDevs[i] = pow(stdDevs[i], 0.5f);
        }

        data_[i*NUM_DATA + SPATIAL_ERR] = glm::floatBitsToInt(stdDevs[i]);
        if (stdDevs[i] < minNorm && 0 <= stdDevs[i]) {
            minNorm = stdDevs[i];
        }
        if (stdDevs[i] > maxNorm && 0 <= stdDevs[i]) {
            maxNorm = stdDevs[i];
        }
    }

    std::sort(stdDevs.begin(), stdDevs.end());
    float medNorm = stdDevs[stdDevs.size() / 2];

    minSpatialError_ = minNorm;
    maxSpatialError_ = maxNorm;
    medianSpatialError_ = medNorm;

    LDEBUG("Min normalized spatial std dev: " << minNorm);
    LDEBUG("Max normalized spatial std dev: " << maxNorm);
    LDEBUG("Median normalized spatial std dev: " << medNorm);

    return true;
}

bool SandTSP::calculateTemporalError() {

    boost::iostreams::mapped_file_source mfile;
    mfile.open(_filename);

    if (!mfile.is_open()) {
        return false;
    }

    const float * voxelData = (float *)mfile.data();
    const long long headerOffset = dataPosition() / sizeof(float);

    LDEBUG("Calculating temporal error");

    // generateLeafCoverages();
    // Statistics
    std::vector<float> meanArray(numTotalNodes_);

    // Save errors
    std::vector<float> errors(numTotalNodes_);

    const unsigned int numBrickVals =
        paddedBrickDim_*paddedBrickDim_*paddedBrickDim_;

    // Calculate temporal error for one brick at a time
    for (unsigned int brick = 0; brick<numTotalNodes_; ++brick) {

        // Save the individual voxel's average over timesteps. Because the
        // BSTs are built by averaging leaf nodes, we only need to sample
        // the brick at the correct coordinate.
        std::vector<float> voxelAverages(numBrickVals);

        // Read the whole brick to fill the averages
        const auto brickStart = headerOffset + static_cast<long long>(brick*numBrickVals);

        // Build a list of the BST leaf bricks (within the same octree level) that
        // this brick covers

        std::list<unsigned int> coveredBricks = CoveredBSTLeafBricks(brick);

        // If the brick is at the lowest BST level, automatically set the error 
        // to -0.1 (enables using -1 as a marker for "no error accepted");
        // Somewhat ad hoc to get around the fact that the error could be
        // 0.0 higher up in the tree
        if (coveredBricks.size() == 1) {
            errors[brick] = -0.1f;
            continue;
        } // done: move to next iteration

        // Calculate standard deviation per voxel, average over brick
        float avgStdDev = 0.f;  
        for (size_t voxel = 0; voxel< numBrickVals; ++voxel) {
            float stdDev = 0.f;
            for (auto leaf = coveredBricks.begin(); leaf != coveredBricks.end(); ++leaf) {
                // Sample the leaves at the corresponding voxel position
                const auto leafOffset = headerOffset + static_cast<long long>(*leaf*numBrickVals + voxel);

                const float sample = voxelData[leafOffset];
                stdDev += pow(sample - voxelData[brickStart + voxel], 2.f);
            }
            stdDev /= static_cast<float>(coveredBricks.size());
            stdDev = sqrt(stdDev);

            avgStdDev += stdDev;

        } // for voxel

        avgStdDev /= static_cast<float>(numBrickVals);
        meanArray[brick] = avgStdDev;
        errors[brick] = avgStdDev;

    } // for all bricks
    mfile.close();

    std::sort(meanArray.begin(), meanArray.end());

    // Adjust errors using user-provided exponents
    float minNorm = 1e20f;
    float maxNorm = 0.f;
    for (unsigned int i = 0; i<numTotalNodes_; ++i) {
        if (errors[i] > 0.f) {
            errors[i] = pow(errors[i], 0.25f);
        }

        data_[i*NUM_DATA + TEMPORAL_ERR] = glm::floatBitsToInt(errors[i]);
        if (errors[i] < minNorm && 0 <= errors[i]) {
            minNorm = errors[i];
        }
        if (errors[i] > maxNorm && 0 <= errors[i]) {
            maxNorm = errors[i];
        }
    }

    std::sort(errors.begin(), errors.end());
    float medNorm = errors[errors.size() / 2];

    minTemporalError_ = minNorm;
    maxTemporalError_ = maxNorm;
    medianTemporalError_ = medNorm;

    LDEBUG("Min normalized temporal std dev: " << minNorm);
    LDEBUG("Max normalized temporal std dev: " << maxNorm);
    LDEBUG("Median normalized temporal std dev: " << medNorm);

    return true;
}

}
