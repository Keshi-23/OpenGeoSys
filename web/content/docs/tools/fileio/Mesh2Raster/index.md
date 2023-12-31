+++
date = "2019-12-03T00:00:00+01:00"
title = "Mesh2Raster"
author = "Karsten Rink"
+++

## Introduction

This utility generates an ASCII-raster file based on a 2D surface mesh. A raster is superimposed on the mesh and pixel values are set to the surface elevation at each pixel's position. If no mesh element is located beneath a pixel it's value is set to NODATA.

## Usage

```bash
   Mesh2Raster  -i <string> -o <string> [-c <real>]


Where:

   -i <string>,  --input-file <string>
     (required)  Mesh input file (*.vtu, *.msh)

   -o <string>,  --output-file <string>
     (required)  Raster output file (*.asc)

   -c <real>,  --cellsize <real>
     edge length of raster cells in result
```

The parameter ```c``` specifies the cell size (i.e. pixel size) of the raster. While optional, it is still recommended to choose a value as the default will be set to the minimum edge length in the input mesh which for unstructured grids may result in a very fine and (extremely) large output raster.

## Simple example

**Input data:**

![2D surface mesh.](Mesh2Raster-input.png#two-third "2D surface mesh.")

The input mesh for this example is a homogeneous, unstructured triangle mesh with an average edge length of 100m.

**Command:**

```bash
Mesh2Raster -i input.vtu -o output.asc -c 50
```

![output50](Mesh2Raster-output50.png#two-third "The generated output raster has a size of 340x333 pixels and represents the original surface well. Given the average edge length of 100m in the original mesh, an even smaller cellsize would not have contained more details but resulted in a larger file size. Conversely, a larger cellsize might result in artefacts due to undersampling, see [Nyquist criterion](https://en.wikipedia.org/wiki/Nyquist_rate).")

**Command:**

```bash
Mesh2Raster -i input.vtu -o output.asc -c 200
```

![output200](Mesh2Raster-output200.png#two-third "The generated output raster has a size of 85x84 pixels and still represents the original surface reasonably well, despite visible undersampling.")

**Command:**

```bash
Mesh2Raster -i input.vtu -o output.asc -c 1000
```

![output1000](Mesh2Raster-output1000.png#two-third "The generated output raster has a size of 17x17 pixels and shows severy undersampling. However, this is the resolution that a large number of weather data products are available at.")

## Application

The resulting ASCII-rasters can be used to represent surface data in geographic information systems. In the absence of input data, a detailed raster can also be used by OpenGeoSys preprocessing tools to generate new surface meshes with different resolution or properties.
