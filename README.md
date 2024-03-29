# ogl_render
OpenGL rendering engine

This OpenGL renderer written as a learning project.
I also made a scene to demo the features: https://www.artstation.com/artwork/rRoV5E

## Required Libs
  - GLEW
  - FreeGLUT
  - stb_image single header lib
  - ISPCTextureCompressor

## Details  
Much of what I learned came from: https://learnopengl.com/. Such a fantastic resource!
  
The renderer is a Forward+ Tile Renderer ( https://takahiroharada.files.wordpress.com/2015/04/forward_plus.pdf ), a rendering approach that partitions the screen into small 2D tiles. Lists of lights are associated with each tile in a “DepthPrepass”. Rather than “binning” a light in a tile by using its proximity to said tile, I instead have a LightMesh (a simple triangle mesh whose volume contains all space that could be illuminated by the light) whose triangles are rasterized directly into the tiles using a compute shader. Here was my main resource for learning how rasterization works: https://www.gabrielgambetta.com/computer-graphics-from-scratch/rasterization.html. Because of this approach, rendering things like Spotlights becomes much more efficient because you don’t have to spend time calculating the lighting for fragments outside of the Spotlight's cone of influence.
  
The renderer supports multiple types of lights: Directional, Spot, and Point. Each of these use a physically accurate model for light attenuation. This ended up being a mistake because it made the lights influence WAY too large. I should have just come up with some Gaussian curve or something. All light types support ShadowMaps. These ShadowMaps are stored within a shared ShadowAtlas which dynamically updates to fit the appropriate number of ShadowMap partitions at the maximum resolution. A Pointlight takes up 6 of these partitions because they are effectively cubemaps. I use simple PCF filtering to smooth out the shadows.

For the BRDF, I chose to use the same PBR model used in TheOrder1886 (https://blog.selfshadow.com/publications/s2013-shading-course/rad/s2013_pbs_rad_slides.pdf ). For environment reflections, I use a method of IBL (ImageBasedLighting) where the scene is captured as a cubemap from a point in space. The cubemap is convolved to generate an IrradianceMap (diffuse environment reflections), and an EnvironmentMap whose mips are used to reflect the environment at different roughness’s. Here was my main resource for PBR and IBL: https://learnopengl.com/PBR/Theory. Incidentally, this is the same approach used by UE4 where they pre-compute both the diffuse/specular reflections and the specularBRDF as a lookup table. These two components are combined to approximate the same PBR BRDF used to shade the models themselves. This approach is called the “split-sum approximation”.
  
For PostProcessing, I implemented simple Bloom and SSAO passes that are added in after the scene is rendered. The SSAO is calculated by reconstructing the ViewPos of a fragment from the DepthBuffer. Then the image is tonemapped down to an LDR space and lastly, I used a 3D LUT to tweak the final colors a bit. I also utilize some low hanging fruit like MSAA and Anisotropic Filtering. However, I realized I was severely limiting image quality by not implementing a post process version of anti-aliasing (like FXAA or TSAA) and generating custom NormalMap mips to deal with specular aliasing.
