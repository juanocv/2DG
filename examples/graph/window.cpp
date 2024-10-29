// window.cpp

#include "window.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <random>

void Window::onCreate() {
  // Load shaders
  auto const assetsPath{abcg::Application::getAssetsPath()};

  abcg::ShaderSource vertexShader;
  vertexShader.source = assetsPath + "node.vert";
  vertexShader.stage = abcg::ShaderStage::Vertex;

  abcg::ShaderSource fragmentShader;
  fragmentShader.source = assetsPath + "node.frag";
  fragmentShader.stage = abcg::ShaderStage::Fragment;

  m_program = abcg::createOpenGLProgram({vertexShader, fragmentShader});

  // Get uniform locations
  m_colorLoc = glGetUniformLocation(m_program, "color");
  m_translationLoc = glGetUniformLocation(m_program, "translation");
  m_scaleLoc = glGetUniformLocation(m_program, "scale");
  m_projMatrixLoc = glGetUniformLocation(m_program, "projMatrix");

  // Create initial nodes and setup model
  createNodes();
  createEdges();
  computeNodeDegrees(); // Compute degrees after creating edges
  setupModel();
}

void Window::onPaint() {
  glClear(GL_COLOR_BUFFER_BIT);

  // Calculate aspect ratio
  auto const aspectRatio = static_cast<float>(m_viewportSize.x) /
                           static_cast<float>(m_viewportSize.y);

  // Compute projection matrix
  if (aspectRatio >= 1.0f) {
    // Wider than tall
    m_projMatrix = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f);
  } else {
    // Taller than wide
    m_projMatrix =
        glm::ortho(-1.0f, 1.0f, -1.0f / aspectRatio, 1.0f / aspectRatio);
  }

  glUseProgram(m_program);

  // Set the projection matrix
  glUniformMatrix4fv(m_projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);

  // Draw edges (lines)
  glUniform3f(m_colorLoc, 1.0f, 1.0f, 1.0f); // White color for edges
  glUniform1f(m_scaleLoc, 1.0f);             // No scaling for lines
  glUniform2f(m_translationLoc, 0.0f, 0.0f); // No translation for lines

  // Prepare data for edges
  std::vector<glm::vec2> edgePositions;
  for (const auto &edge : m_edges) {
    glm::vec2 posA = m_nodes[edge.nodeA].position;
    glm::vec2 posB = m_nodes[edge.nodeB].position;

    // Do NOT adjust positions for aspect ratio here

    edgePositions.push_back(posA);
    edgePositions.push_back(posB);
  }

  glBindVertexArray(m_VAO_edges);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_edges);
  glBufferData(GL_ARRAY_BUFFER, edgePositions.size() * sizeof(glm::vec2),
               edgePositions.data(), GL_DYNAMIC_DRAW);

  glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(edgePositions.size()));

  // Unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Draw nodes
  glUniform3f(m_colorLoc, m_nodeColor.r, m_nodeColor.g, m_nodeColor.b);
  glUniform1f(m_scaleLoc, m_nodeRadius);

  glBindVertexArray(m_VAO_nodes);

  for (const auto &node : m_nodes) {
    glUniform2f(m_translationLoc, node.position.x, node.position.y);
    glDrawArrays(GL_TRIANGLE_FAN, 0, m_circlePoints + 2);
  }

  // Unbind
  glBindVertexArray(0);
  glUseProgram(0);
}

void Window::onPaintUI() {
  // Set the next window to auto-resize
  ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);

  ImGui::Begin("Graph Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

  // Number of Nodes
  ImGui::SliderInt("Number of Nodes", &m_numNodes, 1, 10);

  // Node Radius
  ImGui::SliderFloat("Node Radius", &m_nodeRadius, 0.01f, 0.2f);

  // Node Color
  ImGui::ColorEdit3("Node Color", &m_nodeColor.r);

  // New Graph Button
  if (ImGui::Button("New Graph")) {
    createNodes();
    createEdges();
    computeNodeDegrees();
  }

  ImGui::Separator();

  // Display graph information
  ImGui::Text("Graph Information");
  ImGui::Text("Graph Type: Undirected");
  ImGui::Text("Graph Connectivity: Connected");
  ImGui::Text("Total Nodes: %d", m_numNodes);
  ImGui::Text("Total Edges: %d", static_cast<int>(m_edges.size()));

  // Calculate average degree
  int totalDegree = 0;
  for (const auto &node : m_nodes) {
    totalDegree += node.degree;
  }
  float averageDegree = static_cast<float>(totalDegree) / m_numNodes;
  ImGui::Text("Average Degree: %.2f", averageDegree);

  ImGui::Separator();

  ImGui::Text("Degrees of Nodes:");
  for (size_t i = 0; i < m_nodes.size(); ++i) {
    ImGui::Text("Node %zu: %d", i, m_nodes[i].degree);
  }

  ImGui::Separator();

  // Display adjacency list
  ImGui::Text("Adjacency List:");
  for (size_t i = 0; i < m_nodes.size(); ++i) {
    ImGui::Text("Node %zu:", i);
    ImGui::SameLine();

    // Find adjacent nodes
    std::vector<int> adjacentNodes;
    for (const auto &edge : m_edges) {
      if (edge.nodeA == static_cast<int>(i)) {
        adjacentNodes.push_back(edge.nodeB);
      } else if (edge.nodeB == static_cast<int>(i)) {
        adjacentNodes.push_back(edge.nodeA);
      }
    }

    if (adjacentNodes.empty()) {
      ImGui::Text(" None");
    } else {
      ImGui::Text(" ");
      for (size_t j = 0; j < adjacentNodes.size(); ++j) {
        ImGui::SameLine();
        ImGui::Text("%d", adjacentNodes[j]);
        if (j != adjacentNodes.size() - 1) {
          ImGui::SameLine();
          ImGui::Text(",");
        }
      }
    }
  }

  ImGui::End();

  // Begin a new ImGui window for node labels
  ImGui::Begin("Node Labels", nullptr,
               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoSavedSettings);

  ImGui::SetWindowPos(ImVec2(0, 0));
  ImGui::SetWindowSize(ImVec2(m_viewportSize.x, m_viewportSize.y));

  for (size_t i = 0; i < m_nodes.size(); ++i) {
    // Transform node position to NDC
    glm::vec4 ndcPosition =
        m_projMatrix * glm::vec4(m_nodes[i].position, 0.0f, 1.0f);

    // Convert NDC to screen coordinates
    float x =
        (ndcPosition.x * 0.5f + 0.5f) * static_cast<float>(m_viewportSize.x);
    float y =
        (-ndcPosition.y * 0.5f + 0.5f) * static_cast<float>(m_viewportSize.y);

    // Offset to position the label slightly above the node
    float labelOffsetY = -30.0f; // Adjust as needed
    float labelOffsetX = 5.0f;   // Adjust as needed

    // Set the cursor position and render the label
    ImGui::SetCursorPos(ImVec2(x + labelOffsetX, y + labelOffsetY));
    ImGui::Text("%zu", i);
  }

  ImGui::End(); // End of "Node Labels" window
}

void Window::onResize(const glm::ivec2 &size) {
  glViewport(0, 0, size.x, size.y);
  m_viewportSize = size;
}

void Window::onDestroy() {
  glDeleteProgram(m_program);

  glDeleteBuffers(1, &m_VBO_nodes);
  glDeleteVertexArrays(1, &m_VAO_nodes);

  glDeleteBuffers(1, &m_VBO_edges);
  glDeleteVertexArrays(1, &m_VAO_edges);
}

void Window::createNodes() {
  // Generate random positions for nodes
  m_nodes.clear();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> disX(-1.0f + m_nodeRadius,
                                             1.0f - m_nodeRadius);
  std::uniform_real_distribution<float> disY(-1.0f + m_nodeRadius,
                                             1.0f - m_nodeRadius);

  for (int i = 0; i < m_numNodes; ++i) {
    Node node;
    node.position = {disX(gen), disY(gen)};
    m_nodes.push_back(node);
  }
}

void Window::createEdges() {
  // Create edges to ensure the graph is connected
  m_edges.clear();

  // Connect each node to the next to form a path (ensuring connectivity)
  for (int i = 0; i < m_numNodes - 1; ++i) {
    m_edges.push_back({i, i + 1});
  }

  // Optionally, add some random edges to make the graph more interesting
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> nodeDis(0, m_numNodes - 1);

  int extraEdges = m_numNodes; // Add extra edges
  for (int i = 0; i < extraEdges; ++i) {
    int a = nodeDis(gen);
    int b = nodeDis(gen);
    if (a != b) {
      // Avoid duplicate edges
      if (std::none_of(m_edges.begin(), m_edges.end(), [&](const Edge &e) {
            return (e.nodeA == a && e.nodeB == b) ||
                   (e.nodeA == b && e.nodeB == a);
          })) {
        m_edges.push_back({a, b});
      }
    }
  }

  computeNodeDegrees(); // Recompute degrees after adding edges
}

void Window::computeNodeDegrees() {
  // Reset degrees
  for (auto &node : m_nodes) {
    node.degree = 0;
  }

  // Calculate degrees based on edges
  for (const auto &edge : m_edges) {
    m_nodes[edge.nodeA].degree++;
    m_nodes[edge.nodeB].degree++;
  }
}

void Window::setupModel() {
  // Create a circle model for nodes
  m_circleData.clear();
  const float step = glm::two_pi<float>() / static_cast<float>(m_circlePoints);
  for (int i = 0; i <= m_circlePoints; ++i) {
    float angle = i * step;
    m_circleData.push_back(glm::vec2(std::cos(angle), std::sin(angle)));
  }

  // Create VBO and VAO for nodes
  glGenBuffers(1, &m_VBO_nodes);
  glGenVertexArrays(1, &m_VAO_nodes);

  glBindVertexArray(m_VAO_nodes);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_nodes);
  glBufferData(GL_ARRAY_BUFFER, m_circleData.size() * sizeof(glm::vec2),
               m_circleData.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Create VBO and VAO for edges
  glGenBuffers(1, &m_VBO_edges);
  glGenVertexArrays(1, &m_VAO_edges);

  glBindVertexArray(m_VAO_edges);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_edges);
  // We'll update the data dynamically
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
