/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2016                                                               *
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

#ifndef __CHUNK_H__
#define __CHUNK_H__

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <ostream>

#include <modules/globebrowsing/rendering/culling.h>

#include <modules/globebrowsing/globes/chunkindex.h>
#include <modules/globebrowsing/geodetics/geodetic2.h>


namespace openspace {

    class ChunkLodGlobe;


    class Chunk {
    public:

        enum Status{
            DO_NOTHING,
            WANT_MERGE,
            WANT_SPLIT,
        };
        
        Chunk(ChunkLodGlobe* owner, const ChunkIndex& chunkIndex);

        /// Updates chunk internally and returns a desired level
        Status update(const RenderData& data);

        const GeodeticPatch& surfacePatch() const;
        ChunkLodGlobe* const owner() const;
        const ChunkIndex index() const;
        bool isVisible() const;

        void setIndex(const ChunkIndex& index);
        void setOwner(ChunkLodGlobe* newOwner);


    private:
        ChunkLodGlobe* _owner;
        ChunkIndex _index;
        bool _isVisible;
        GeodeticPatch _surfacePatch;

    };

}



#endif // __CHUNK_H__
