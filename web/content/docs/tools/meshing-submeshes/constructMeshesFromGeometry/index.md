+++
date = "2018-06-19T15:56:57+01:00"
title = "Construct meshes from bulk mesh and geometries"
author = "Dmitri Naumov"
+++

## General

`constructMeshesFromGeometry` is a conversion tool for the `gml` files.  Given a
"bulk" mesh and the `gml` geometry file the tool generates meshes for all named
geometries.

The meshes are used for the boundary conditions and provide node ids and element
ids mappings to the bulk mesh.

## Example

For example a conversion of a mesh from `Tests/Data/LIE/Hydromechanics` single
fracture 3D project:

```bash
constructMeshesFromGeometry -m single_fracture_3D.vtu -g single_fracture_3D.gml

debug: Reading geometry file '../s/Tests/Data/LIE/HydroMechanics/single_fracture_3D.gml'.
debug: Project configuration from file '../s/Tests/Data/LIE/HydroMechanics/single_fracture_3D.gml' read.
debug: The search length for mesh "single_fracture_3D" is 1.000000e-09.
debug: Creating mesh from geometry single_fracture_3D inlet.
debug: Found 3 elements in the mesh
debug: Creating mesh from geometry single_fracture_3D outlet.
debug: Found 3 elements in the mesh
debug: Creating mesh from geometry single_fracture_3D left.
debug: Found 46 elements in the mesh
debug: Creating mesh from geometry single_fracture_3D right.
debug: Found 46 elements in the mesh
debug: Creating mesh from geometry single_fracture_3D front.
debug: Found 1150 elements in the mesh
debug: Creating mesh from geometry single_fracture_3D back.
debug: Found 1150 elements in the mesh
debug: Creating mesh from geometry single_fracture_3D bottom.
debug: Found 25 elements in the mesh
debug: Creating mesh from geometry single_fracture_3D top.
debug: Found 25 elements in the mesh
```

yields new meshes for each named geometry: `single_fracture_3D_inlet.vtu`,
`single_fracture_3D_right.vtu`, `single_fracture_3D_outlet.vtu`,
`single_fracture_3D_left.vtu`, `single_fracture_3D_front.vtu`,
`single_fracture_3D_top.vtu`, `single_fracture_3D_bottom.vtu`, and
`single_fracture_3D_back.vtu`.

![The front part of the mesh with the extracted geometries](single_fracture_3D_geometries.png "Shows the front part of the mesh with the extracted geometries. Note, the lower dimensional inlet boundary, shown as a line.")
