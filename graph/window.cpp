// window.cpp

#include "window.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <random>
#include <stack>

void Window::onCreate() {
  // Carregar shaders
  auto const assetsPath{abcg::Application::getAssetsPath()};

  abcg::ShaderSource vertexShader;
  vertexShader.source = assetsPath + "node.vert";
  vertexShader.stage = abcg::ShaderStage::Vertex;

  abcg::ShaderSource fragmentShader;
  fragmentShader.source = assetsPath + "node.frag";
  fragmentShader.stage = abcg::ShaderStage::Fragment;

  m_program = abcg::createOpenGLProgram({vertexShader, fragmentShader});

  // Obter locais das variáveis uniformes
  m_colorLoc = glGetUniformLocation(m_program, "color");
  m_translationLoc = glGetUniformLocation(m_program, "translation");
  m_scaleLoc = glGetUniformLocation(m_program, "scale");
  m_projMatrixLoc = glGetUniformLocation(m_program, "projMatrix");

  // Criar nós iniciais e configurar o modelo
  createNodes();
  createEdges();
  computeNodeDegrees();
  setupModel();

  // Definir a cor de fundo como branco
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

#ifdef __EMSCRIPTEN__
  // Set up the resize callback
  emscripten_set_resize_callback(
      EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
      [](int eventType, const EmscriptenUiEvent *e, void *userData) -> EM_BOOL {
        Window *window = static_cast<Window *>(userData);
        window->onResize({static_cast<int>(e->windowInnerWidth),
                          static_cast<int>(e->windowInnerHeight)});
        return EM_TRUE;
      });
#endif
}

void Window::onPaint() {
  // Limpar o buffer de cor
  glClear(GL_COLOR_BUFFER_BIT);

  // Calcular a proporção de aspecto
  auto const aspectRatio = static_cast<float>(m_viewportSize.x) /
                           static_cast<float>(m_viewportSize.y);

  // Calcular a matriz de projeção
  if (aspectRatio >= 1.0f) {
    // Mais largo que alto
    m_projMatrix = glm::ortho(-aspectRatio, aspectRatio, -1.0f, 1.0f);
  } else {
    // Mais alto que largo
    m_projMatrix =
        glm::ortho(-1.0f, 1.0f, -1.0f / aspectRatio, 1.0f / aspectRatio);
  }

  glUseProgram(m_program);

  // Definir a matriz de projeção
  glUniformMatrix4fv(m_projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);

  // Desenhar arestas (linhas)
  glUniform3f(m_colorLoc, 0.0f, 0.0f, 0.0f); // Cor das arestas (preto)
  glUniform1f(m_scaleLoc, 1.0f);             // Sem escala para linhas
  glUniform2f(m_translationLoc, 0.0f, 0.0f); // Sem translação para linhas

  // Preparar dados para as arestas
  std::vector<glm::vec2> edgePositions;
  for (const auto &edge : m_edges) {
    glm::vec2 posA = m_nodes[edge.nodeA].position;
    glm::vec2 posB = m_nodes[edge.nodeB].position;

    edgePositions.push_back(posA);
    edgePositions.push_back(posB);
  }

  glBindVertexArray(m_VAO_edges);
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_edges);
  glBufferData(GL_ARRAY_BUFFER, edgePositions.size() * sizeof(glm::vec2),
               edgePositions.data(), GL_DYNAMIC_DRAW);

  glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(edgePositions.size()));

  // Desvincular
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Desenhar nós
  glUniform3f(m_colorLoc, m_nodeColor.r, m_nodeColor.g, m_nodeColor.b);
  glUniform1f(m_scaleLoc, m_nodeRadius);

  glBindVertexArray(m_VAO_nodes);

  for (const auto &node : m_nodes) {
    glUniform2f(m_translationLoc, node.position.x, node.position.y);
    glDrawArrays(GL_TRIANGLE_FAN, 0, m_circlePoints + 2);
  }

  // Desvincular
  glBindVertexArray(0);
  glUseProgram(0);
}

void Window::onPaintUI() {
  // Definir a próxima janela para começar minimizada
  ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);

  // Definir a próxima janela para auto redimensionar
  ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);

  ImGui::Begin("Configurações do Grafo", nullptr,
               ImGuiWindowFlags_AlwaysAutoResize);

  // Número de Nós
  ImGui::SliderInt("Número de Nós", &m_numNodes, 1, 10);

  // Raio dos Nós
  ImGui::SliderFloat("Raio dos Nós", &m_nodeRadius, 0.01f, 0.2f);

  // Cor dos Nós
  ImGui::ColorEdit3("Cor dos Nós", &m_nodeColor.r);

  // Opção para grafo conectado ou desconexo
  ImGui::Checkbox("Grafo Conectado", &m_connectedGraph);

  // Botão para gerar um novo grafo
  if (ImGui::Button("Novo Grafo")) {
    createNodes();
    createEdges();
    computeNodeDegrees();
  }

  ImGui::Separator();

  // Verificar se o grafo é conectado
  bool connected = isGraphConnected();

  ImGui::Text("Informações do Grafo");
  ImGui::Text("Tipo do Grafo: Não Dirigido");
  ImGui::Text("Conectividade do Grafo: %s",
              connected ? "Conectado" : "Desconexo");
  ImGui::Text("Total de Nós: %d", m_numNodes);
  ImGui::Text("Total de Arestas: %d", static_cast<int>(m_edges.size()));

  // Calcular grau médio
  int totalDegree = 0;
  for (const auto &node : m_nodes) {
    totalDegree += node.degree;
  }
  float averageDegree = static_cast<float>(totalDegree) / m_numNodes;
  ImGui::Text("Grau Médio: %.2f", averageDegree);

  ImGui::Separator();

  ImGui::Text("Grau dos Nós:");
  for (size_t i = 0; i < m_nodes.size(); ++i) {
    ImGui::Text("Nó %zu: %d", i, m_nodes[i].degree);
  }

  ImGui::Separator();

  // Exibir lista de adjacência
  ImGui::Text("Lista de Adjacência:");
  for (size_t i = 0; i < m_nodes.size(); ++i) {
    ImGui::Text("Nó %zu:", i);
    ImGui::SameLine();

    // Encontrar nós adjacentes
    std::vector<int> adjacentNodes;
    for (const auto &edge : m_edges) {
      if (edge.nodeA == static_cast<int>(i)) {
        adjacentNodes.push_back(edge.nodeB);
      } else if (edge.nodeB == static_cast<int>(i)) {
        adjacentNodes.push_back(edge.nodeA);
      }
    }

    if (adjacentNodes.empty()) {
      ImGui::Text(" Nenhum");
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

  // Renderizar rótulos dos nós
  ImGui::Begin("Rótulos dos Nós", nullptr,
               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoSavedSettings);

  // Atualiza a posição da janela e tamanho a cada frame
  ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetWindowSize(ImVec2(m_viewportSize.x, m_viewportSize.y),
                       ImGuiCond_Always);

  // Captura o ratio em pixels do dispositivo
  float dpiScale = 1.0f;
#ifdef __EMSCRIPTEN__
  dpiScale = emscripten_get_device_pixel_ratio();
#endif

  // Get the canvas size in CSS pixels
  float canvasWidth = m_viewportSize.x / dpiScale;
  float canvasHeight = m_viewportSize.y / dpiScale;

  // Update ImGui window size
  ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetWindowSize(ImVec2(canvasWidth, canvasHeight), ImGuiCond_Always);
  
  // Iniciar a janela ImGui para os rótulos dos nós
  ImGui::Begin("Rótulos dos Nós", nullptr,
               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                   ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoSavedSettings);

  // Definir a posição e o tamanho da janela para cobrir toda a viewport
  ImGui::SetWindowPos(ImVec2(0, 0), ImGuiCond_Always);
  ImGui::SetWindowSize(
      ImVec2(m_viewportSize.x / dpiScale, m_viewportSize.y / dpiScale),
      ImGuiCond_Always);

  for (size_t i = 0; i < m_nodes.size(); ++i) {
    // Transformar posição do nó para NDC
    glm::vec4 ndcPosition =
        m_projMatrix * glm::vec4(m_nodes[i].position, 0.0f, 1.0f);

    // Converter NDC para coordenadas de tela
    float x = (ndcPosition.x * 0.5f + 0.5f) * (m_viewportSize.x / dpiScale);
    float y = (-ndcPosition.y * 0.5f + 0.5f) * (m_viewportSize.y / dpiScale);

    // Calcular o tamanho do texto
    std::string labelText = std::to_string(i);
    ImVec2 textSize = ImGui::CalcTextSize(labelText.c_str());

    // Centralizar o texto
    float centeredX = x - textSize.x / 2.0f;
    float centeredY = y - textSize.y / 2.0f;

    // Definir a posição do cursor
    ImGui::SetCursorPos(ImVec2(centeredX, centeredY));

    // Definir a cor do texto para preto
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1));

    // Renderizar o rótulo
    ImGui::Text("%s", labelText.c_str());

    // Restaurar a cor do texto
    ImGui::PopStyleColor();
  }

  ImGui::End();
}

void Window::onResize(const glm::ivec2 &size) {
  // Get the device pixel ratio
  float dpiScale = 1.0f;
#ifdef __EMSCRIPTEN__
  dpiScale = emscripten_get_device_pixel_ratio();

  // Get the canvas size in CSS pixels
  double canvasWidthCss, canvasHeightCss;
  emscripten_get_element_css_size("canvas", &canvasWidthCss, &canvasHeightCss);

  // Calculate the canvas size in pixels
  int canvasWidth = static_cast<int>(canvasWidthCss * dpiScale);
  int canvasHeight = static_cast<int>(canvasHeightCss * dpiScale);

  // Adjust the viewport size
  m_viewportSize = glm::vec2(canvasWidth, canvasHeight);

  // Set the OpenGL viewport
  glViewport(0, 0, canvasWidth, canvasHeight);

  // Update ImGui display size and scaling
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(canvasWidthCss),
                          static_cast<float>(canvasHeightCss));
  io.DisplayFramebufferScale = ImVec2(dpiScale, dpiScale);
#else
  // For native builds
  m_viewportSize = glm::vec2(size);

  // Set the OpenGL viewport
  glViewport(0, 0, size.x, size.y);

  // Update ImGui display size and scaling
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize =
      ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
  io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
#endif
}

void Window::onDestroy() {
  glDeleteProgram(m_program);

  glDeleteBuffers(1, &m_VBO_nodes);
  glDeleteVertexArrays(1, &m_VAO_nodes);

  glDeleteBuffers(1, &m_VBO_edges);
  glDeleteVertexArrays(1, &m_VAO_edges);
}

void Window::createNodes() {
  // Gerar posições aleatórias para os nós
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
  // Limpar arestas existentes
  m_edges.clear();

  if (m_connectedGraph) {
    // Conectar cada nó ao próximo para garantir que o grafo seja conectado
    for (int i = 0; i < m_numNodes - 1; ++i) {
      m_edges.push_back({i, i + 1});
    }

    // Adicionar arestas aleatórias
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> nodeDis(0, m_numNodes - 1);

    int extraEdges = m_numNodes; // Número de arestas extras
    for (int i = 0; i < extraEdges; ++i) {
      int a = nodeDis(gen);
      int b = nodeDis(gen);
      if (a != b) {
        // Evitar arestas duplicadas
        if (std::none_of(m_edges.begin(), m_edges.end(), [&](const Edge &e) {
              return (e.nodeA == a && e.nodeB == b) ||
                     (e.nodeA == b && e.nodeB == a);
            })) {
          m_edges.push_back({a, b});
        }
      }
    }

  } else {
    // Dividir os nós em dois grupos
    int splitIndex = m_numNodes / 2;

    // Verificar se temos pelo menos dois nós em cada grupo
    if (splitIndex == 0 || splitIndex == m_numNodes) {
      // Não é possível dividir os nós em dois grupos, não adiciona arestas
    } else {
      // Adicionar arestas aleatórias dentro do primeiro grupo
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<int> nodeDis1(0, splitIndex - 1);

      int extraEdgesGroup1 =
          splitIndex; // Número de arestas extras para o primeiro grupo
      for (int i = 0; i < extraEdgesGroup1; ++i) {
        int a = nodeDis1(gen);
        int b = nodeDis1(gen);
        if (a != b) {
          // Evitar arestas duplicadas
          if (std::none_of(m_edges.begin(), m_edges.end(), [&](const Edge &e) {
                return (e.nodeA == a && e.nodeB == b) ||
                       (e.nodeA == b && e.nodeB == a);
              })) {
            m_edges.push_back({a, b});
          }
        }
      }

      // Adicionar arestas aleatórias dentro do segundo grupo
      std::uniform_int_distribution<int> nodeDis2(splitIndex, m_numNodes - 1);

      int extraEdgesGroup2 =
          m_numNodes -
          splitIndex; // Número de arestas extras para o segundo grupo
      for (int i = 0; i < extraEdgesGroup2; ++i) {
        int a = nodeDis2(gen);
        int b = nodeDis2(gen);
        if (a != b) {
          // Evitar arestas duplicadas
          if (std::none_of(m_edges.begin(), m_edges.end(), [&](const Edge &e) {
                return (e.nodeA == a && e.nodeB == b) ||
                       (e.nodeA == b && e.nodeB == a);
              })) {
            m_edges.push_back({a, b});
          }
        }
      }
    }
  }

  // Recalcular graus dos nós
  computeNodeDegrees();
}

void Window::computeNodeDegrees() {
  // Resetar graus
  for (auto &node : m_nodes) {
    node.degree = 0;
  }

  // Calcular graus com base nas arestas
  for (const auto &edge : m_edges) {
    m_nodes[edge.nodeA].degree++;
    m_nodes[edge.nodeB].degree++;
  }
}

bool Window::isGraphConnected() {
  if (m_nodes.empty())
    return true;

  // Vetor para marcar nós visitados
  std::vector<bool> visited(m_nodes.size(), false);

  // Iniciar DFS a partir do primeiro nó
  std::stack<int> stack;
  stack.push(0);
  visited[0] = true;

  while (!stack.empty()) {
    int current = stack.top();
    stack.pop();

    // Encontrar nós adjacentes
    for (const auto &edge : m_edges) {
      int neighbor = -1;
      if (edge.nodeA == current) {
        neighbor = edge.nodeB;
      } else if (edge.nodeB == current) {
        neighbor = edge.nodeA;
      }

      if (neighbor != -1 && !visited[neighbor]) {
        visited[neighbor] = true;
        stack.push(neighbor);
      }
    }
  }

  // Verificar se todos os nós foram visitados
  return std::all_of(visited.begin(), visited.end(), [](bool v) { return v; });
}

void Window::setupModel() {
  // Criar modelo de círculo para os nós
  m_circleData.clear();
  const float step = glm::two_pi<float>() / static_cast<float>(m_circlePoints);
  for (int i = 0; i <= m_circlePoints; ++i) {
    float angle = i * step;
    m_circleData.push_back(glm::vec2(std::cos(angle), std::sin(angle)));
  }

  // Criar VBO e VAO para os nós
  glGenBuffers(1, &m_VBO_nodes);
  glGenVertexArrays(1, &m_VAO_nodes);

  glBindVertexArray(m_VAO_nodes);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_nodes);
  glBufferData(GL_ARRAY_BUFFER, m_circleData.size() * sizeof(glm::vec2),
               m_circleData.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Desvincular
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Criar VBO e VAO para as arestas
  glGenBuffers(1, &m_VBO_edges);
  glGenVertexArrays(1, &m_VAO_edges);

  glBindVertexArray(m_VAO_edges);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_edges);
  // Atualizaremos os dados dinamicamente
  glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

  // Desvincular
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}