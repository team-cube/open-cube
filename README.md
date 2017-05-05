# Open Cube
Open Cube is a fork of Tesseract which is based on the Cube 2: Sauerbraten engine. 
Both Tesseract and Cube 2 use SVN which makes branching and merging awkward and does not allow for pull requests. By removing the binary data and putting the media folder in a submodule getting the source is faster.

The goal of Open Cube is to make mapping more fun by using modern dynamic rendering techniques, so
that you can get instant feedback on lighting changes, not just geometry.

Open Cube removes the static lightmapping system of Sauerbraten and replaces
it with completely dynamic lighting system based on deferred shading and
shadowmapping.

It provides a bunch of new rendering features such as:

* deferred shading
* omnidirectional point lights using cubemap shadowmaps
* perspective projection spotlight shadowmaps
* orthographic projection sunlight using cascaded shadowmaps
* HDR rendering with tonemapping and bloom
* real-time diffuse global illumination for sunlight (radiance hints)
* volumetric lighting
* screen-space ambient occlusion
* screen-space reflections and refractions for water and glass (use as many water planes as you want now!)
* screen-space refractive alpha cubes
* deferred MSAA, subpixel morphological anti-aliasing (SMAA 1x, T2x, S2x, and 4x), FXAA, and temporal AA
* runs on both OpenGL Core (3.0+) and legacy (2.1+) contexts
