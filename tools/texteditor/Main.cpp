#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Piecetable.hpp"
#include <print>

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* window = SDL_CreateWindow("Rune Editor", 800, 600, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    TTF_Font* font = TTF_OpenFont(DEFAULT_FONT_PATH, 24);

    Arena persistent = Arena::bootstrap(1024 * 1024 * 64);
    Arena frame_scratch = Arena::bootstrap(1024 * 1024 * 16);
    
    PieceTable table = PieceTable::create(&persistent, "Welcome to the Rune Editor.");
    s64 cursor_pos = table.original.size; // Start at the end

    // Start SDL text input mode
    SDL_StartTextInput(window);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            
            // 1. Regular Character Input (UTF-8)
            if (event.type == SDL_EVENT_TEXT_INPUT) {
                String8 input(event.text.text);
                table.insert(cursor_pos, input);
                cursor_pos += input.size;
            }

            // 2. Control Keys (Backspace, Enter, Arrows)
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch(event.key.key) {
                    case SDLK_BACKSPACE: {
                        if (cursor_pos > 0) {
                            // table.remove(cursor_pos - 1, 1); // We need to add this!
                            cursor_pos--; 
                        }
                    } break;
                    case SDLK_RETURN: {
                        table.insert(cursor_pos, "\n");
                        cursor_pos += 1;
                    } break;
                    case SDLK_LEFT: {
                        if (cursor_pos > 0) cursor_pos--;
                    } break;
                    case SDLK_RIGHT: {
                        // In production, check against total table length
                        cursor_pos++; 
                    } break;
                }
            }
        }

        // --- RENDERING ---
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255);
        SDL_RenderClear(renderer);

        {
            ArenaTemp tmp(&frame_scratch);
            String8 doc = table.get_all_text(&frame_scratch);
            
            SDL_Color white = {230, 230, 230, 255};
            // Use Wrapped rendering to handle newlines automatically for now
            SDL_Surface* surf = TTF_RenderText_Blended_Wrapped(font, doc.str, 0, white, 700);
            
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                SDL_FRect dst = {50, 50, (float)surf->w, (float)surf->h};
                SDL_RenderTexture(renderer, tex, NULL, &dst);
                
                // --- Simple Cursor Drawing ---
                // In a real editor, you'd calculate current cursor (x,y) 
                // but for testing, let's just show status
                std::print("\rCursor: {} | Pieces: {}      ", cursor_pos, table.piece_count);

                SDL_DestroyTexture(tex);
                SDL_DestroySurface(surf);
            }
        }

        SDL_RenderPresent(renderer);
        frame_scratch.clear();
    }

    SDL_StopTextInput(window);
    persistent.release();
    return 0;
}