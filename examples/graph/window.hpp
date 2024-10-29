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
  void onResize(const glm::ivec2& size) override;
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
  int m_numNodes{5};
  float m_nodeRadius{0.05f};
  glm::vec3 m_nodeColor{1.0f};

  GLuint m_program{};
  GLint m_colorLoc{};
  GLint m_translationLoc{};
  GLint m_scaleLoc{};
  GLint m_projMatrixLoc{};

  GLuint m_VAO_nodes{};
  GLuint m_VBO_nodes{};
  GLuint m_VAO_edges{};
  GLuint m_VBO_edges{};

  glm::ivec2 m_viewportSize{};

  int m_circlePoints{100};

  std::vector<glm::vec2> m_circleData;
  glm::mat4 m_projMatrix{};

  void createNodes();
  void createEdges();
  void computeNodeDegrees();
  void setupModel();
};

#endif
