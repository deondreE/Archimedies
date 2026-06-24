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
  int win_w, rend_w;
  SDL_Window *window =
      SDL_GetRenderWindow(renderer);
  if (!window)
    return;

  SDL_GetWindowSize(window, &win_w, NULL);
  SDL_GetRenderOutputSize(renderer, &rend_w, NULL);
  float dpi_scale = (win_w > 0) ? (float)rend_w / (float)win_w : 1.0f;

  static float last_scale = 0.0f;
  if (dpi_scale != last_scale) {
    if (app.font)
      TTF_CloseFont(app.font);
    app.font =
        TTF_OpenFont(app.fontPath.c_str(), (int)(app.fontSize * dpi_scale));
    TTF_SetFontHinting(app.font, TTF_HINTING_LIGHT);
    last_scale = dpi_scale;
  }

  for (auto &[c, glyph] : app.glyphCache) {
    if (glyph.texture)
      SDL_DestroyTexture(glyph.texture);
  }
  app.glyphCache.clear();

  const char *charset = "0123456789ABCDEF "
                        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@"
                        "#$%^&*()_+-=[]{};':\",./<>?\\|`~.";
  SDL_Color white = {255, 255, 255, 255};

  for (const char *p = charset; *p != '\0'; p++) {
    char c = *p;

    SDL_Surface *surf = TTF_RenderGlyph_Blended(app.font, (Uint32)c, white);

    if (surf) {
      SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
      if (tex) {
        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
        app.glyphCache[c] = {tex, surf->w, surf->h};

        if (c == 'A' || app.charW == 0) {
          app.charW = (float)surf->w / dpi_scale;
          app.charH = (float)surf->h / dpi_scale;
        }
      }
      SDL_DestroySurface(surf);
    }
  }
}

NodeRegion GetNodeRegion(Vector2 mousePos, const Rect &bounds, float handleSize = 10.0f) {
  // Bottom Left
  if (mousePos.x <= bounds.x + handleSize &&
      mousePos.y >= (bounds.y + bounds.h) - handleSize) {
    return NodeRegion::BOTTOM_LEFT;
  }

  // Bottom Right
  if (mousePos.x >= (bounds.x + bounds.w) - handleSize &&
      mousePos.y >= (bounds.y + bounds.h) - handleSize) {
    return NodeRegion::BOTTOM_RIGHT;
  }

  return NodeRegion::BODY;
}
