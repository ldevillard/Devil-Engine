#include "data/mesh/WireCone.h"

#include "system/editor/Gizmo.h"

#pragma region Public Methods

WireCone::WireCone() : Mesh()
{
    generateCone(25);

	setupMesh();
}

void WireCone::Draw(Shader* shader) const
{
	glBindVertexArray(VAO);
	glLineWidth(Gizmo::GIZMO_WIDTH); // maybe set up in a constant file
	glDrawElements(GL_LINES, static_cast<GLsizei>(Indices.size()), GL_UNSIGNED_INT, 0);
	glLineWidth(1); // reset to default
	glBindVertexArray(0);
}

#pragma endregion

#pragma region Private Methods

void WireCone::generateCone(unsigned int edgeCount)
{
    // base radius and height
    float radius = 0.1f;
    float height = 0.2f;

    // base vertices
    Vertices.push_back(Vertex(glm::vec3(0.0f, -height / 2.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.5f, 0.5f)));
    float angleIncrement = glm::radians(360.0f / edgeCount);
    for (unsigned int i = 0; i < edgeCount; ++i) 
    {
        float angle = i * angleIncrement;
        Vertices.push_back(Vertex(glm::vec3(cos(angle) * radius, -height / 2.0f, sin(angle) * radius), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(cos(angle) * 0.5f + 0.5f, sin(angle) * 0.5f + 0.5f)));
    }

    Vertices.push_back(Vertex(glm::vec3(0.0f, height / 2.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.5f, 0.5f)));

    // connect base vertices to the apex
    for (unsigned int i = 1; i <= edgeCount; ++i) 
    {
        Indices.push_back(0);
        Indices.push_back(i);
    }
    Indices.push_back(0);
    Indices.push_back(1);

    // connect edges of the base
    for (unsigned int i = 1; i < edgeCount; ++i) 
    {
        Indices.push_back(i);
        Indices.push_back(i + 1);
    }
    Indices.push_back(edgeCount);
    Indices.push_back(1);

    // connect edges from the apex to the base
    for (unsigned int i = 1; i <= edgeCount; ++i) 
    {
        Indices.push_back(edgeCount + 1);
        Indices.push_back(i);
    }
}

#pragma endregion