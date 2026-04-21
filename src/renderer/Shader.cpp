#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
    std::string vertSrc = readFile(vertPath);
    std::string fragSrc = readFile(fragPath);

    GLuint vert = compileStage(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileStage(GL_FRAGMENT_SHADER, fragSrc);

    m_id = glCreateProgram();
    glAttachShader(m_id, vert);
    glAttachShader(m_id, frag);
    glLinkProgram(m_id);

    GLint ok;
    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        glDeleteProgram(m_id); m_id = 0;
        glDeleteShader(vert);
        glDeleteShader(frag);
        throw std::runtime_error(std::string("Shader link error: ") + log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

Shader Shader::fromSource(const std::string& vertSrc, const std::string& fragSrc) {
    Shader s;
    GLuint vert = s.compileStage(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = s.compileStage(GL_FRAGMENT_SHADER, fragSrc);

    s.m_id = glCreateProgram();
    glAttachShader(s.m_id, vert);
    glAttachShader(s.m_id, frag);
    glLinkProgram(s.m_id);

    GLint ok;
    glGetProgramiv(s.m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(s.m_id, sizeof(log), nullptr, log);
        glDeleteProgram(s.m_id); s.m_id = 0;
        glDeleteShader(vert);
        glDeleteShader(frag);
        throw std::runtime_error(std::string("Shader link error: ") + log);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return s;
}

Shader::~Shader() {
    if (m_id) glDeleteProgram(m_id);
}

Shader::Shader(Shader&& o) noexcept : m_id(o.m_id), m_uniformCache(std::move(o.m_uniformCache)) {
    o.m_id = 0;
}
Shader& Shader::operator=(Shader&& o) noexcept {
    if (this != &o) {
        if (m_id) glDeleteProgram(m_id);
        m_id = o.m_id; o.m_id = 0;
        m_uniformCache = std::move(o.m_uniformCache);
    }
    return *this;
}

void Shader::bind()   const { glUseProgram(m_id); }
void Shader::unbind() const { glUseProgram(0); }

GLuint Shader::compileStage(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(shader, 1, &c, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile error: ") + log);
    }
    return shader;
}

GLint Shader::location(const std::string& name) const {
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) return it->second;
    GLint loc = glGetUniformLocation(m_id, name.c_str());
    m_uniformCache[name] = loc;
    return loc;
}

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open shader file: " + path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void Shader::setInt  (const std::string& n, int v)              const { glUniform1i (location(n), v); }
void Shader::setFloat(const std::string& n, float v)            const { glUniform1f (location(n), v); }
void Shader::setVec2 (const std::string& n, const glm::vec2& v) const { glUniform2fv(location(n),1,glm::value_ptr(v)); }
void Shader::setVec3 (const std::string& n, const glm::vec3& v) const { glUniform3fv(location(n),1,glm::value_ptr(v)); }
void Shader::setVec4 (const std::string& n, const glm::vec4& v) const { glUniform4fv(location(n),1,glm::value_ptr(v)); }
void Shader::setMat4 (const std::string& n, const glm::mat4& v) const { glUniformMatrix4fv(location(n),1,GL_FALSE,glm::value_ptr(v)); }
