#include "apptypes.h"

bool is_point_in_circle(float px, float py, float cx, float cy, float radius) {
  float dx = px - cx, dy = py - cy;
  return (dx * dx + dy * dy) <= (radius * radius);
}

bool is_point_in_rect(float px, float py, SDL_FRect rect) {
  return (px >= rect.x && px <= rect.x + rect.w && py >= rect.y &&
          py <= rect.y + rect.h);
}

Node *FindNodeAtPoint(std::vector<Node> &nodes, Vector2 pos) {
  for (auto &n : nodes) {
    SDL_FRect r{n.UI_bounds.x, n.UI_bounds.y, n.UI_bounds.w, n.UI_bounds.h};
    if (is_point_in_rect(pos.x, pos.y, r))
      return &n;
  }
  return nullptr;
}

void ClearGlyphCache(AppContext &app) {
  for (auto &[c, g] : app.glyphCache) {
    SDL_DestroyTexture(g.texture);
  }
  app.glyphCache.clear();
}

// Pre-renders characters into textures for fast UI rendering
void BuildGlyphCache(AppContext &app, SDL_Renderer *renderer) {
  ClearGlyphCache(app);
  std::string charset = "0123456789ABCDEF "
                        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@"
                        "#$%^&*()_+-=[]{};':\",./<>?\\|`~.";
  SDL_Color white = {255, 255, 255, 255};

  for (char c : charset) {
    std::string s(1, c);
    SDL_Surface *surf = TTF_RenderText_Blended(app.font, s.c_str(), 0, white);
    if (surf) {
      app.glyphCache[c] = {SDL_CreateTextureFromSurface(renderer, surf),
                           surf->w, surf->h};
      app.charW = (float)surf->w;
      app.charH = (float)surf->h;
      SDL_DestroySurface(surf);
    }
  }
}
