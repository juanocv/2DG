// window.hpp

#ifndef WINDOW_HPP_
#define WINDOW_HPP_

#include "abcgOpenGL.hpp"
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

class Window : public abcg::OpenGLWindow {
protected:
  void onCreate() override;
  void onPaint() override;
  void onPaintUI() override;
  void onResize(const glm::ivec2 &size) override;
  void onDestroy() override;

private:
  struct Node {
    glm::vec2 position;
    int degree{0};
  };

  struct Edge {
    int nodeA;
    int nodeB;
  };

  std::vector<Node> m_nodes;
  std::vector<Edge> m_edges;
  int m_numNodes{5};                       // Número padrão de nós
  float m_nodeRadius{0.05f};               // Raio padrão dos nós
  glm::vec3 m_nodeColor{1.0f, 0.0f, 0.0f}; // Cor padrão dos nós (vermelho)
  bool m_connectedGraph{true};             // Indica se o grafo é conectado

  GLuint m_program{};
  GLint m_colorLoc{};
  GLint m_translationLoc{};
  GLint m_scaleLoc{};
  GLint m_projMatrixLoc{};

  GLuint m_VAO_nodes{};
  GLuint m_VBO_nodes{};
  GLuint m_VAO_edges{};
  GLuint m_VBO_edges{};

  GLuint m_fontTexture{};
  GLint m_fontTextureLoc{};
  GLuint m_textProgram{};
  GLint m_textColorLoc{};
  GLint m_textProjMatrixLoc{};

  GLuint m_VAO_text{};
  GLuint m_VBO_text{};

  struct Character {
    glm::vec2 texCoords[4]; // Texture coordinates for the quad
    float advance;          // How much to move after rendering this character
  };

  // Map digits to Character data
  std::array<Character, 10> m_characters;

  glm::ivec2 m_viewportSize{};

  int m_circlePoints{100};

  std::vector<glm::vec2> m_circleData;
  glm::mat4 m_projMatrix{};

  void createNodes();
  void createEdges();
  void computeNodeDegrees();
  bool isGraphConnected(); // Função para verificar conectividade
  void setupModel();
  void renderText(std::string text, glm::vec2 position);
  void initCharacters();
  void setupTextRendering();
};

#endif
