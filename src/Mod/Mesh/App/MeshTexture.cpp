/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "MeshTexture.h"

using namespace Mesh;

MeshTexture::MeshTexture(const Mesh::MeshObject& mesh, const MeshCore::Material &material)
  : materialRefMesh(material)
{
    countPointsRefMesh = mesh.countPoints();
    unsigned long countFacets = mesh.countFacets();

    if (material.binding == MeshCore::MeshIO::PER_VERTEX && material.diffuseColor.size() == countPointsRefMesh) {
        binding = MeshCore::MeshIO::PER_VERTEX;
        kdTree.reset(new MeshCore::MeshKDTree(mesh.getKernel().GetPoints()));
    }
    else if (material.binding == MeshCore::MeshIO::PER_FACE && material.diffuseColor.size() == countFacets) {
        binding = MeshCore::MeshIO::PER_FACE;
        kdTree.reset(new MeshCore::MeshKDTree(mesh.getKernel().GetPoints()));
        refPnt2Fac.reset(new MeshCore::MeshRefPointToFacets(mesh.getKernel()));
    }
}

void MeshTexture::apply(const Mesh::MeshObject& mesh, MeshCore::Material &material)
{
    // copy the color values because the passed material could be the same instance as 'materialRefMesh'
    std::vector<App::Color> textureColor = materialRefMesh.diffuseColor;
    material.diffuseColor.clear();
    material.binding = MeshCore::MeshIO::OVERALL;

    if (kdTree.get()) {
        // the points of the current mesh
        std::vector<App::Color> diffuseColor;
        const MeshCore::MeshPointArray& points = mesh.getKernel().GetPoints();
        const MeshCore::MeshFacetArray& facets = mesh.getKernel().GetFacets();

        if (binding == MeshCore::MeshIO::PER_VERTEX) {
            diffuseColor.reserve(points.size());
            for (size_t index=0; index<points.size(); index++) {
                unsigned long pos = kdTree->FindExact(points[index]);
                if (pos < countPointsRefMesh) {
                    diffuseColor.push_back(textureColor[pos]);
                }
            }

            if (diffuseColor.size() == points.size()) {
                material.diffuseColor.swap(diffuseColor);
                material.binding = MeshCore::MeshIO::PER_VERTEX;
            }
        }
        else if (binding == MeshCore::MeshIO::PER_FACE) {
            // the values of the map give the point indices before the cut
            std::vector<unsigned long> pointMap;
            pointMap.reserve(points.size());
            for (size_t index=0; index<points.size(); index++) {
                unsigned long pos = kdTree->FindExact(points[index]);
                if (pos < countPointsRefMesh) {
                    pointMap.push_back(pos);
                }
            }

            // now determine the facet indices before the cut
            if (pointMap.size() == points.size()) {
                diffuseColor.reserve(facets.size());
                for (auto it : facets) {
                    std::vector<unsigned long> found = refPnt2Fac->GetIndices(pointMap[it._aulPoints[0]],
                                                                              pointMap[it._aulPoints[1]],
                                                                              pointMap[it._aulPoints[2]]);
                    if (found.size() == 1) {
                        diffuseColor.push_back(textureColor[found.front()]);
                    }
                }
            }

            if (diffuseColor.size() == facets.size()) {
                material.diffuseColor.swap(diffuseColor);
                material.binding = MeshCore::MeshIO::PER_FACE;
            }
        }
    }
}
