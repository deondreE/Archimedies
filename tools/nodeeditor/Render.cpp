#include "apptypes.h"
#include <cmath>

// Draws a Cubic Bezier curve between ports for professional line look 
void draw_bezier(SDL_Renderer *renderer, Vector2 start, Vector2 end,
                 float thickness) {
  float offset = SDL_fabsf(end.x - start.x) / 2.0f;
  if (offset < 50.0f)
    offset = 50.0f;

  Vector2 cp1 = {start.x + offset, start.y};
  Vector2 cp2 = {end.x - offset, end.y};

  float dx = end.x - start.x, dy = end.y - start.y;
  float approx_len = SDL_sqrtf(dx * dx + dy * dy);
  int steps = (int)SDL_clamp(approx_len / 4.0f, 20.0f, 120.0f);

  Uint8 r, g, b, a;
  SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_FColor col = {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};

  std::vector<SDL_Vertex> verts;
  std::vector<int> indices;
  verts.reserve((steps + 1) * 2);
  indices.reserve(steps * 6);

  float half = thickness / 2.0f;

  for (int i = 0; i <= steps; i++) {
    float t = (float)i / (float)steps;
    float invT = 1.0f - t;

    Vector2 pt = {
        (invT * invT * invT * start.x) + (3 * invT * invT * t * cp1.x) +
            (3 * invT * t * t * cp2.x) + (t * t * t * end.x),
        (invT * invT * invT * start.y) + (3 * invT * invT * t * cp1.y) +
            (3 * invT * t * t * cp2.y) + (t * t * t * end.y)};

    // Analytic derivative of the cubic bezier — true tangent at this t,
    // never degenerates to (0,0) the way a backward-difference can at i=0
    Vector2 d = {
        3 * invT * invT * (cp1.x - start.x) + 6 * invT * t * (cp2.x - cp1.x) +
            3 * t * t * (end.x - cp2.x),
        3 * invT * invT * (cp1.y - start.y) + 6 * invT * t * (cp2.y - cp1.y) +
            3 * t * t * (end.y - cp2.y)};

    float len = SDL_sqrtf(d.x * d.x + d.y * d.y);
    if (len < 0.0001f)
      len = 0.0001f;
    Vector2 normal = {-d.y / len, d.x / len};

    verts.push_back(
        {{pt.x + normal.x * half, pt.y + normal.y * half}, col, {0, 0}});
    verts.push_back(
        {{pt.x - normal.x * half, pt.y - normal.y * half}, col, {0, 0}});

    if (i > 0) {
      int base = (i - 1) * 2;
      // two triangles per segment, forming the quad between this point
      // and the previous one
      indices.push_back(base);
      indices.push_back(base + 1);
      indices.push_back(base + 2);
      indices.push_back(base + 1);
      indices.push_back(base + 3);
      indices.push_back(base + 2);
    }
  }

  SDL_RenderGeometry(renderer, nullptr, verts.data(), (int)verts.size(),
                     indices.data(), (int)indices.size());
}

// Iterates through glyph cache to draw strings character by character
void DrawText(AppContext &app, const std::string &text, float x, float y,
              SDL_Color color, SDL_Renderer *renderer) {
  if (text.empty())
    return;

  SDL_Window *window = SDL_GetRenderWindow(renderer);
  int win_w, rend_w;

  if (!SDL_GetWindowSize(window, &win_w, NULL) ||
      !SDL_GetRenderOutputSize(renderer, &rend_w, NULL)) {
    return;
  }

  // 2. Calculate scale (e.g., 0.5 for Retina)
  float inv_scale = (rend_w > 0) ? (float)win_w / (float)rend_w : 1.0f;

  for (char c : text) {
    // 3. Skip whitespace but still advance the cursor
    if (c == ' ') {
      x += app.charW;
      continue;
    }

    char lookup = app.glyphCache.contains(c) ? c : '.';
    auto it = app.glyphCache.find(lookup);
    if (it == app.glyphCache.end())
      continue;

    Glyph &g = it->second;

    SDL_SetTextureBlendMode(g.texture, SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(g.texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(g.texture, color.a);

    SDL_FRect dest = {x, y, (float)g.w * inv_scale, (float)g.h * inv_scale};

    SDL_RenderTexture(renderer, g.texture, nullptr, &dest);

    x += app.charW;
  }
}

// Manually draws a filled circle via point-strips
void draw_circle(SDL_Renderer *renderer, float cx, float cy, float radius, SDL_FColor color) {
  const int segments = (radius < 10) ? 16 : (int)(radius * 3);

  std::vector<SDL_Vertex> vertices(segments + 2);
  std::vector<int> indices(segments * 3);

  vertices[0].position = {cx, cy};
  vertices[0].color = color;

  for (int i = 0; i <= segments; ++i) {
    float angle = i * 2.0f * (float)M_PI / segments;
    vertices[i + 1].position = {cx + (radius * cosf(angle)),
                                cy + (radius * sinf(angle))};
    vertices[i + 1].color = color;
  }

  for (int i = 0; i < segments; ++i) {
    indices[i * 3 + 0] = 0;
    indices[i * 3 + 1] = i + 1;
    indices[i * 3 + 2] = i + 2;
  }

  SDL_RenderGeometry(renderer, nullptr, vertices.data(), (int)vertices.size(),
                     indices.data(), (int)indices.size());
}
