#pragma once
#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include "../math/Math.hpp"

namespace mc {

class Shader {
public:
    Shader() = default;
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
    ~Shader();

    void bind() const;
    void unbind() const;

    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, float x, float y);
    void setVec3(const std::string& name, float x, float y, float z);
    void setMat4(const std::string& name, const Mat4& matrix);

    GLuint getProgramId() const { return programId; }

private:
    GLuint programId{0};
    std::unordered_map<std::string, GLint> uniformLocations;

    GLint getUniformLocation(const std::string& name);
    GLuint compileShader(GLenum type, const std::string& source);
};

} // namespace mc
