# r267
Simple Vulkan based Renderer

Now have some examples of using and viewer with using simple model format and converter to this format.

### Implemented: ###
* Simple GBuffer
* SSAO
* Normal Mapping and Parallax Occlusion Mapping
* Simple PBR Materils and Image Based Light

### Examples: ###
* Triangle Simple show triangle 
* Fullscreen quad - Show fullscreen quad with uv coords
* Texture   - Load texture and show it
* Mesh - Load and show mesh
* Converter - Loader models with using assimp and convert to inner format
* Compute   - Simple raytracing at Compute Shaders (Not finish yet)
* GUI       - Simple GUI example (GUI based at nuklear)
* Viewer    - Loader models with described effects

### Known Issues ### 
* ~~Doesn't correct free vulkan memory~~
* ~~Has problem in Compute demo (Old vertex format in shader)~~

### Dependence: ### 
* GLFW3 
* glm
* Vulkan
* assimp
* OpenImageIO
* Nuklear

![alt lambo](https://github.com/kurono267/r267/blob/master/images/lambo.png)
