
#include "lvgl/lvgl.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "lvgl/src/drivers/glfw/lv_opengles_debug.h" /* GL_CALL */

#include "ibl_sampler.h"
#include "shader_cache.h"

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>  /* floor */


/*
#ifndef STB_HAS_BEEN_INCLUDED
#define STB_HAS_BEEN_INCLUDED
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#pragma GCC diagnostic pop
#endif
*/
#include "stb_image/stb_image.h"

void (*callback)(const char *, float, float);

iblSampler::iblSampler(void) {
    textureSize = 256;
    //ggxSampleCount = 1024;
    ggxSampleCount = 256;
    //lambertianSampleCount = 2048;
    lambertianSampleCount = 512;
    sheenSamplCount = 64;
    lodBias = 0.0;
    lowestMipLevel = 4;
    lutResolution = 1024;

    scaleValue = 1.0;

    mipmapCount = GL_NONE;
    mipmapLevels = 0;

    lambertianTextureID = GL_NONE;
    ggxTextureID = GL_NONE;
    sheenTextureID = GL_NONE;
    ggxLutTextureID = GL_NONE;
    charlieLutTextureID = GL_NONE;
    inputTextureID = GL_NONE;
    cubemapTextureID = GL_NONE;
    framebuffer = GL_NONE;

    _shaderCache = ShaderCache(env_src_includes, sizeof(env_src_includes)/sizeof(key_value), NULL, NULL );
    shaderCache = &_shaderCache;

    callback = NULL;
}

unsigned int iblSampler::internalFormat(void)
{
    return GL_RGBA8; 
}

unsigned int iblSampler::textureTargetType(void)
{
    return GL_UNSIGNED_BYTE;
}

iblSampler::t_texture iblSampler::prepareTextureData(t_image* image)
{
    t_texture texture = { GL_RGB32F, GL_RGB, GL_FLOAT, NULL };
    // Reset scaling of hdrs 
    scaleValue = 1.0;

    {
        texture.internalFormat = internalFormat();
        texture.format = GL_RGBA;
        texture.type =  GL_UNSIGNED_BYTE;

        unsigned long int numPixels = image->dataFloatLength / 3;

        float max_value = 0.0;
        float clamped_sum = 0.0;
        float diff_sum = 0.0;
        unsigned int src = 0;
        unsigned int dst = 0;
        for(unsigned int i = 0; i < numPixels; i++) {
            float max_component = std::max(*(image->dataFloat + src + 0), *(image->dataFloat + src + 1));
            max_component = std::max(max_component, *(image->dataFloat + src + 2));
            if(max_component > 1.0) {
                diff_sum += max_component-1.0; }
            clamped_sum += std::min(max_component, 1.0f);
            max_value =  std::max(max_component, max_value); 
            src += 3;
            dst += 4;
        }

        float scaleFactor = 1.0;  
        if(clamped_sum > 1.0) {
            // Apply global scale factor to compensate for intensity lost when clamping
            scaleFactor = (clamped_sum+diff_sum)/clamped_sum; 
            std::cout << "  +--> HDR Intensity Scale = " << std::to_string(scaleFactor) << " \n";
        }

        if(max_value > 1.0){
            std::cout << " [!]-> Environment light intensity cannot be displayed correctly on this device \n"; }
        
        //texture.data = new Uint8Array(numPixels * 4);
        texture.data = (char *)malloc(numPixels * 4);
        src = 0;
        dst = 0;
        for(unsigned int i = 0; i < numPixels; ++i) {
            // copy the pixels and pad the alpha channel
            texture.data[dst+0] = std::min(int((image->dataFloat[src+0])*255), 255);
            texture.data[dst+1] = std::min(int((image->dataFloat[src+1])*255), 255);
            texture.data[dst+2] = std::min(int((image->dataFloat[src+2])*255), 255);
            // unused
            texture.data[dst+3] = 255; 
            src += 3;
            dst += 4;
        }

        scaleValue =  scaleFactor;
        return texture;
    } 
}

unsigned int iblSampler::loadTextureHDR(t_image* image)
{
    t_texture texture = prepareTextureData(image);
    unsigned int textureID;
    GL_CALL(glCreateTextures(GL_TEXTURE_2D, 1, &textureID));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, textureID));
    GL_CALL(glTexImage2D(
        GL_TEXTURE_2D, // target
        0, // level
        texture.internalFormat, 
        image->width,
        image->height,
        0, // border
        texture.format, // format of the pixel data
        texture.type, // type of the pixel data
        texture.data ));
    free(texture.data);
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    return textureID;
}

unsigned int iblSampler::createCubemapTexture(bool withMipmaps)
{
    unsigned int targetTexture;
    GL_CALL(glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &targetTexture));
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, targetTexture));
    for(int i = 0; i < 6; ++i) {
        GL_CALL(glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat(), textureSize, textureSize, 0, GL_RGBA, textureTargetType(), NULL )); }
    if(withMipmaps) {
        GL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)); } 
    else {
        GL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR)); }
    GL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    return targetTexture; 
}

unsigned int iblSampler::createLutTexture( void )
{
    unsigned int targetTexture;
    GL_CALL(glCreateTextures(GL_TEXTURE_2D, 1, &targetTexture));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, targetTexture));
    GL_CALL(glTexImage2D( GL_TEXTURE_2D, 0, internalFormat(), lutResolution, lutResolution, 0, GL_RGBA, textureTargetType(), NULL ));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    return targetTexture; 
}

bool getExtension(const char* _ext_name)
{
    int NumberOfExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &NumberOfExtensions);
    for(unsigned int i=0; i<(unsigned int)NumberOfExtensions; i++) {
        const GLubyte *ccc=glGetStringi(GL_EXTENSIONS, i);
        if ( strcmp((const char *)ccc, _ext_name) == 0 ){
        return true;
        }
    }
    return false;
}    

void iblSampler::doinit(t_image* xpanoramaImage, const char* env_filename)
{
    // vv -- WebGL Naming
    if (getExtension("GL_NV_float") && getExtension("GL_ARB_color_buffer_float")) { //&& getExtension("OES_texture_float_linear")) {
        std::cout << "THIS DEVICE SUPPORTS FLOAT FORMAT TEXTURES\n";
    }
    // Native naming #2
    if (getExtension("GL_ARB_color_buffer_float") || getExtension("GL_NV_half_float")) {
        std::cout << "THIS DEVICE SUPPORTS HALF_FLOAT FORMAT TEXTURES\n";
    }

    int src_width, src_height, src_nrChannels;
    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MIN) std::cout << "Loading Environment Panorama/Equirectangular HDR: " << env_filename << "\n";
    #endif
    float *data = stbi_loadf(env_filename, &src_width, &src_height, &src_nrChannels, 3);
    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MED) std::cout << "  +--> Width = " << src_width << " / Height = " << src_height << " / Channels = " << src_nrChannels << "\n";
    #endif
    t_image panoramaImage = t_image();
    panoramaImage.width = src_width;
    panoramaImage.height = src_height;
    panoramaImage.dataFloatLength = panoramaImage.width * panoramaImage.height * 3;
    panoramaImage.dataFloat = (float*)calloc(panoramaImage.dataFloatLength, sizeof(float));
    for (unsigned long int i=0; i < panoramaImage.dataFloatLength; i++) {
        panoramaImage.dataFloat[i] = data[i];
    }
    stbi_image_free(data);
    inputTextureID = loadTextureHDR(&panoramaImage);
    free(panoramaImage.dataFloat);
    cubemapTextureID = createCubemapTexture(true);
    GL_CALL(glCreateFramebuffers(1, {&framebuffer}));
    lambertianTextureID = createCubemapTexture(false);
    ggxTextureID = createCubemapTexture(true);
    sheenTextureID = createCubemapTexture(true);
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, ggxTextureID));
    GL_CALL(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, sheenTextureID));
    GL_CALL(glGenerateMipmap(GL_TEXTURE_CUBE_MAP));
    mipmapLevels = std::floor(std::log2(textureSize))+1 - lowestMipLevel; 
}

void iblSampler::panoramaToCubeMap(void)
{
    for(int i = 0; i < 6; ++i) {
        //std::cout << "panoramaToCubeMap pass #" << i << "\n";
        if (callback != NULL) { callback("Processing CubeMap filter...", i, 36); }

        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer));
        GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemapTextureID, 0));
        GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureID));
        GL_CALL(glViewport(0, 0, textureSize, textureSize));
        GL_CALL(glClearColor(1.0, 0.0, 0.0, 0.0));
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT));
        pProgram shader = shaderCache->getShaderProgram( shaderCache, 
            shaderCache->selectShader ( shaderCache, "panorama_to_cubemap.frag", NO_DEFINES, 0 ), 
            shaderCache->selectShader ( shaderCache, "fullscreen.vert", NO_DEFINES, 0 ) );
        GL_CALL(glUseProgram(shader->program));
        //  TEXTURE0 = active.
        GL_CALL(glActiveTexture(GL_TEXTURE0+0));
        // Bind texture ID to active texture
        GL_CALL(glBindTexture(GL_TEXTURE_2D, inputTextureID));
        // map shader uniform to texture unit (TEXTURE0)
        GLuint location;
        GL_CALL(location = glGetUniformLocation(shader->program,"u_panorama"));
        GL_CALL(glUniform1i(location, 0)); // texture unit 0 (TEXTURE0)
        shader->updateUniform1i(shader, "u_currentFace", i);
        //fullscreen triangle
        GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 3)); 
    }
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureID));
    GL_CALL(glGenerateMipmap(GL_TEXTURE_CUBE_MAP)); 
}

void iblSampler::applyFilter(
    unsigned int distribution,
    float roughness,
    unsigned int targetMipLevel,
    unsigned int targetTexture,
    unsigned int sampleCount,
    float _lodBias,
    const char* _strProgress,
    float _baseProgress )
{
    unsigned int currentTextureSize = textureSize >> targetMipLevel;
    for(unsigned int i = 0; i < 6; ++i) {
        if (callback != NULL && (_baseProgress > 0.f)) { callback(_strProgress, i + _baseProgress, 36); }
        GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer));
        GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, targetTexture, targetMipLevel));
        GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, targetTexture));
        GL_CALL(glViewport(0, 0, currentTextureSize, currentTextureSize));
        GL_CALL(glClearColor(1.0, 0.0, 0.0, 0.0));
        GL_CALL(glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT));
        pProgram shader = shaderCache->getShaderProgram( shaderCache, 
            shaderCache->selectShader ( shaderCache, "ibl_filtering.frag", NO_DEFINES, 0 ), 
            shaderCache->selectShader ( shaderCache, "fullscreen.vert", NO_DEFINES, 0 ) );                
        GL_CALL(glUseProgram(shader->program));
        //  TEXTURE0 = active.
        GL_CALL(glActiveTexture(GL_TEXTURE0));
        // Bind texture ID to active texture
        GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureID));
        // map shader uniform to texture unit (TEXTURE0)
        unsigned int location = glGetUniformLocation(shader->program,"u_cubemapTexture");
        GL_CALL(glUniform1i(location, 0)); // texture unit 0
        shader->updateUniform1f(shader, "u_roughness", roughness);
        shader->updateUniform1i(shader, "u_sampleCount", sampleCount);
        shader->updateUniform1i(shader, "u_width", textureSize);
        shader->updateUniform1f(shader, "u_lodBias", _lodBias);
        shader->updateUniform1i(shader, "u_distribution", distribution);
        shader->updateUniform1i(shader, "u_currentFace", i);
        shader->updateUniform1i(shader, "u_isGeneratingLUT", 0);
        shader->updateUniform1i(shader, "u_floatTexture", 0);
        shader->updateUniform1f(shader, "u_intensityScale", scaleValue);
        //fullscreen triangle
        GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 3));
    }
}

void iblSampler::cubeMapToLambertian(void )
{
    applyFilter(
        0,
        0.0,
        0,
        lambertianTextureID,
        lambertianSampleCount,
        0.0,
        "Processing Lambertian filter...",
        6.f
    ); 
}

void iblSampler::cubeMapToGGX(void )
{
    for(unsigned int currentMipLevel = 0; currentMipLevel <= mipmapLevels; ++currentMipLevel) {
        float roughness = (float)(currentMipLevel) / (float)(mipmapLevels - 1);
        applyFilter(
            1,
            roughness,
            currentMipLevel,
            ggxTextureID,
            ggxSampleCount,
            0.0,
            "Processing GGX filter...",
            (currentMipLevel == 0) ? 12.f : -1.f
        ); 
    } 
}

void iblSampler::cubeMapToSheen(void )
{
    for(unsigned int currentMipLevel = 0; currentMipLevel <= mipmapLevels; ++currentMipLevel) {
        float roughness = (float)(currentMipLevel) / (float)(mipmapLevels - 1);
        applyFilter(
            2,
            roughness,
            currentMipLevel,
            sheenTextureID,
            sheenSamplCount,
            0.0,
            "Processing Sheen filter...",
            (currentMipLevel == 0) ? 18.f : -1.f
        );
    }
}

void iblSampler::sampleLut(unsigned int distribution, unsigned int targetTexture, unsigned int currentTextureSize)
{
    GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, framebuffer));
    GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targetTexture, 0));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, targetTexture));
    GL_CALL(glViewport(0, 0, currentTextureSize, currentTextureSize));
    GL_CALL(glClearColor(1.0, 0.0, 0.0, 0.0));
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT));
    pProgram shader = shaderCache->getShaderProgram( shaderCache, 
        shaderCache->selectShader ( shaderCache, "ibl_filtering.frag", NO_DEFINES, 0 ), 
        shaderCache->selectShader ( shaderCache, "fullscreen.vert", NO_DEFINES, 0 ) );   
        
    GL_CALL(glUseProgram(shader->program));
    //  TEXTURE0 = active.
    GL_CALL(glActiveTexture(GL_TEXTURE0+0));
    // Bind texture ID to active texture
    GL_CALL(glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureID));
    // map shader uniform to texture unit (TEXTURE0)
    unsigned int location = glGetUniformLocation(shader->program,"u_cubemapTexture");
    GL_CALL(glUniform1i(location, 0)); // texture unit 0
    shader->updateUniform1f( shader, "u_roughness", 0.0);
    shader->updateUniform1i( shader, "u_sampleCount", 128);
    //shader->updateUniform1i( shader, "u_sampleCount", 512);
    shader->updateUniform1i( shader, "u_width", 0.0);
    shader->updateUniform1f( shader, "u_lodBias", 0.0);
    shader->updateUniform1i( shader, "u_distribution", distribution);
    shader->updateUniform1i( shader, "u_currentFace", 0);
    shader->updateUniform1i( shader, "u_isGeneratingLUT", 1);
    //fullscreen triangle
    GL_CALL(glDrawArrays(GL_TRIANGLES, 0, 3)); 
}

void iblSampler::sampleGGXLut( void )
{
    ggxLutTextureID = createLutTexture();
    sampleLut(1, ggxLutTextureID, lutResolution);
}

void iblSampler::sampleCharlieLut( void )
{
    charlieLutTextureID = createLutTexture();
    sampleLut(2, charlieLutTextureID, lutResolution);
}

void iblSampler::filterAll( void (*_callback)(const char *, float, float) )
{
    callback = _callback;

    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MIN) std::cout << "Processing Environment...\n";
    #endif

    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MAX) std::cout << "  +--> panorama->CubeMap filter\n";
    #endif
    panoramaToCubeMap();

    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MAX) std::cout << "  +--> cubeMap->Lambertian filter\n"; 
    #endif
    cubeMapToLambertian();
    
    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MAX) std::cout << "  +--> cubeMap->GGX filter\n";
    #endif
    cubeMapToGGX();
    
    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MAX) std::cout << "  +--> cubeMap->Sheen filter\n";
    #endif
    cubeMapToSheen();
    if (callback != NULL) { callback("Sampling GGX LUT...", 27, 36); }
    
    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MAX) std::cout << "  +--> Sampling GGX Lut\n";
    #endif
    sampleGGXLut();
    if (callback != NULL) { callback("Sampling Charlie LUT...", 33, 36); }
    
    #ifdef IBL_SAMPLER_VERBOSITY
    if (IBL_SAMPLER_VERBOSITY >= VERBOSITY_MAX) std::cout << "  +--> Sampling Charlie Lut\n";
    #endif
    sampleCharlieLut();
    GL_CALL(glBindFramebuffer( GL_FRAMEBUFFER, GL_NONE));
}

void iblSampler::destroy_iblSampler(void) {
    destroy_ShaderCache(shaderCache);
}
// These functions have been migrated from gl_renwin (a temporary development file) into
//  this class.  They may move around again when more refactoring is complete.

void (*ibl_load_progress_callback)(const char*, const char* , float, float, float, float) = NULL;

void setIBLLoadPhaseCallback(void (*_ibl_load_progress_callback)(const char*, const char* , float, float, float, float)) {
    ibl_load_progress_callback= _ibl_load_progress_callback;
}

void envFilterCallback(const char* phase_title, float phase_current, float phase_max) {
    if (ibl_load_progress_callback != NULL) { ibl_load_progress_callback(phase_title, "SUBTEST1234", 0.f + ((phase_current / phase_max)*2.0f), 5.f, 0.f, 0.f); }
}

gl_environment_textures setup_environment(gl_environment_textures* _lastEnv, const char* _env_filename )
{
    gl_environment_textures _ret = gl_environment_textures();
    if ((_lastEnv != NULL) && (_lastEnv->loaded == true)) { 
        _ret.loaded = true; 
        _ret.diffuse = _lastEnv->diffuse; 
        _ret.specular = _lastEnv->specular; 
        _ret.sheen = _lastEnv->sheen;
        _ret.ggxLut = _lastEnv->ggxLut;
        _ret.charlieLut = _lastEnv->charlieLut;
        _ret.mipCount = _lastEnv->mipCount;
        _ret.iblIntensityScale = _lastEnv->iblIntensityScale;
        return _ret;
    }
    auto environmentFiltering = iblSampler();
    environmentFiltering.doinit(NULL, _env_filename);
    environmentFiltering.filterAll(envFilterCallback);

    _ret.loaded             = true;
    _ret.diffuse            = environmentFiltering.lambertianTextureID; 
    _ret.specular           = environmentFiltering.ggxTextureID; 
    _ret.sheen              = environmentFiltering.sheenTextureID;
    _ret.ggxLut             = environmentFiltering.ggxLutTextureID;
    _ret.charlieLut         = environmentFiltering.charlieLutTextureID;
    _ret.mipCount           = environmentFiltering.mipmapLevels;
    _ret.iblIntensityScale  = environmentFiltering.scaleValue;

    environmentFiltering.destroy_iblSampler();
    return _ret;
}
