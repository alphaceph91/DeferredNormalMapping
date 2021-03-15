#pragma once
#ifndef ASSETS_H
#define ASSETS_H



#include <glad/glad.h> // holds all OpenGL type declarations
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <string>
#include <vector>
#include <optional>
#include <any>
#include <chrono> // for timing

#include <util/model.h>

bool powerOf2(int n)
{
    return (n & (n - 1)) == 0; // see http://www.graphics.stanford.edu/~seander/bithacks.html or https://stackoverflow.com/questions/108318/whats-the-simplest-way-to-test-whether-a-number-is-a-power-of-2-in-c
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    // stbi_set_flip_vertically_on_load(flipVertically);

    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        try
        {
            if (width <= 0 || height <= 0) throw "Texture is 0 in at least one dimension!";

            // test for power of 2
            if (!powerOf2(width) || !powerOf2(height)) throw "Texture is not power of 2!"; // if this happens make sure that the texture has power of 2 dimensions (e.g., 512, 1024, ...)


            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;
            else
                throw "Number of Channels not supported!";

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        catch (const char* emsg) {
            std::cout << "Failed to use texture " << path << " because: " << emsg << endl;
        }

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


typedef std::map<const std::string, std::string> CubeMapPaths;
// utility function for loading a cube map texture from file
// ---------------------------------------------------
unsigned int loadCubemap(CubeMapPaths cubemap) {

    unsigned int cubeTextureID;
    glGenTextures(1, &cubeTextureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTextureID);

    std::string faces[6] = { "right", "left","top","bottom","front","back" };

    //stbi_set_flip_vertically_on_load(true);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < 6; i++) {
        auto imgpath = cubemap[faces[i]];
        unsigned char* image = stbi_load(imgpath.c_str(), &width, &height, &nrComponents, 0);

        if (image) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
            stbi_image_free(image);
        }
        else {
            std::cout << "Cubemap texture failed to load for: " << faces[i] << std::endl;
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return cubeTextureID;
}

// forward declarations
class Model; // difined in model.h

// typedefs to ease template usage
typedef std::pair<const std::string, std::any> AssetItem;
typedef std::map<const std::string, std::any> AssetItemMap;
typedef std::pair<const std::string, AssetItemMap> AssetGroup;
typedef std::map<const std::string, AssetItemMap> Assets;

// if textures should be flipped upside down use { TEX_FLIP, true }
const std::string TEX_FLIP = "setting-flip-texture";

// a std::map (global variable) that stores all the assets that need to be loaded (i.e., textures and models). Also makes sure that assets are only loaded once!
std::map<const std::string, std::any> loadedAssets;




// Helper class for textures
class Tex
{
private:
    unsigned int m_id; // OpenGL texture id to use with glBindTexture
public:
    Tex(unsigned int id) :m_id{ id } {}
    operator unsigned int() { return m_id; } // cast operator
};

class AssetManager
{
private:
    Assets m_assets;
    std::string m_active;

    // checks if there is a TEX_FLIP="setting-flip-texture" key in the group and check if it is boolean
    bool flipImagesForGroup(const std::string& group)
    {
        auto g = m_assets.at(group);
        if (g.find(TEX_FLIP) != g.end()) // key found
            return GetAsset<bool>(group, TEX_FLIP);
        else
            return false; // if key is not set we assume no flipping!
    }

    bool GroupExists(const std::string& group)
    {
        if (m_assets.find(group) != m_assets.end()) // key found
            return true;
        else
            return false;
    }

    template <class T>
    T Convert(std::any& r)
    {
        try
        {
            return std::any_cast<T>(r);
        }
        catch (const std::bad_any_cast& e)
        {
            std::cout << e.what() << '\n';
            std::cout << "object is of type " << r.type().name() << " and cannot be cast to " << typeid(T).name() << std::endl;
            throw e;
            //return T();
        }
    }

    template <>
    Model Convert(std::any& r)
    {
        try
        {
            auto path = std::any_cast<const char*>(r);

            if (loadedAssets.count(path) <= 0) // not loaded yet (lazy init)
            {
                std::cout << "Loading Model " << path << " ... ";
                auto t1 = std::chrono::high_resolution_clock::now();
                Model* m = new Model(path);
                loadedAssets.insert(std::pair<const std::string, std::any>(path, *m));
                auto t2 = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

                std::cout << "done (in " << (duration / 1000) << " milliseconds)." << std::endl;
            }
            auto m = std::any_cast<Model&>(loadedAssets.at(path));
            return m;
        }
        catch (const std::bad_any_cast& e)
        {
            std::cout << e.what() << '\n';
            throw e;
        }
    }

    template <>
    Tex Convert(std::any& r)
    {
        try
        {   // handle 6 face cube maps
            auto cubemap = std::any_cast<CubeMapPaths>(r);
            auto uniquename = "cubemap_" + cubemap["front"];

            if (loadedAssets.count(uniquename) <= 0) // not loaded yet (lazy init)
            {
                std::cout << "Loading CubeMap " << uniquename << " ... ";
                auto t1 = std::chrono::high_resolution_clock::now();
                loadedAssets.insert(std::pair<const std::string, std::any>(uniquename, Tex(loadCubemap(cubemap))));
                auto t2 = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

                std::cout << "done (in " << (duration / 1000) << " milliseconds)." << std::endl;
            }
            auto c = std::any_cast<Tex>(loadedAssets.at(uniquename));
            return c;

        }
        catch (const std::bad_any_cast& e_cubemap) // if not a cube map
        {
            try
            {   // handle 2D textures
                auto path = std::any_cast<const char*>(r);

                if (loadedAssets.count(path) <= 0) // not loaded yet (lazy init)
                {
                    std::cout << "Loading Texture " << path << " ... ";
                    auto t1 = std::chrono::high_resolution_clock::now();
                    loadedAssets.insert(std::pair<const std::string, std::any>(path, Tex(loadTexture(path))));
                    auto t2 = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

                    std::cout << "done (in " << (duration / 1000) << " milliseconds)." << std::endl;
                }
                auto t = std::any_cast<Tex>(loadedAssets.at(path));
                return t;
            }
            catch (const std::bad_any_cast& e)
            {
                std::cout << e.what() << '\n';
                throw e;
            }
        }
    }

public: 
    AssetManager(const Assets assets) : m_assets{ assets } { m_active = m_assets.begin()->first;  }

    template<class T>
    T GetAsset(const std::string& group, const std::string &name)
    {
        return Convert<T>(m_assets.at(group).at(name));
    }

    // Textures need a specialized function, due to the possiblity of flipping it vertically
    template<>
    Tex GetAsset(const std::string& group, const std::string& name)
    {
        // Optionally tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
        stbi_set_flip_vertically_on_load(flipImagesForGroup(group));

        return Convert<Tex>(m_assets.at(group).at(name));
    }

    template<class T>
    T GetActiveAsset(const std::string& name)
    {
        return GetAsset<T>(m_active, name);
    }

    std::vector<std::string> GetGroups() const
    {
        std::vector<std::string> keys;
        for (auto const& amp : m_assets) { keys.push_back(amp.first); }
        return keys;
    }

    void SetActiveGroup(const std::string& group)
    {
        // make sure the group exists!
        if (!GroupExists(group)) return;
        m_active = group;
    }

    void SetActiveGroup(const int id) { SetActiveGroup(GetGroups().at(id));}
    const std::string GetActiveGroup() { return m_active; }
    const int GetActiveGroupId() { 
        auto groups = GetGroups();
        return std::distance(groups.begin(), std::find(groups.begin(), groups.end(), GetActiveGroup())); 
    }

};


#endif
