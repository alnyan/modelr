#include "texture.h"
#include <iostream>
#include <stdio.h>
#include <png.h>
#include "../res/lodepng.h"
#include "../res/assets.h"

Texture::Texture(GLuint texID): m_textureID{texID} {}

Texture::~Texture() {
    glDeleteTextures(1, &m_textureID);
}

void Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, m_textureID);
}

Texture *Texture::loadPng(const std::string &path) {
    std::vector<unsigned char> data;
    unsigned int width, height, error;
    if ((error = lodepng::decode(data, width, height, Assets::getTexturePath(path)))) {
        std::cerr << "Error" << std::endl;
        return nullptr;
    }

    GLuint texID;
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
    glGenerateMipmap(GL_TEXTURE_2D);

    return new Texture(texID);
    //FILE *fp = fopen(path.c_str(), "rb");
    //png_bytep png_data = nullptr;
    //png_structp png_ptr;
    //png_infop png_info_ptr, png_end_info_ptr;
    //png_uint_32 width, height, png_row_bytes;
    //int bit_depth, color_type;
    //char pngSig[8];
    //png_bytep *png_row_ptrs;
    //GLuint texID;

    //if (!fp) {
        //return nullptr;
    //}

    //if (fread(pngSig, 1, sizeof(pngSig), fp) != sizeof(pngSig)) {
        //fclose(fp);
        //return nullptr;
    //}

    //if (png_sig_cmp((png_const_bytep) pngSig, 0, sizeof(pngSig))) {
        //fclose(fp);
        //std::cerr << "File is not a PNG image" << std::endl;
        //return nullptr;
    //}

    //png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

    //if (!png_ptr) {
        //std::cout << "Failed to init libpng" << std::endl;
        //fclose(fp);
        //return nullptr;
    //}

    //png_info_ptr = png_create_info_struct(png_ptr);

    //if (!png_info_ptr) {
        //std::cout << "Failed to init libpng" << std::endl;
        //png_destroy_read_struct(&png_ptr, nullptr, nullptr);
        //fclose(fp);
        //return nullptr;
    //}

    //png_end_info_ptr = png_create_info_struct(png_ptr);

    //if (!png_end_info_ptr) {
        //std::cout << "Failed to init libpng" << std::endl;
        //png_destroy_read_struct(&png_ptr, &png_info_ptr, nullptr);
        //fclose(fp);
        //return nullptr;
    //}

    //if (setjmp(png_jmpbuf(png_ptr))) {
        //goto error;
    //}

    //png_init_io(png_ptr, fp);
    //png_set_sig_bytes(png_ptr, sizeof(pngSig));

    //png_read_info(png_ptr, png_info_ptr);

    //png_get_IHDR(png_ptr, png_info_ptr, &width, &height, &bit_depth, &color_type, nullptr, nullptr, nullptr);

    //if (width > 4096 || height > 4096) {
        //std::cerr << "Texture too big" << std::endl;
        //goto error;
    //}

    //std::cout << "Texture size: " << width << "x" << height << std::endl;

    //if (!(png_row_ptrs = (png_bytep *) malloc(sizeof(png_bytep) * height))) {
        //std::cerr << "Failed to allocate row pointers" << std::endl;
    //}
    //png_read_update_info(png_ptr, png_info_ptr);
    //png_row_bytes = png_get_rowbytes(png_ptr, png_info_ptr);

    //if (!(png_data = (png_bytep) malloc(png_row_bytes * height))) {
        //std::cerr << "Failed to allocate image buffer" << std::endl;
        //free(png_row_ptrs);
        //goto error;
    //}

    //for (png_uint_32 i = 0; i < height; ++i) {
        //png_row_ptrs[i] = &png_data[i * png_row_bytes];
    //}

    //png_read_image(png_ptr, png_row_ptrs);
    //png_read_end(png_ptr, nullptr);

    //free(png_row_ptrs);

    //// The expected data is in RGBA format in png_data
    //glGenTextures(1, &texID);
    //glBindTexture(GL_TEXTURE_2D, texID);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_BYTE, png_data);

    //free(png_data);
//error:
    //png_destroy_read_struct(&png_ptr, &png_info_ptr, &png_end_info_ptr);
    //fclose(fp);
    //return nullptr;
}
