#include "Renderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

namespace {
    std::string readTextFile(const std::filesystem::path& path) {
        std::ifstream file (path, std::ios::binary);
        if (!file) {
            throw std::runtime_error ("Unable to open shader: " + path.string());
        }
        std::ostringstream content;
        content << file.rdbuf();
        return content.str();
    }

    GLuint compileShader(GLenum type, const std::string& source, const std::filesystem::path& path) {
        const GLuint shader = glCreateShader (type);
        const char* sourcePointer = source.c_str();
        glShaderSource (shader, 1, &sourcePointer, nullptr);
        glCompileShader (shader);

        GLint successful = GL_FALSE;
        glGetShaderiv (shader, GL_COMPILE_STATUS, &successful);
        if (successful == GL_FALSE) {
            GLint logLength = 0;
            glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &logLength);
            std::string log (static_cast <std::size_t> (logLength), '\0');
            glGetShaderInfoLog (shader, logLength, nullptr, log.data());
            glDeleteShader (shader);
            throw std::runtime_error ("Shader compilation failed for " + path.string() + ":\n" + log);
        }
        return shader;
    }

    glm::vec3 normalizeOr(const glm::vec3& value, const glm::vec3& fallback) {
        const float length = glm::length (value);
        return length > 0.0001F ? value / length : fallback;
    }
}

Renderer::~Renderer() {
    for (const Mesh& mesh: meshes_) {
        if (mesh.indexBuffer != 0) {
            glDeleteBuffers (1, &mesh.indexBuffer);
            glDeleteBuffers (1, &mesh.vertexBuffer);
            glDeleteVertexArrays (1, &mesh.vertexArray);
        }
    }
    for (const GLuint texture: textures_) {
        glDeleteTextures (1, &texture);
    }
    if (program_ != 0) {
        glDeleteProgram (program_);
    }
}

void Renderer::initialize(const std::filesystem::path& projectRoot) {
    createProgram (projectRoot);

    std::vector <unsigned int> indices;
    const auto plane = makePlaneVertices();
    indices = { 0, 1, 2, 0, 2, 3 };
    createMesh (meshes_[static_cast <std::size_t> (Primitive::Plane)], plane, indices);

    indices.clear();
    const auto cube = makeCubeVertices (indices);
    createMesh (meshes_[static_cast <std::size_t> (Primitive::Cube)], cube, indices);

    indices.clear();
    const auto cylinder = makeCylinderVertices (indices, false);
    createMesh (meshes_[static_cast <std::size_t> (Primitive::Cylinder)], cylinder, indices);

    indices.clear();
    const auto cone = makeCylinderVertices (indices, true);
    createMesh (meshes_[static_cast <std::size_t> (Primitive::Cone)], cone, indices);

    indices.clear();
    const auto sphere = makeSphereVertices (indices);
    createMesh (meshes_[static_cast <std::size_t> (Primitive::Sphere)], sphere, indices);

    glUseProgram (program_);
    glProgramUniform1i (program_, glGetUniformLocation (program_, "uDiffuseMap"), 0);
    glProgramUniform1i (program_, glGetUniformLocation (program_, "uNormalMap"), 1);
    glProgramUniform1i (program_, glGetUniformLocation (program_, "uPackedMap"), 2);
}

GLuint Renderer::loadTexture(const std::filesystem::path& filePath, bool srgb) {
    stbi_set_flip_vertically_on_load (1);
    int width = 0;
    int height = 0;
    int sourceChannels = 0;
    stbi_uc* pixels = stbi_load (filePath.string().c_str(), &width, &height, &sourceChannels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        throw std::runtime_error ("Unable to load texture " + filePath.string() + ": " + stbi_failure_reason());
    }

    GLuint texture = 0;
    glCreateTextures (GL_TEXTURE_2D, 1, &texture);
    const GLint levels = 1 + static_cast <GLint> (std::floor (
                             std::log2 (static_cast <float> (std::max (width, height)))
                         ));
    glTextureStorage2D (texture, levels, srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8, width, height);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glTextureSubImage2D (texture, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
    stbi_image_free (pixels);

    glTextureParameteri (texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri (texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri (texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri (texture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateTextureMipmap (texture);
    textures_.push_back (texture);
    return texture;
}

void Renderer::beginFrame(const glm::mat4& viewProjection, const glm::vec3& cameraPosition) {
    glUseProgram (program_);
    glProgramUniformMatrix4fv (program_, viewProjectionLocation_, 1, GL_FALSE, glm::value_ptr (viewProjection));
    glProgramUniform3fv (program_, cameraPositionLocation_, 1, glm::value_ptr (cameraPosition));
    constexpr glm::vec3 lightPosition { 0.0F, 3.62F, 0.0F };
    glProgramUniform3fv (program_, lightPositionLocation_, 1, glm::value_ptr (lightPosition));
}

void Renderer::draw(Primitive primitive, const glm::mat4& model, const Material& material) {
    const Mesh& mesh = meshes_[static_cast <std::size_t> (primitive)];
    setUniforms (model, material);
    glBindTextureUnit (0, material.diffuseTexture);
    glBindTextureUnit (1, material.normalTexture);
    glBindTextureUnit (2, material.packedTexture);
    glBindVertexArray (mesh.vertexArray);
    glDrawElements (GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
}

std::vector <Renderer::Vertex> Renderer::makePlaneVertices() {
    constexpr glm::vec3 normal { 0.0F, 0.0F, 1.0F };
    constexpr glm::vec3 tangent { 1.0F, 0.0F, 0.0F };
    constexpr glm::vec3 bitangent { 0.0F, 1.0F, 0.0F };
    return {
        { { -0.5F, -0.5F, 0.0F }, normal, tangent, { 0.0F, 0.0F } },
        { { 0.5F, -0.5F, 0.0F }, normal, tangent, { 1.0F, 0.0F } },
        { { 0.5F, 0.5F, 0.0F }, normal, tangent, { 1.0F, 1.0F } },
        { { -0.5F, 0.5F, 0.0F }, normal, tangent, { 0.0F, 1.0F } }
    };
}

std::vector <Renderer::Vertex> Renderer::makeCubeVertices(std::vector <unsigned int>& indices) {
    std::vector <Vertex> vertices;
    const std::array <std::pair <glm::vec3, glm::vec3>, 6> faceBasis {
        {
            { { 0.0F, 0.0F, 1.0F }, { 1.0F, 0.0F, 0.0F } },
            { { 0.0F, 0.0F, -1.0F }, { -1.0F, 0.0F, 0.0F } },
            { { 1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, -1.0F } },
            { { -1.0F, 0.0F, 0.0F }, { 0.0F, 0.0F, 1.0F } },
            { { 0.0F, 1.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
            { { 0.0F, -1.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } }
        }
    };

    for (const auto& [normal, tangent]: faceBasis) {
        const glm::vec3 bitangent = glm::cross (normal, tangent);
        const glm::vec3 center = normal * 0.5F;
        const unsigned int base = static_cast <unsigned int> (vertices.size());
        vertices.push_back ({ center - tangent * 0.5F - bitangent * 0.5F, normal, tangent, { 0.0F, 0.0F } });
        vertices.push_back ({ center + tangent * 0.5F - bitangent * 0.5F, normal, tangent, { 1.0F, 0.0F } });
        vertices.push_back ({ center + tangent * 0.5F + bitangent * 0.5F, normal, tangent, { 1.0F, 1.0F } });
        vertices.push_back ({ center - tangent * 0.5F + bitangent * 0.5F, normal, tangent, { 0.0F, 1.0F } });
        indices.insert (indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
    }
    return vertices;
}

std::vector <Renderer::Vertex> Renderer::makeCylinderVertices(std::vector <unsigned int>& indices, bool cone) {
    constexpr int segmentCount = 24;
    std::vector <Vertex> vertices;
    vertices.reserve (segmentCount * 4 + (segmentCount + 1) * 2);
    for (int segment = 0; segment < segmentCount; ++segment) {
        const float angle = static_cast <float> (segment) / static_cast <float> (segmentCount) * 6.28318530718F;
        const float x = std::cos (angle);
        const float z = std::sin (angle);
        const float nextAngle = static_cast <float> (segment + 1) / static_cast <float> (segmentCount) * 6.28318530718F;
        const float nextX = std::cos (nextAngle);
        const float nextZ = std::sin (nextAngle);
        const unsigned int base = static_cast <unsigned int> (vertices.size());
        const glm::vec3 normal = normalizeOr ({ x, cone ? 0.5F : 0.0F, z }, { x, 0.0F, z });
        const glm::vec3 nextNormal = normalizeOr ({ nextX, cone ? 0.5F : 0.0F, nextZ }, { nextX, 0.0F, nextZ });
        const glm::vec3 tangent = normalizeOr ({ -z, 0.0F, x }, { 1.0F, 0.0F, 0.0F });
        const glm::vec3 nextTangent = normalizeOr ({ -nextZ, 0.0F, nextX }, { 1.0F, 0.0F, 0.0F });
        vertices.push_back (
            { { x * 0.5F, -0.5F, z * 0.5F }, normal, tangent, { static_cast <float> (segment) / segmentCount, 0.0F } }
        );
        vertices.push_back (
            {
                { x * (cone ? 0.001F : 0.5F), 0.5F, z * (cone ? 0.001F : 0.5F) },
                normal,
                tangent,
                { static_cast <float> (segment) / segmentCount, 1.0F }
            }
        );
        vertices.push_back (
            {
                { nextX * 0.5F, -0.5F, nextZ * 0.5F },
                nextNormal,
                nextTangent,
                { static_cast <float> (segment + 1) / segmentCount, 0.0F }
            }
        );
        vertices.push_back (
            {
                { nextX * (cone ? 0.001F : 0.5F), 0.5F, nextZ * (cone ? 0.001F : 0.5F) },
                nextNormal,
                nextTangent,
                { static_cast <float> (segment + 1) / segmentCount, 1.0F }
            }
        );
        indices.insert (indices.end(), { base, base + 1, base + 2, base + 2, base + 1, base + 3 });
    }

    const unsigned int bottomCenter = static_cast <unsigned int> (vertices.size());
    vertices.push_back ({ { 0.0F, -0.5F, 0.0F }, { 0.0F, -1.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.5F, 0.5F } });
    const unsigned int bottomRim = static_cast <unsigned int> (vertices.size());
    for (int segment = 0; segment <= segmentCount; ++segment) {
        const float angle = static_cast <float> (segment) / static_cast <float> (segmentCount) * 6.28318530718F;
        const float x = std::cos (angle);
        const float z = std::sin (angle);
        vertices.push_back (
            {
                { x * 0.5F, -0.5F, z * 0.5F },
                { 0.0F, -1.0F, 0.0F },
                { 1.0F, 0.0F, 0.0F },
                { x * 0.5F + 0.5F, z * 0.5F + 0.5F }
            }
        );
    }
    for (int segment = 0; segment < segmentCount; ++segment) {
        indices.insert (
            indices.end(),
            {
                bottomCenter,
                bottomRim + static_cast <unsigned int> (segment),
                bottomRim + static_cast <unsigned int> (segment + 1)
            }
        );
    }

    if (!cone) {
        const unsigned int topCenter = static_cast <unsigned int> (vertices.size());
        vertices.push_back ({ { 0.0F, 0.5F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.5F, 0.5F } });
        const unsigned int topRim = static_cast <unsigned int> (vertices.size());
        for (int segment = 0; segment <= segmentCount; ++segment) {
            const float angle = static_cast <float> (segment) / static_cast <float> (segmentCount) * 6.28318530718F;
            const float x = std::cos (angle);
            const float z = std::sin (angle);
            vertices.push_back (
                {
                    { x * 0.5F, 0.5F, z * 0.5F },
                    { 0.0F, 1.0F, 0.0F },
                    { 1.0F, 0.0F, 0.0F },
                    { x * 0.5F + 0.5F, z * 0.5F + 0.5F }
                }
            );
        }
        for (int segment = 0; segment < segmentCount; ++segment) {
            indices.insert (
                indices.end(),
                {
                    topCenter,
                    topRim + static_cast <unsigned int> (segment + 1),
                    topRim + static_cast <unsigned int> (segment)
                }
            );
        }
    }
    return vertices;
}

std::vector <Renderer::Vertex> Renderer::makeSphereVertices(std::vector <unsigned int>& indices) {
    constexpr int latitudeCount = 12;
    constexpr int longitudeCount = 20;
    std::vector <Vertex> vertices;
    for (int latitude = 0; latitude <= latitudeCount; ++latitude) {
        const float v = static_cast <float> (latitude) / latitudeCount;
        const float phi = v * 3.14159265359F;
        for (int longitude = 0; longitude <= longitudeCount; ++longitude) {
            const float u = static_cast <float> (longitude) / longitudeCount;
            const float theta = u * 6.28318530718F;
            const glm::vec3 normal {
                std::sin (phi) * std::cos (theta),
                std::cos (phi),
                std::sin (phi) * std::sin (theta)
            };
            const glm::vec3 tangent = normalizeOr ({ -std::sin (theta), 0.0F, std::cos (theta) }, { 1.0F, 0.0F, 0.0F });
            vertices.push_back ({ normal * 0.5F, normal, tangent, { u, 1.0F - v } });
        }
    }
    for (int latitude = 0; latitude < latitudeCount; ++latitude) {
        for (int longitude = 0; longitude < longitudeCount; ++longitude) {
            const unsigned int first = static_cast <unsigned int> (latitude * (longitudeCount + 1) + longitude);
            const unsigned int second = first + static_cast <unsigned int> (longitudeCount + 1);
            indices.insert (indices.end(), { first, first + 1, second, first + 1, second + 1, second });
        }
    }
    return vertices;
}

void Renderer::createMesh(Mesh& mesh, const std::vector <Vertex>& vertices, const std::vector <unsigned int>& indices) {
    glCreateVertexArrays (1, &mesh.vertexArray);
    glCreateBuffers (1, &mesh.vertexBuffer);
    glCreateBuffers (1, &mesh.indexBuffer);
    glNamedBufferStorage (
        mesh.vertexBuffer,
        static_cast <GLsizeiptr> (vertices.size() * sizeof (Vertex)),
        vertices.data(),
        0
    );
    glNamedBufferStorage (
        mesh.indexBuffer,
        static_cast <GLsizeiptr> (indices.size() * sizeof (unsigned int)),
        indices.data(),
        0
    );
    glVertexArrayVertexBuffer (mesh.vertexArray, 0, mesh.vertexBuffer, 0, static_cast <GLsizei> (sizeof (Vertex)));
    glVertexArrayElementBuffer (mesh.vertexArray, mesh.indexBuffer);
    glEnableVertexArrayAttrib (mesh.vertexArray, 0);
    glEnableVertexArrayAttrib (mesh.vertexArray, 1);
    glEnableVertexArrayAttrib (mesh.vertexArray, 2);
    glEnableVertexArrayAttrib (mesh.vertexArray, 3);
    glVertexArrayAttribFormat (
        mesh.vertexArray,
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast <GLuint> (offsetof (Vertex, position))
    );
    glVertexArrayAttribFormat (
        mesh.vertexArray,
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast <GLuint> (offsetof (Vertex, normal))
    );
    glVertexArrayAttribFormat (
        mesh.vertexArray,
        2,
        3,
        GL_FLOAT,
        GL_FALSE,
        static_cast <GLuint> (offsetof (Vertex, tangent))
    );
    glVertexArrayAttribFormat (
        mesh.vertexArray,
        3,
        2,
        GL_FLOAT,
        GL_FALSE,
        static_cast <GLuint> (offsetof (Vertex, textureCoordinate))
    );
    for (GLuint attribute = 0; attribute < 4; ++attribute) {
        glVertexArrayAttribBinding (mesh.vertexArray, attribute, 0);
    }
    mesh.indexCount = static_cast <GLsizei> (indices.size());
}

void Renderer::createProgram(const std::filesystem::path& projectRoot) {
    const auto vertexPath = projectRoot / "Source/Shaders/Vertex/Scene.vert.glsl";
    const auto fragmentPath = projectRoot / "Source/Shaders/Fragment/Scene.frag.glsl";
    const GLuint vertexShader = compileShader (GL_VERTEX_SHADER, readTextFile (vertexPath), vertexPath);
    const GLuint fragmentShader = compileShader (GL_FRAGMENT_SHADER, readTextFile (fragmentPath), fragmentPath);
    program_ = glCreateProgram();
    glAttachShader (program_, vertexShader);
    glAttachShader (program_, fragmentShader);
    glLinkProgram (program_);
    glDeleteShader (vertexShader);
    glDeleteShader (fragmentShader);

    GLint successful = GL_FALSE;
    glGetProgramiv (program_, GL_LINK_STATUS, &successful);
    if (successful == GL_FALSE) {
        GLint logLength = 0;
        glGetProgramiv (program_, GL_INFO_LOG_LENGTH, &logLength);
        std::string log (static_cast <std::size_t> (logLength), '\0');
        glGetProgramInfoLog (program_, logLength, nullptr, log.data());
        throw std::runtime_error ("Shader program link failed:\n" + log);
    }

    viewProjectionLocation_ = glGetUniformLocation (program_, "uViewProjection");
    modelLocation_ = glGetUniformLocation (program_, "uModel");
    normalMatrixLocation_ = glGetUniformLocation (program_, "uNormalMatrix");
    cameraPositionLocation_ = glGetUniformLocation (program_, "uCameraPosition");
    colorLocation_ = glGetUniformLocation (program_, "uMaterialColor");
    uvScaleLocation_ = glGetUniformLocation (program_, "uUvScale");
    hasDiffuseLocation_ = glGetUniformLocation (program_, "uHasDiffuse");
    hasNormalLocation_ = glGetUniformLocation (program_, "uHasNormalMap");
    hasPackedLocation_ = glGetUniformLocation (program_, "uHasPackedMap");
    emissiveLocation_ = glGetUniformLocation (program_, "uEmissive");
    ambientFloorLocation_ = glGetUniformLocation (program_, "uAmbientFloor");
    directLightScaleLocation_ = glGetUniformLocation (program_, "uDirectLightScale");
    lightPositionLocation_ = glGetUniformLocation (program_, "uLightPosition");
}

void Renderer::setUniforms(const glm::mat4& model, const Material& material) {
    const glm::mat3 normalMatrix = glm::transpose (glm::inverse (glm::mat3 (model)));
    glProgramUniformMatrix4fv (program_, modelLocation_, 1, GL_FALSE, glm::value_ptr (model));
    glProgramUniformMatrix3fv (program_, normalMatrixLocation_, 1, GL_FALSE, glm::value_ptr (normalMatrix));
    glProgramUniform3fv (program_, colorLocation_, 1, glm::value_ptr (material.color));
    glProgramUniform2fv (program_, uvScaleLocation_, 1, glm::value_ptr (material.uvScale));
    glProgramUniform1i (program_, hasDiffuseLocation_, material.diffuseTexture != 0 ? GL_TRUE : GL_FALSE);
    glProgramUniform1i (program_, hasNormalLocation_, material.normalTexture != 0 ? GL_TRUE : GL_FALSE);
    glProgramUniform1i (program_, hasPackedLocation_, material.packedTexture != 0 ? GL_TRUE : GL_FALSE);
    glProgramUniform1f (program_, emissiveLocation_, material.emissive);
    glProgramUniform1f (program_, ambientFloorLocation_, material.ambientFloor);
    glProgramUniform1f (program_, directLightScaleLocation_, material.directLightScale);
}
