#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdio.h>
#include <getopt.h>

#include <iostream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_memory_editor.h"
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
  void (*step_instr)(void* ctx);
  void (*step_scanline)(void* ctx);
  void (*step_frame)(void* ctx);

  void* ctx;
};

struct UiState {
  ImGui::FileBrowser open_rom_dialog;

  bool cpu_pane_visible = false;
  bool memory_pane_visible = false;
  bool ppu_pane_visible = false;

  bool is_running = false;
  bool is_rom_loaded = false;

  MemoryEditor wram_mem_edit;
  MemoryEditor vram_mem_edit;

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
void show_ppu_pane(UiContext& ui);
void show_memory_pane(UiContext& ui);
void update_window_title(App* app, const char* rom_path);

void usage(const char* prog) {
  fprintf(stderr,
          "Usage: %s [ROM]\n"
          "  -v     Show version\n"
          "  -h     Show usage\n"
          "\n",
          prog);
}

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
  app->ui.state.is_running = true;
  update_window_title(app, path);
}

void app_reset_emulator(void* ctx) {
  App* app = (App*)ctx;

  nex_reset(app->emulator.nes);
}

void app_step_instr(void* ctx) {
  App* app = (App*)ctx;

  nex_step(app->emulator.nes);
}

void app_step_scanline(void* ctx) {
  App* app = (App*)ctx;

  nex_step_scanline(app->emulator.nes);
}

void app_step_frame(void* ctx) {
  App* app = (App*)ctx;

  nex_step_frame(app->emulator.nes);
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
  window.width = 256 * 2 * main_scale;
  window.height = 224 * 2 * main_scale;
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

      .step_instr = app_step_instr,
      .step_scanline = app_step_scanline,
      .step_frame = app_step_frame,

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
  double last_time = glfwGetTime();
  double cycle_accumulator = 0.0;

  while (!glfwWindowShouldClose(app.window.handle)) {
    double now = glfwGetTime();
    double dt = now - last_time;
    last_time = now;

    if (dt > 0.25) {
      dt = 0.25;
    }

    // Poll events
    glfwPollEvents();

    // setup window
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // rendering
    show_ui(app.ui);

    if (app.ui.state.is_rom_loaded && app.ui.state.is_running) {
      cycle_accumulator += dt * NEX_CPU_HZ;

      while (cycle_accumulator >= 1.0) {
        nex_tick(app.emulator.nes);
        cycle_accumulator--;
      }
    } else {
      cycle_accumulator = 0.0;
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

int main(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt(argc, argv, "vh")) != -1) {
    switch (opt) {
      case 'v':
        printf("0.1.0\n");
        return EXIT_SUCCESS;
      case 'h':
        usage(argv[0]);
        return EXIT_SUCCESS;
      default:
        usage(argv[0]);
        return EXIT_FAILURE;
    }
  }

  int remaining = argc - optind;

  if (remaining > 1) {
    fprintf(stderr, "error: expected at most one ROM file\n");
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  const char* rom_path = (remaining == 1) ? argv[optind] : NULL;

  App app{};
  if (app_init(app) != 0) {
    return EXIT_FAILURE;
  }

  if (rom_path) {
    app_load_rom(&app, rom_path);
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
  bool shift = io.KeyShift;

  // File actions
  if (mod && ImGui::IsKeyPressed(ImGuiKey_O)) {
    ui.state.open_rom_dialog.Open();
  }

  if (mod && ImGui::IsKeyPressed(ImGuiKey_Q)) {
    ui.actions->quit(ui.actions->ctx);
  }

  // View actions
  if (shift && mod && ImGui::IsKeyPressed(ImGuiKey_C)) {
    ui.state.cpu_pane_visible = !ui.state.cpu_pane_visible;
  }

  if (shift && mod && ImGui::IsKeyPressed(ImGuiKey_P)) {
    ui.state.ppu_pane_visible = !ui.state.ppu_pane_visible;
  }

  if (shift && mod && ImGui::IsKeyPressed(ImGuiKey_M)) {
    ui.state.memory_pane_visible = !ui.state.memory_pane_visible;
  }

  // Debug actions
  if (mod && ImGui::IsKeyPressed(ImGuiKey_R)) {
    ui.actions->reset(ui.actions->ctx);
  }

  if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
    ui.state.is_running = !ui.state.is_running;
  }

  if (ImGui::IsKeyPressed(ImGuiKey_F9)) {
    ui.actions->step_frame(ui.actions->ctx);
  }

  if (ImGui::IsKeyPressed(ImGuiKey_F10)) {
    ui.actions->step_scanline(ui.actions->ctx);
  }

  if (ImGui::IsKeyPressed(ImGuiKey_F11)) {
    ui.actions->step_instr(ui.actions->ctx);
  }
}

void show_ui(UiContext& ui) {
  handle_shortcuts(ui);
  show_menu_bar(ui);

  if (ui.state.cpu_pane_visible) {
    show_cpu_pane(ui);
  }

  if (ui.state.ppu_pane_visible) {
    show_ppu_pane(ui);
  }

  if (ui.state.memory_pane_visible) {
    show_memory_pane(ui);
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
      ImGui::MenuItem("CPU", "Shift+" PRIMARY_MOD "+C",
                      &ui.state.cpu_pane_visible);
      ImGui::MenuItem("PPU", "Shift+" PRIMARY_MOD "+P",
                      &ui.state.ppu_pane_visible);
      ImGui::MenuItem("Memory", "Shift+" PRIMARY_MOD "+M",
                      &ui.state.memory_pane_visible);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Emulation")) {
      if (ImGui::MenuItem(ui.state.is_running ? "Pause" : "Resume", "F5")) {
        ui.state.is_running = !ui.state.is_running;
      }

      if (ImGui::MenuItem("Reset", PRIMARY_MOD "+R")) {
        ui.actions->reset(ui.actions->ctx);
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Step Frame", "F9")) {
        ui.actions->step_frame(ui.actions->ctx);
      }
      if (ImGui::MenuItem("Step Scanline", "F10")) {
        ui.actions->step_scanline(ui.actions->ctx);
      }
      if (ImGui::MenuItem("Step Instruction", "F11")) {
        ui.actions->step_instr(ui.actions->ctx);
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
    ui.actions->step_instr(ui.actions->ctx);
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

void show_ppu_pane(UiContext& ui) {
  if (!ImGui::Begin("PPU", &ui.state.ppu_pane_visible)) {
    ImGui::End();
    return;
  }

  NexPpuState state = nex_get_ppu_state(ui.emu->nes);

  ImGui::Text("Frame:    %llu", state.frame);
  ImGui::Text("Scanline: %d", state.scanline);
  ImGui::Text("Dot:      %d", state.dot);

  ImGui::SeparatorText("Registers");
  if (ImGui::TreeNode("control", "Control: %02X", state.ctrl)) {
    ImGui::Text("Sprite Size: %s", state.ctrl & (1 << 5) ? "8x16" : "8x8");
    ImGui::Text("Vblank NMI: %s",
                state.ctrl & (1 << 7) ? "Enabled" : "Disabled");

    ImGui::TreePop();
  }
  if (ImGui::TreeNode("mask", "Mask: %02X", state.mask)) {
    ImGui::Text("Greyscale: %d", state.mask & 1);
    ImGui::Text("Background Rendering: %s",
                state.mask & (1 << 2) ? "Enabled" : "Disabled");
    ImGui::Text("Sprite Rendering: %s",
                state.mask & (1 << 3) ? "Enabled" : "Disabled");

    ImGui::TreePop();
  }
  if (ImGui::TreeNode("status", "Status: %02X", state.status)) {
    ImGui::Text("Sprite Overflow: %s",
                state.status & (1 << 5) ? "True" : "False");
    ImGui::Text("Sprite 0 Hit:    %s",
                state.status & (1 << 6) ? "True" : "False");
    ImGui::Text("Vblank:          %s",
                state.status & (1 << 7) ? "True" : "False");
    ImGui::TreePop();
  }

  ImGui::Separator();

  ImGui::End();
}

void show_memory_pane(UiContext& ui) {
  if (!ImGui::Begin("Memory", &ui.state.memory_pane_visible)) {
    ImGui::End();
    return;
  }

  ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
  if (ImGui::BeginTabBar("Memory Locations", tab_bar_flags)) {
    if (ImGui::BeginTabItem("CPU WRAM")) {
      uint8_t data[NEX_WRAM_SIZE];
      nex_read_wram(ui.emu->nes, data);
      ui.state.wram_mem_edit.DrawContents(data, NEX_WRAM_SIZE);

      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("PPU VRAM")) {
      uint8_t data[NEX_VRAM_SIZE];
      nex_read_vram(ui.emu->nes, data);
      ui.state.vram_mem_edit.DrawContents(data, NEX_VRAM_SIZE);

      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("CHR ROM")) {
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::End();
}

void update_window_title(App* app, const char* rom_path) {
  std::filesystem::path path(rom_path);

  std::string title = "nex - ";
  title += path.filename().string();

  glfwSetWindowTitle(app->window.handle, title.c_str());
}