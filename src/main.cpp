#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>

#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imfilebrowser.h"
#include <nex/nes.h>

#ifdef __APPLE__
#define PRIMARY_MOD "Cmd"
#else
#define PRIMARY_MOD "Ctrl"
#endif

struct Window {
  GLFWwindow* handle;
  int width;
  int height;
};

struct AppActions {
  void (*quit)(void* ctx);
  void (*load_rom)(void* ctx, const char* path);
  void (*reset)(void* ctx);

  void* ctx;
};

struct UiState {
  ImGui::FileBrowser open_rom_dialog;
  bool cpu_pane_visible = false;
  bool is_running = false;
  bool is_rom_loaded = false;

  UiState() : open_rom_dialog(0) {}
};

struct Emulator {
  NES* nes;
};

struct UiContext {
  AppActions* actions;
  Emulator* emu;
  UiState state;
};

struct App {
  Window window;
  Emulator emulator;
  UiContext ui;
  AppActions actions;
};

void show_ui(UiContext& ui);
void show_menu_bar(UiContext& ui);
void show_cpu_pane(UiContext& ui);
void update_window_title(App* app, const char* rom_path);

void app_quit(void* ctx) {
  App* app = (App*)ctx;

  glfwSetWindowShouldClose(app->window.handle, GLFW_TRUE);
}

void app_load_rom(void* ctx, const char* path) {
  App* app = (App*)ctx;

  if (nex_load_rom(app->emulator.nes, path) != 0) {
    // TODO: better error handling
    fprintf(stderr, "NEX: Failed to load ROM\n");
  }

  nex_reset(app->emulator.nes);

  app->ui.state.is_rom_loaded = true;
  update_window_title(app, path);
}

void app_reset_emulator(void* ctx) {
  App* app = (App*)ctx;

  nex_reset(app->emulator.nes);
}

void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int window_init(Window& window) {
  glfwSetErrorCallback(glfw_error_callback);

  if (!glfwInit()) {
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  float main_scale =
      ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
  window.width = 256 * 3 * main_scale;
  window.height = 224 * 3 * main_scale;
  window.handle =
      glfwCreateWindow(window.width, window.height, "nex", NULL, NULL);
  if (!window.handle) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window.handle);
  if (!gladLoadGL()) {
    std::cerr << "GLAD Error: Failed to initialize GLAD\n";
  }
  glfwSwapInterval(1);

  return 0;
};

int actions_init(App& app) {
  app.actions = {
      .quit = app_quit,
      .load_rom = app_load_rom,
      .reset = app_reset_emulator,

      .ctx = &app,
  };
  return 0;
}

int ui_init(UiContext& ui, Window& window, AppActions& actions, Emulator& emu) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window.handle, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // (optional) set browser properties
  ui.state.open_rom_dialog.SetTitle("Open ROM");
  ui.state.open_rom_dialog.SetTypeFilters({".nes"});

  ui.actions = &actions;
  ui.emu = &emu;

  return 0;
}

int emulator_init(Emulator& emulator) {
  emulator.nes = nex_create();
  if (!emulator.nes) {
    return -1;
  }

  return 0;
}

int app_init(App& app) {
  if (window_init(app.window) != 0) {
    return -1;
  }

  if (emulator_init(app.emulator) != 0) {
    return -1;
  }

  if (actions_init(app) != 0) {
    return -1;
  }

  if (ui_init(app.ui, app.window, app.actions, app.emulator) != 0) {
    return -1;
  }

  return 0;
}

int app_run(App& app) {
  while (!glfwWindowShouldClose(app.window.handle)) {
    // Poll events
    glfwPollEvents();

    // setup window
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // rendering
    show_ui(app.ui);

    if (app.ui.state.is_rom_loaded && app.ui.state.is_running) {
      nex_step(app.emulator.nes);
    }

    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(app.window.handle, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow* backup_current_context = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(app.window.handle);
  }

  return 0;
}

void app_destroy(App& app) {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(app.window.handle);
  glfwTerminate();

  nex_free(app.emulator.nes);
}

int main() {
  App app{};
  if (app_init(app) != 0) {
    return EXIT_FAILURE;
  }

  app_run(app);

  app_destroy(app);
  return EXIT_SUCCESS;
}

void handle_shortcuts(UiContext& ui) {
  ImGuiIO& io = ImGui::GetIO();

  // Don't steal shortcuts while typing in text boxes.
  if (io.WantTextInput) {
    return;
  }

  bool mod = io.KeyCtrl || io.KeySuper;  // Ctrl on Windows/Linux, Cmd on macOS

  if (mod && ImGui::IsKeyPressed(ImGuiKey_O)) {
    ui.state.open_rom_dialog.Open();
  }

  if (mod && ImGui::IsKeyPressed(ImGuiKey_Q)) {
    ui.actions->quit(ui.actions->ctx);
  }

  if (mod && ImGui::IsKeyPressed(ImGuiKey_R)) {
    ui.actions->reset(ui.actions->ctx);
  }
}

void show_ui(UiContext& ui) {
  handle_shortcuts(ui);
  show_menu_bar(ui);

  if (ui.state.cpu_pane_visible) {
    show_cpu_pane(ui);
  }
}

void show_menu_bar(UiContext& ui) {
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open ROM", PRIMARY_MOD "+O")) {
        ui.state.open_rom_dialog.Open();
      }

      if (ImGui::MenuItem("Quit", PRIMARY_MOD "+Q")) {
        ui.actions->quit(ui.actions->ctx);
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("CPU", nullptr, &ui.state.cpu_pane_visible);
      ImGui::MenuItem("PPU", nullptr, nullptr);
      ImGui::MenuItem("Memory", nullptr, nullptr);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Emulation")) {
      if (ImGui::MenuItem(ui.state.is_running ? "Pause" : "Resume")) {
        ui.state.is_running = !ui.state.is_running;
      }

      if (ImGui::MenuItem("Reset", PRIMARY_MOD "+R")) {
        ui.actions->reset(ui.actions->ctx);
      }

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }

  ui.state.open_rom_dialog.Display();

  if (ui.state.open_rom_dialog.HasSelected()) {
    ui.actions->load_rom(
        ui.actions->ctx,
        ui.state.open_rom_dialog.GetSelected().string().c_str());
    ui.state.open_rom_dialog.ClearSelected();
  }
}

void show_cpu_pane(UiContext& ui) {
  if (!ImGui::Begin("CPU", &ui.state.cpu_pane_visible)) {
    ImGui::End();
    return;
  }

  ImGui::SeparatorText("Registers");

  NexCpuState state = nex_get_cpu_state(ui.emu->nes);

  ImGui::Text("PC  : %04X", state.PC);
  ImGui::Text("A   : %02X", state.A);
  ImGui::Text("X   : %02X", state.X);
  ImGui::Text("Y   : %02X", state.Y);
  ImGui::Text("P   : %02X", state.P);
  ImGui::Text("SP  : %02X", state.SP);
  ImGui::Separator();

  ImGui::Text("PPU : (%03d, %03d)", state.scanline, state.dot);
  ImGui::SameLine();
  ImGui::Text("CYC : %llu", state.total_cycles);

  ImGui::Separator();

  if (!ui.state.is_rom_loaded) {
    ImGui::BeginDisabled(true);
  }

  if (ImGui::Button("Step")) {
    nex_step(ui.emu->nes);
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    ui.actions->reset(ui.actions->ctx);
  }

  if (!ui.state.is_rom_loaded) {
    ImGui::EndDisabled();
  }

  if (ui.state.is_rom_loaded) {
    ImGui::SeparatorText("Disassembly");
    NexDisasmLine line;
    if (!nex_disassemble_at(ui.emu->nes, state.PC, &line)) {
      // handle error
    }

    char bytes_str[10];
    size_t pos = 0;

    for (uint8_t i = 0; i < line.bytes_count; i++) {
      pos += snprintf(bytes_str + pos, sizeof(bytes_str) - pos, "%02X ",
                      line.bytes[i]);
    }

    ImGui::Text("%04X  %-8s %-3s %s", line.addr, bytes_str, line.mnemonic,
                line.operand);
  }

  ImGui::End();
}

void update_window_title(App* app, const char* rom_path) {
  std::filesystem::path path(rom_path);

  std::string title = "nex - ";
  title += path.filename().string();

  glfwSetWindowTitle(app->window.handle, title.c_str());
}