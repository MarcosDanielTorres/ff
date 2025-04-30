#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

#if 0
struct Shader
{
    u32 id;
};

void shader_use(Shader shader)
{
    glUseProgram(shader.id);
}

void shader_set_bool(Shader shader, const char *name, b32 value) const
{
    glUniform1i(glGetUniformLocation(shader.id, name), value);
}
// ------------------------------------------------------------------------
void shader_set_int(Shader shader, const char *name, int value) const
{
    glUniform1i(glGetUniformLocation(shader.id, name), value);
}
// ------------------------------------------------------------------------
void shader_set_float(Shader shader, const char *name, float value) const
{
    glUniform1f(glGetUniformLocation(shader.id, name), value);
}
// ------------------------------------------------------------------------
void shader_set_vec2(Shader shader, const char *name, const glm::vec2& value) const
{
    glUniform2fv(glGetUniformLocation(shader.id, name), 1, &value[0]);
}
void shader_set_vec2(Shader shader, const char *name, f32 x, f32 y) const
{
    glUniform2f(glGetUniformLocation(shader.id, name), x, y);
}
// ------------------------------------------------------------------------
void shader_set_vec3(Shader shader, const char *name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(shader.id, name), 1, &value[0]);
}
void shader_set_vec3(Shader shader, const char *name, f32 x, f32 y, f32 z) const
{
    glUniform3f(glGetUniformLocation(shader.id, name), x, y, z);
}
// ------------------------------------------------------------------------
void shader_set_vec4(Shader shader, const char *name, const glm::vec4& value) const
{
    glUniform4fv(glGetUniformLocation(shader.id, name), 1, &value[0]);
}
void shader_set_vec4(Shader shader, const char *name, f32 x, f32 y, f32 z, f32 w) const
{
    glUniform4f(glGetUniformLocation(shader.id, name), x, y, z, w);
}
// ------------------------------------------------------------------------
void shader_set_mat2(Shader shader, const char *name, const glm::mat2& mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(shader.id, name), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void shader_set_mat3(Shader shader, const char *name, const glm::mat3& mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(shader.id, name), 1, GL_FALSE, &mat[0][0]);
}
// ------------------------------------------------------------------------
void shader_set_mat4(Shader shader, const char *name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(shader.id, name), 1, GL_FALSE, &mat[0][0]);
}
#endif

class Shader
{
public:
	unsigned int ID;
    OpenGL *opengl;
	// constructor generates the shader on the fly
	// ------------------------------------------------------------------------
	Shader(const char* vertexPath, const char* fragmentPath, OpenGL *opengll)
	{
        opengl = opengll;

		{
			std::filesystem::path cwd = std::filesystem::current_path();
			//AIM_DEBUG("CURRENT PATH: %s", cwd.string().c_str());
			printf("CURRENT PATH: %s", cwd.string().c_str());
		}


        // TODO deal with assets
		//std::string vertexFullPath = std::string(AIM_ENGINE_ASSETS_PATH) + vertexPath;
		//std::string fragmentFullPath = std::string(AIM_ENGINE_ASSETS_PATH)  + fragmentPath;
		//AIM_DEBUG("Looking for vert shader at: %s", vertexFullPath.c_str());
		//AIM_DEBUG("Looking for frag shader at: %s", fragmentFullPath.c_str());
		std::string vertexFullPath = std::string("build/") + vertexPath;
		std::string fragmentFullPath =  std::string("build/") + fragmentPath;
		printf("Looking for vert shader at: %s\n", vertexFullPath.c_str());
		printf("Looking for frag shader at: %s\n", fragmentFullPath.c_str());

		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try
		{
			// open files
			vShaderFile.open(vertexFullPath);
			fShaderFile.open(fragmentFullPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure& e)
		{
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();
		// 2. compile shaders
		unsigned int vertex, fragment;
		// vertex shader
		vertex = opengl->glCreateShader(GL_VERTEX_SHADER);
		opengl->glShaderSource(vertex, 1, &vShaderCode, NULL);
		opengl->glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");
		// fragment Shader
		fragment = opengl->glCreateShader(GL_FRAGMENT_SHADER);
		opengl->glShaderSource(fragment, 1, &fShaderCode, NULL);
		opengl->glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");
		// shader Program
		ID = opengl->glCreateProgram();
		opengl->glAttachShader(ID, vertex);
		opengl->glAttachShader(ID, fragment);
		opengl->glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");
		// delete the shaders as they're linked into our program now and no longer necessary
		opengl->glDeleteShader(vertex);
		opengl->glDeleteShader(fragment);

	}
	// activate the shader
	// ------------------------------------------------------------------------
	void use() const
	{
		opengl->glUseProgram(ID);
	}
	// utility uniform functions
	// ------------------------------------------------------------------------
	void setBool(const std::string& name, bool value) const
	{
		opengl->glUniform1i(opengl->glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	// ------------------------------------------------------------------------
	void setInt(const std::string& name, int value) const
	{
		opengl->glUniform1i(opengl->glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setFloat(const std::string& name, float value) const
	{
		opengl->glUniform1f(opengl->glGetUniformLocation(ID, name.c_str()), value);
	}
	// ------------------------------------------------------------------------
	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		opengl->glUniform2fv(opengl->glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec2(const std::string& name, float x, float y) const
	{
		opengl->glUniform2f(opengl->glGetUniformLocation(ID, name.c_str()), x, y);
	}
	// ------------------------------------------------------------------------
	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		opengl->glUniform3fv(opengl->glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec3(const std::string& name, float x, float y, float z) const
	{
		opengl->glUniform3f(opengl->glGetUniformLocation(ID, name.c_str()), x, y, z);
	}
	// ------------------------------------------------------------------------
	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		opengl->glUniform4fv(opengl->glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setVec4(const std::string& name, float x, float y, float z, float w) const
	{
		opengl->glUniform4f(opengl->glGetUniformLocation(ID, name.c_str()), x, y, z, w);
	}
	// ------------------------------------------------------------------------
	void setMat2(const std::string& name, const glm::mat2& mat) const
	{
		opengl->glUniformMatrix2fv(opengl->glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat3(const std::string& name, const glm::mat3& mat) const
	{
		opengl->glUniformMatrix3fv(opengl->glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}
	// ------------------------------------------------------------------------
	void setMat4(const std::string& name, const glm::mat4& mat) const
	{
		opengl->glUniformMatrix4fv(opengl->glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

private:
	// utility function for checking shader compilation/linking errors.
	// ------------------------------------------------------------------------
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			opengl->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				opengl->glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			opengl->glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				opengl->glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};