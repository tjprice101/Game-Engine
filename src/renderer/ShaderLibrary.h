#pragma once
#include "renderer/Shader.h"
#include <unordered_map>
#include <string>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <iostream>

// ---- ShaderLibrary ----------------------------------------------------------
// Centralised shader registry with:
//  - Named access: ShaderLibrary::get("tile_lit")
//  - #include preprocessing (relative to shader directory)
//  - Hot-reload: pollHotReload() checks file modification times each frame
//  - Shader variants: loadVariant("tile_lit:ALPHA_CLIP", vertPath, fragPath, "#define ALPHA_CLIP 1\n")

class ShaderLibrary {
public:
    static ShaderLibrary& instance();

    // ---- Registration -------------------------------------------------------
    void load(const std::string& name,
              const std::string& vertPath,
              const std::string& fragPath,
              const std::string& preamble = "");

    // ---- Access -------------------------------------------------------------
    Shader& get(const std::string& name);
    bool    has(const std::string& name) const;

    // ---- Hot-reload (call once per frame in dev builds) ---------------------
    void pollHotReload();
    void enableHotReload(bool enable) { m_hotReload = enable; }

    // ---- Shader directory (prefix prepended to load() paths) ---------------
    void setBaseDir(const std::string& dir) { m_baseDir = dir; }
    const std::string& baseDir() const      { return m_baseDir; }

    // ---- Preprocessing (public for testing) ---------------------------------
    // Resolves #include "file" directives relative to `dir`
    static std::string preprocess(const std::string& src,
                                  const std::string& dir,
                                  const std::string& preamble = "",
                                  int depth = 0);

private:
    ShaderLibrary() = default;

    struct Entry {
        Shader      shader;
        std::string vertPath;
        std::string fragPath;
        std::string preamble;
        std::filesystem::file_time_type vertMtime{};
        std::filesystem::file_time_type fragMtime{};
    };

    Shader compile(const std::string& vertPath,
                   const std::string& fragPath,
                   const std::string& preamble);

    static std::string readFile(const std::string& path);
    static std::filesystem::file_time_type mtime(const std::string& path);

    std::unordered_map<std::string, Entry> m_shaders;
    std::string m_baseDir    = "assets/shaders/";
    bool        m_hotReload  = true;
};
