#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    // Compile from in-memory GLSL source strings (used by ShaderLibrary)
    static Shader fromSource(const std::string& vertSrc, const std::string& fragSrc);

    // Non-copyable, movable
    Shader(const Shader&)            = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& o) noexcept;
    Shader& operator=(Shader&& o) noexcept;

    void bind()   const;
    void unbind() const;
    bool valid()  const { return m_id != 0; }

    // Uniform setters
    void setInt   (const std::string& name, int v)              const;
    void setFloat (const std::string& name, float v)            const;
    void setVec2  (const std::string& name, const glm::vec2& v) const;
    void setVec3  (const std::string& name, const glm::vec3& v) const;
    void setVec4  (const std::string& name, const glm::vec4& v) const;
    void setMat4  (const std::string& name, const glm::mat4& v) const;

    GLuint id() const { return m_id; }

private:
    GLuint compileStage(GLenum type, const std::string& src);
    GLint  location(const std::string& name) const;
    static std::string readFile(const std::string& path);

    GLuint m_id = 0;
    mutable std::unordered_map<std::string, GLint> m_uniformCache;
};
