+++
date = "2018-03-07T15:56:57+01:00"
title = "Create Boundary Condition in Polygonal Region"
author = "Thomas Fischer"
+++

## General

In order to create boundary conditions in a polygonal region the following workflow can be used:

1. Extract surface using [ExtractSurface]({{< ref "extract-surface" >}}) (a: Original subsurface mesh; b: Extracted surface)

    ![Sub-surface mesh](01-SubSurfaceMesh_web.png "Sub-surface mesh")

    ![Extracted surface](02-ExtractSurface_Web.png "Extracted surface")

2. Mark the mesh elements within the polygonal region deploying the tool [ResetPropertiesInPolygonalRegions]({{< ref "set-properties-in-polygonal-region" >}})

    ![Surface and polygon](03a-MarkedRegionsAtSurface_Web.png "Surface and polygon")

    ![Marked elements in polygonal region are colored yellow](03b-MarkedRegionsAtSurface_Web.png "Marked elements in polygonal region are colored yellow")

    ![Marked regions visualized by different colors](03c-MarkedRegionsAtSurface_Web.png "Marked regions visualized by different colors")

3. Remove marked/unmarked mesh elements deploying tool [removeMeshElements]({{< ref "remove-mesh-elements" >}})

    ![Resulting patches visualized by different colors and z-translations](04-ExtractedRegionPatches_Web.png "Resulting patches visualized by different colors and z-translations")

4. Compute the associated area for the nodes of the surface mesh deploying tool [ComputeNodeAreasFromSurfaceMesh]({{< ref "compute-node-areas-from-surface-mesh" >}})

The surface mesh patches (created until step 3) can be used as input for OGS-6 simulations. For OGS-5 simulations only the additional step 4 has to be performed.
