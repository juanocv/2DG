// main.cpp

#include "abcg.hpp"
#include "window.hpp"

int main(int argc, char **argv) {
  try {
    abcg::Application app(argc, argv);
    Window window;
    window.setWindowSettings({
        .width = 800,
        .height = 600,
        .showFPS = true,
        .showFullscreenButton = false,
        .title = "Gerador de Grafos Conectados",
    });
    app.run(window);
  } catch (std::exception const &e) {
    fmt::print("Exception: {}\n", e.what());
    return -1;
  }
  return 0;
}
