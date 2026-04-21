#include "ShaderLibrary.h"
#include <stdexcept>

ShaderLibrary& ShaderLibrary::instance() {
    static ShaderLibrary lib;
    return lib;
}

// ---- Load / compile ---------------------------------------------------------

void ShaderLibrary::load(const std::string& name,
                          const std::string& vertPath,
                          const std::string& fragPath,
                          const std::string& preamble)
{
    Entry e;
    e.vertPath = m_baseDir + vertPath;
    e.fragPath = m_baseDir + fragPath;
    e.preamble = preamble;
    e.shader   = compile(e.vertPath, e.fragPath, preamble);
    e.vertMtime= mtime(e.vertPath);
    e.fragMtime= mtime(e.fragPath);
    m_shaders[name] = std::move(e);
}

Shader ShaderLibrary::compile(const std::string& vertPath,
                               const std::string& fragPath,
                               const std::string& preamble)
{
    std::string vertDir = vertPath.substr(0, vertPath.rfind('/') + 1);
    std::string fragDir = fragPath.substr(0, fragPath.rfind('/') + 1);

    std::string vertSrc = preprocess(readFile(vertPath), vertDir, preamble);
    std::string fragSrc = preprocess(readFile(fragPath), fragDir, preamble);

    // Use the Shader class by compiling from in-memory source (via temp strings)
    // We create the shader inline using the existing Shader API
    return Shader::fromSource(vertSrc, fragSrc);
}

// ---- Access -----------------------------------------------------------------

Shader& ShaderLibrary::get(const std::string& name) {
    auto it = m_shaders.find(name);
    if (it == m_shaders.end())
        throw std::runtime_error("ShaderLibrary: unknown shader '" + name + "'");
    return it->second.shader;
}

bool ShaderLibrary::has(const std::string& name) const {
    return m_shaders.count(name) > 0;
}

// ---- Hot-reload -------------------------------------------------------------

void ShaderLibrary::pollHotReload() {
    if (!m_hotReload) return;
    for (auto& [name, e] : m_shaders) {
        auto vm = mtime(e.vertPath);
        auto fm = mtime(e.fragPath);
        if (vm != e.vertMtime || fm != e.fragMtime) {
            try {
                e.shader   = compile(e.vertPath, e.fragPath, e.preamble);
                e.vertMtime = vm;
                e.fragMtime = fm;
                std::cout << "[ShaderLibrary] Hot-reloaded: " << name << '\n';
            } catch (const std::exception& ex) {
                std::cerr << "[ShaderLibrary] Reload failed (" << name << "): "
                          << ex.what() << '\n';
                e.vertMtime = vm; // don't keep re-trying on every frame
                e.fragMtime = fm;
            }
        }
    }
}

// ---- Preprocessing ----------------------------------------------------------

std::string ShaderLibrary::preprocess(const std::string& src,
                                       const std::string& dir,
                                       const std::string& preamble,
                                       int depth)
{
    if (depth > 16) return src;

    std::ostringstream out;
    if (depth == 0 && !preamble.empty())
        out << preamble << '\n';

    std::istringstream ss(src);
    std::string line;
    while (std::getline(ss, line)) {
        // Handle: #include "filename"
        auto pos = line.find("#include");
        if (pos != std::string::npos) {
            auto q1 = line.find('"', pos);
            auto q2 = line.rfind('"');
            if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
                std::string incFile = dir + line.substr(q1 + 1, q2 - q1 - 1);
                std::string incDir  = incFile.substr(0, incFile.rfind('/') + 1);
                try {
                    out << preprocess(readFile(incFile), incDir, "", depth + 1);
                } catch (...) {
                    std::cerr << "[ShaderLibrary] Cannot include: " << incFile << '\n';
                }
                continue;
            }
        }
        out << line << '\n';
    }
    return out.str();
}

// ---- Helpers ----------------------------------------------------------------

std::string ShaderLibrary::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("ShaderLibrary: cannot open '" + path + "'");
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::filesystem::file_time_type ShaderLibrary::mtime(const std::string& path) {
    std::error_code ec;
    return std::filesystem::last_write_time(path, ec);
}
