#include <format>
#include <fstream>
#include <map>
#include <print>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

struct Glyph {
  SDL_Texture *texture;
  int w, h;
};

struct AppContext {
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;
  TTF_Font *font = nullptr;
  float fontSize = 52.0f;
  std::string fontPath =
      "/Users/deondreenglish/Library/Fonts/MapleMono-Regular.ttf";

  bool running = true;
  std::vector<uint8_t> buffer;

  size_t cursorIndex = 0;
  int bytesPerRow = 16;
  int scrollOffsetLines = 0;

  std::map<char, Glyph> glyphCache;
  float charW = 0.0f, charH = 0.0f;
};

bool LoadFileIntoBuffer(const char *filename, AppContext &app) {
  if (!filename)
    return false;
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file) {
    std::println("Error: Could not open file {}", filename);
    return false;
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  app.buffer.resize(static_cast<size_t>(size));
  file.read(reinterpret_cast<char *>(app.buffer.data()), size);

  app.cursorIndex = 0;
  app.scrollOffsetLines = 0;
  std::println("Loaded {} bytes from {}", size, filename);
  return true;
}

bool LoadFile(const std::string &filename, AppContext &app) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file) {
    std::println("Error: Could not open file {}", filename);
    return false;
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  app.buffer.resize(static_cast<size_t>(size));
  file.read(reinterpret_cast<char *>(app.buffer.data()), size);
  return true;
}

void SDLCALL OnFileSelected(void *userdata, const char *const *filelist,
                            int filter) {
  AppContext *app = static_cast<AppContext *>(userdata);
  if (filelist && filelist[0]) {
    LoadFileIntoBuffer(filelist[0], *app);
  }
}

void ClearGlyphCache(AppContext &app) {
  for (auto &[c, g] : app.glyphCache) {
    SDL_DestroyTexture(g.texture);
  }
  app.glyphCache.clear();
}

void BuildGlyphCache(AppContext &app) {
  ClearGlyphCache(app);
  std::string charset =
      "0123456789ABCDEF abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "!@#$%^&*()_+-=[]{};':\",./<>?\\|`~.";
  SDL_Color white = {255, 255, 255, 255};

  for (char c : charset) {
    std::string s(1, c);
    SDL_Surface *surf = TTF_RenderText_Blended(app.font, s.c_str(), 0, white);
    if (surf) {
      app.glyphCache[c] = {SDL_CreateTextureFromSurface(app.renderer, surf),
                           surf->w, surf->h};
      app.charW = (float)surf->w;
      app.charH = (float)surf->h;
      SDL_DestroySurface(surf);
    }
  }
}

void DrawText(AppContext &app, const std::string &text, float x, float y,
              SDL_Color color) {
  for (char c : text) {
    char lookup = app.glyphCache.contains(c) ? c : '.';
    Glyph &g = app.glyphCache[lookup];
    SDL_SetTextureColorMod(g.texture, color.r, color.g, color.b);
    SDL_FRect dest = {x, y, (float)g.w, (float)g.h};
    SDL_RenderTexture(app.renderer, g.texture, nullptr, &dest);
    x += app.charW;
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 1;
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();

  AppContext app;
  if (!LoadFile(argv[1], app))
    return 1;

  app.window =
      SDL_CreateWindow("Hex Editor Pro", 1200, 800,
                       SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
  app.renderer = SDL_CreateRenderer(app.window, nullptr);
  // SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");

  app.font = TTF_OpenFont(app.fontPath.c_str(), app.fontSize);
  if (!app.font)
    return 1;
  BuildGlyphCache(app);

  while (app.running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT)
        app.running = false;

      if (event.type == SDL_EVENT_KEY_DOWN) {
        bool cmd = (event.key.mod & SDL_KMOD_GUI);
        bool shift = (event.key.mod & SDL_KMOD_SHIFT);

        if (cmd && event.key.key == SDLK_O) {
          SDL_ShowOpenFileDialog(OnFileSelected, &app, app.window, nullptr, 0,
                                 nullptr, false);
        }

        if (cmd && shift &&
            (event.key.key == SDLK_EQUALS || event.key.key == SDLK_MINUS)) {
          app.fontSize += (event.key.key == SDLK_EQUALS) ? 2.0f : -2.0f;
          app.fontSize = std::max(8.0f, app.fontSize);
          TTF_CloseFont(app.font);
          app.font = TTF_OpenFont(app.fontPath.c_str(), app.fontSize);
          BuildGlyphCache(app);
        }

        switch (event.key.key) {
        case SDLK_UP:
          if (app.cursorIndex >= (size_t)app.bytesPerRow)
            app.cursorIndex -= app.bytesPerRow;
          break;
        case SDLK_DOWN:
          if (app.cursorIndex + app.bytesPerRow < app.buffer.size())
            app.cursorIndex += app.bytesPerRow;
          break;
        case SDLK_LEFT:
          if (app.cursorIndex > 0)
            app.cursorIndex--;
          break;
        case SDLK_RIGHT:
          if (app.cursorIndex < app.buffer.size() - 1)
            app.cursorIndex++;
          break;
        }
      }
    }

    // --- DYNAMIC LAYOUT CALCULATIONS ---
    int winW, winH;
    // Use Pixels for High DPI support so text fills the physical screen
    SDL_GetWindowSizeInPixels(app.window, &winW, &winH);

    float marginX = 20.0f;
    float marginY = 20.0f;

    // Dynamic visible lines based on the CURRENT window height
    int visibleLines = static_cast<int>(
        (static_cast<float>(winH) - (marginY * 2.0f)) / app.charH);

    int cursorLine = static_cast<int>(app.cursorIndex / app.bytesPerRow);
    if (cursorLine < app.scrollOffsetLines) {
      app.scrollOffsetLines = cursorLine;
    } else if (cursorLine >= app.scrollOffsetLines + visibleLines) {
      app.scrollOffsetLines = cursorLine - visibleLines + 1;
    }

    SDL_SetRenderDrawColor(app.renderer, 15, 15, 18, 255);
    SDL_RenderClear(app.renderer);

    float addrX = marginX;
    float hexXStart = addrX + (app.charW * 10);
    float asciiXStart = hexXStart + (app.bytesPerRow * app.charW * 3) + 20.0f;

    // --- RENDER LOOP ---
    for (int i = 0; i < visibleLines; ++i) {
      int currentLine = i + app.scrollOffsetLines;
      size_t rowStart = static_cast<size_t>(currentLine) * app.bytesPerRow;
      if (rowStart >= app.buffer.size())
        break;

      float y = marginY + (static_cast<float>(i) * app.charH);

      // 1. Address Column
      DrawText(app, std::format("{:08X}", rowStart), addrX, y,
               {80, 85, 90, 255});

      // 2. Hex and ASCII Columns
      for (int col = 0; col < app.bytesPerRow; ++col) {
        size_t idx = rowStart + col;
        if (idx >= app.buffer.size())
          break;

        float hX = hexXStart + (static_cast<float>(col) * app.charW * 3.0f);
        float aX = asciiXStart + (static_cast<float>(col) * app.charW);

        // Cursor Highlight
        if (idx == app.cursorIndex) {
          SDL_FRect hb = {hX - 2.0f, y, app.charW * 2.0f + 4.0f, app.charH};
          SDL_FRect ab = {aX, y, app.charW, app.charH};
          SDL_SetRenderDrawColor(app.renderer, 0, 100, 200, 180);
          SDL_RenderFillRect(app.renderer, &hb);
          SDL_RenderFillRect(app.renderer, &ab);
        }

        // Render Hex Code
        DrawText(app, std::format("{:02X}", app.buffer[idx]), hX, y,
                 {220, 220, 220, 255});

        // Render ASCII Translator
        char c = (app.buffer[idx] >= 32 && app.buffer[idx] <= 126)
                     ? static_cast<char>(app.buffer[idx])
                     : '.';
        DrawText(app, std::string(1, c), aX, y, {120, 120, 130, 255});
      }
    }

    // Draw Vertical Divider Line that fills the whole height
    SDL_SetRenderDrawColor(app.renderer, 40, 40, 45, 255);
    SDL_RenderLine(app.renderer, asciiXStart - 12.0f, 0, asciiXStart - 12.0f,
                   static_cast<float>(winH));

    SDL_RenderPresent(app.renderer);
  }

  ClearGlyphCache(app);
  TTF_CloseFont(app.font);
  TTF_Quit();
  SDL_DestroyRenderer(app.renderer);
  SDL_DestroyWindow(app.window);
  SDL_Quit();

  return 0;
}