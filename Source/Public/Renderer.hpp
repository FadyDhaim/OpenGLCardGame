#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

enum class Primitive {
    Plane,
    Cube,
    Cylinder,
    Cone,
    Sphere
};

struct Material {
    glm::vec3 color { 1.0F };
    GLuint diffuseTexture = 0;
    GLuint normalTexture = 0;
    GLuint packedTexture = 0;
    glm::vec2 uvScale { 1.0F };
    float emissive = 0.0F;
    float ambientFloor = 0.12F;
    float directLightScale = 1.0F;
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void initialize(const std::filesystem::path& projectRoot);
     GLuint loadTexture(const std::filesystem::path& filePath, bool srgb);
    void beginFrame(
        const glm::mat4& viewProjection,
        const glm::vec3& cameraPosition,
        const glm::vec3& lightPosition,
        const glm::vec3& lightColor,
        float lightFalloffDistance
    );
    void draw(Primitive primitive, const glm::mat4& model, const Material& material);

private:
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 textureCoordinate;
    };

    struct Mesh {
        GLuint vertexArray = 0;
        GLuint vertexBuffer = 0;
        GLuint indexBuffer = 0;
        GLsizei indexCount = 0;
    };

     static std::vector <Vertex> makePlaneVertices();
     static std::vector <Vertex> makeCubeVertices(std::vector <unsigned int>& indices);
     static std::vector <Vertex> makeCylinderVertices(std::vector <unsigned int>& indices, bool cone);
     static std::vector <Vertex> makeSphereVertices(std::vector <unsigned int>& indices);
    void createMesh(Mesh& mesh, const std::vector <Vertex>& vertices, const std::vector <unsigned int>& indices);
    void createProgram(const std::filesystem::path& projectRoot);
    void setUniforms(const glm::mat4& model, const Material& material);

    GLuint program_ = 0;
    std::array <Mesh, 5> meshes_ {};
    GLint viewProjectionLocation_ = -1;
    GLint modelLocation_ = -1;
    GLint normalMatrixLocation_ = -1;
    GLint cameraPositionLocation_ = -1;
    GLint colorLocation_ = -1;
    GLint uvScaleLocation_ = -1;
    GLint hasDiffuseLocation_ = -1;
    GLint hasNormalLocation_ = -1;
    GLint hasPackedLocation_ = -1;
    GLint emissiveLocation_ = -1;
    GLint ambientFloorLocation_ = -1;
    GLint directLightScaleLocation_ = -1;
    GLint lightPositionLocation_ = -1;
    GLint lightColorLocation_ = -1;
    GLint lightFalloffDistanceLocation_ = -1;
    std::vector <GLuint> textures_;
};
