#pragma once
#include <GL/glew.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

// graphics/Shader.hpp : loads, compiles, and wraps an OpenGL shader program.
class Shader {
public:
    Shader() : programID(0) {}

    ~Shader() {
        if (programID) glDeleteProgram(programID);
    }

    // Pure: compiles a shader from source string, returns shader ID.
    static GLuint compileShader(GLenum type, const std::string& src) {
        GLuint id = glCreateShader(type);
        const char* c = src.c_str();
        glShaderSource(id, 1, &c, nullptr);
        glCompileShader(id);
        GLint ok;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetShaderInfoLog(id, 512, nullptr, log);
            std::cerr << "Shader compile error: " << log << std::endl;
        }
        return id;
    }

    // Input wrapper: reads a text file into a string.
    static std::string readFile(const std::string& path) {
        std::ifstream f(path);
        std::stringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    // Loads vertex + fragment shader from file paths.
    void load(const std::string& vertPath, const std::string& fragPath) {
        GLuint vert = compileShader(GL_VERTEX_SHADER, readFile(vertPath));
        GLuint frag = compileShader(GL_FRAGMENT_SHADER, readFile(fragPath));
        programID = glCreateProgram();
        glAttachShader(programID, vert);
        glAttachShader(programID, frag);
        glLinkProgram(programID);
        GLint ok;
        glGetProgramiv(programID, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[512];
            glGetProgramInfoLog(programID, 512, nullptr, log);
            std::cerr << "Shader link error: " << log << std::endl;
        }
        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    // Loads vertex + fragment shader from source strings directly.
    void loadFromSource(const std::string& vertSrc, const std::string& fragSrc) {
        GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
        GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
        programID = glCreateProgram();
        glAttachShader(programID, vert);
        glAttachShader(programID, frag);
        glLinkProgram(programID);
        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void use() const { glUseProgram(programID); }
    GLuint id() const { return programID; }

    void setMat4(const std::string& name, const glm::mat4& val) const {
        glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, glm::value_ptr(val));
    }

    void setVec3(const std::string& name, const glm::vec3& val) const {
        glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, glm::value_ptr(val));
    }

    void setFloat(const std::string& name, float val) const {
        glUniform1f(glGetUniformLocation(programID, name.c_str()), val);
    }

    void setInt(const std::string& name, int val) const {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), val);
    }

private:
    GLuint programID;
};
