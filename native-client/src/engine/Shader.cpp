#include "Shader.hpp"
#include <iostream>
#include <vector>

namespace mc {

Shader::Shader(const std::string& vertexSrc, const std::string& fragmentSrc) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    programId = glCreateProgram();
    glAttachShader(programId, vs);
    glAttachShader(programId, fs);
    glLinkProgram(programId);

    GLint linked = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint len = 0;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetProgramInfoLog(programId, len, &len, log.data());
        std::cerr << "[Shader] Link error: " << log.data() << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() {
    if (programId) {
        glDeleteProgram(programId);
    }
}

void Shader::bind() const {
    glUseProgram(programId);
}

void Shader::unbind() const {
    glUseProgram(0);
}

GLint Shader::getUniformLocation(const std::string& name) {
    auto it = uniformLocations.find(name);
    if (it != uniformLocations.end()) return it->second;
    GLint loc = glGetUniformLocation(programId, name.c_str());
    uniformLocations[name] = loc;
    return loc;
}

void Shader::setInt(const std::string& name, int value) {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, float x, float y) {
    glUniform2f(getUniformLocation(name), x, y);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) {
    glUniform3f(getUniformLocation(name), x, y, z);
}

void Shader::setMat4(const std::string& name, const Mat4& matrix) {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, matrix.data());
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint s = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint compiled = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len);
        glGetShaderInfoLog(s, len, &len, log.data());
        std::cerr << "[Shader] Compilation error (" << (type == GL_VERTEX_SHADER ? "VS" : "FS") << "): " << log.data() << std::endl;
    }
    return s;
}

} // namespace mc
