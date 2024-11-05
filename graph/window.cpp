// window.cpp

#include "window.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <random>
#include <stack>

void Window::onCreate() {
  // Carrega os shaders para nós e arestas
  auto const assetsPath{abcg::Application::getAssetsPath()};

  abcg::ShaderSource vertexShader;
  vertexShader.source = assetsPath + "node.vert";
  vertexShader.stage = abcg::ShaderStage::Vertex;

  abcg::ShaderSource fragmentShader;
  fragmentShader.source = assetsPath + "node.frag";
  fragmentShader.stage = abcg::ShaderStage::Fragment;

  m_program = abcg::createOpenGLProgram({vertexShader, fragmentShader});

  // Adquire as localizações uniformes para o shader
  m_colorLoc = glGetUniformLocation(m_program, "color");
  m_translationLoc = glGetUniformLocation(m_program, "translation");
  m_scaleLoc = glGetUniformLocation(m_program, "scale");
  m_projMatrixLoc = glGetUniformLocation(m_program, "projMatrix");

  // Cria nós e arestas
  createNodes();
  createEdges();
  computeNodeDegrees();
  setupModel();

  // Define a cor do plano de fundo para branco
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

  // Carrega a textura da fonte
  abcg::OpenGLTextureCreateInfo textureInfo{.path = assetsPath + "font.png",
                                            .generateMipmaps = false,
                                            .flipUpsideDown = false};
  m_fontTexture = abcg::loadOpenGLTexture(textureInfo);

  // Define parâmetros de textura
  glBindTexture(GL_TEXTURE_2D, m_fontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  // Cria um programa de shader para renderização dos textos
  m_textProgram = abcg::createOpenGLProgram(
      {{.source = assetsPath + "text.vert", .stage = abcg::ShaderStage::Vertex},
       {.source = assetsPath + "text.frag",
        .stage = abcg::ShaderStage::Fragment}});

  // Adquire as localizações uniformes para o programa de shader
  m_textColorLoc = glGetUniformLocation(m_textProgram, "textColor");
  m_textProjMatrixLoc = glGetUniformLocation(m_textProgram, "projMatrix");
  m_fontTextureLoc = glGetUniformLocation(m_textProgram, "fontTexture");

  // Ativa o texto do programa de shader e define a textura da fonte como uniforme
  glUseProgram(m_textProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_fontTexture);
  glUniform1i(m_fontTextureLoc, 0);
  glUseProgram(0);

  // Habilita blending para transparência
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Inicializa dados de caracteres e define a renderização de texto VAO/VBO
  initCharacters();
  setupTextRendering();
}

void Window::initCharacters() {
  // Define as coordenadas de textura para cada dígito (0-9)
  // Cada número tem 10x20 pixels de proporção em uma textura 64x64
  const float charWidth = 10.0f / 64.0f;
  const float charHeight = 20.0f / 64.0f;

  for (int i = 0; i < 10; ++i) {
    float x = (i % 6) * charWidth;  // 6 números na primeira fileira
    float y = (i / 6) * charHeight; // O restante na segunda fileira

    // Ajusta as coordenadas de textura em sentido horário do canto inferior esquerdo
    m_characters[i].texCoords[0] = {x, y};                     // Bottom-left
    m_characters[i].texCoords[1] = {x + charWidth, y};         // Bottom-right
    m_characters[i].texCoords[2] = {x + charWidth, y + charHeight}; // Top-right
    m_characters[i].texCoords[3] = {x, y + charHeight};        // Top-left
    
    m_characters[i].advance = 0.1f; // Normaliza o avanço de largura
  }
}

void Window::setupTextRendering() {
  glGenVertexArrays(1, &m_VAO_text);
  glGenBuffers(1, &m_VBO_text);

  glBindVertexArray(m_VAO_text);

  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_text);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, nullptr,
               GL_DYNAMIC_DRAW);

  // Atributos do vértice
  glEnableVertexAttribArray(0); // Posição
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

  glEnableVertexAttribArray(1); // Coordenada da textura
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
                        (void *)(2 * sizeof(GLfloat)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void Window::renderText(std::string text, glm::vec2 position) {
  float scale = 0.05f; // Tamanho dos números (texto)

  // Centraliza o texto em relação à posição dos nós
  float textWidth = text.length() * scale;
  float xOffset = -textWidth / 2.0f;
  float yOffset = -scale;

  std::vector<GLfloat> vertices;

  float x = position.x + xOffset;
  float y = position.y + yOffset;

  for (char c : text) {
    if (c < '0' || c > '9')
      continue;

    Character ch = m_characters[c - '0'];

    float w = scale;
    float h = scale * 2.0f; // Mantém o aspect ratio 1:2

    // Define os vértices quad com as coordenadas corretas da textura
    GLfloat quadVertices[] = {
        // Posições        // Coordenadas de textura
        x,     y + h, ch.texCoords[0].x, ch.texCoords[0].y, // Superior-esquerda
        x + w, y + h, ch.texCoords[1].x, ch.texCoords[1].y, // Superior-direita
        x + w, y,     ch.texCoords[2].x, ch.texCoords[2].y, // inferior-direita

        x,     y + h, ch.texCoords[0].x, ch.texCoords[0].y, // Superior-esquerda
        x + w, y,     ch.texCoords[2].x, ch.texCoords[2].y, // Inferior-direita
        x,     y,     ch.texCoords[3].x, ch.texCoords[3].y  // Inferior-esquerda
    };

    vertices.insert(vertices.end(), std::begin(quadVertices),
                    std::end(quadVertices));
    x += w; // Avança o cursor
  }

  // Atualiza os dados do vértice
  glBindBuffer(GL_ARRAY_BUFFER, m_VBO_text);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat),
               vertices.data(), GL_DYNAMIC_DRAW);

  // Desenha o texto
  glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 4));
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

  // Habilita blending para texto
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Define renderização do texto
  glUseProgram(m_textProgram);
  glUniformMatrix4fv(m_textProjMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);
  glUniform3f(m_textColorLoc, 0.0f, 0.0f, 0.0f); // Black text

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_fontTexture);

  glBindVertexArray(m_VAO_text);

  // Renderiza os rótulos dos nós
  for (size_t i = 0; i < m_nodes.size(); ++i) {
    renderText(std::to_string(i), m_nodes[i].position);
  }

  // Limpeza
  glBindVertexArray(0);
  glUseProgram(0);
  glDisable(GL_BLEND);
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

  glDeleteProgram(m_textProgram);
  glDeleteTextures(1, &m_fontTexture);
  glDeleteBuffers(1, &m_VBO_text);
  glDeleteVertexArrays(1, &m_VAO_text);
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
  // Criar modelo de círculo para os vértices
  m_circleData.clear();

  // Adicionar o centro do vértice
  m_circleData.push_back(glm::vec2(0.0f, 0.0f)); // Center of the circle

  // Gerar pontos de perímetro
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