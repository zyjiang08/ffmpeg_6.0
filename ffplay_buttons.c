/*
 * FFplay Simple Button UI Implementation
 * Copyright (c) 2025
 *
 * This file is part of FFmpeg.
 */

#include "ffplay_buttons.h"
#include <string.h>

// Button layout constants
#define BUTTON_HEIGHT 40
#define BUTTON_MIN_WIDTH 80
#define BUTTON_SPACING 10
#define CONTROLS_MARGIN 20
#define CONTROLS_BOTTOM_OFFSET 20

// Helper function to check if point is inside rect
static int point_in_rect(int x, int y, const SDL_Rect *rect)
{
    return x >= rect->x && x < rect->x + rect->w &&
           y >= rect->y && y < rect->y + rect->h;
}

// Helper function to draw filled rectangle
static void draw_filled_rect(SDL_Renderer *renderer, const SDL_Rect *rect, const SDL_Color *color)
{
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
    SDL_RenderFillRect(renderer, rect);
}

// Helper function to draw text (simple, without SDL_ttf for now)
static void draw_simple_text(SDL_Renderer *renderer, const char *text,
                             int x, int y, const SDL_Color *color)
{
    // For now, we'll use simple rectangles to represent text
    // In a full implementation, this would use SDL_ttf
    // This is a placeholder to keep the implementation simple
    (void)renderer;
    (void)text;
    (void)x;
    (void)y;
    (void)color;
}

void buttons_init(ButtonManager *mgr, int window_width, int window_height)
{
    memset(mgr, 0, sizeof(ButtonManager));

    mgr->window_width = window_width;
    mgr->window_height = window_height;
    mgr->button_height = BUTTON_HEIGHT;
    mgr->button_spacing = BUTTON_SPACING;
    mgr->controls_visible = 1;
    mgr->controls_y_offset = 0;

    // Initialize button colors
    SDL_Color bg_normal = {50, 50, 50, 200};
    SDL_Color bg_hover = {70, 70, 70, 220};
    SDL_Color bg_press = {30, 30, 30, 240};
    SDL_Color text_color = {255, 255, 255, 255};

    // Initialize each button
    const char *labels[BUTTON_ID_COUNT] = {
        "Play",    // BUTTON_ID_PLAY_PAUSE
        "Stop",    // BUTTON_ID_STOP
        "<<",      // BUTTON_ID_SEEK_BACKWARD
        ">>"       // BUTTON_ID_SEEK_FORWARD
    };

    for (int i = 0; i < BUTTON_ID_COUNT; i++) {
        mgr->buttons[i].id = i;
        mgr->buttons[i].state = BUTTON_STATE_NORMAL;
        mgr->buttons[i].label = labels[i];
        mgr->buttons[i].bg_color = bg_normal;
        mgr->buttons[i].hover_color = bg_hover;
        mgr->buttons[i].press_color = bg_press;
        mgr->buttons[i].text_color = text_color;
        mgr->buttons[i].visible = 1;
    }

    buttons_update_layout(mgr, window_width, window_height);
}

void buttons_update_layout(ButtonManager *mgr, int window_width, int window_height)
{
    mgr->window_width = window_width;
    mgr->window_height = window_height;

    // Calculate total width needed for all buttons
    int total_buttons = BUTTON_ID_COUNT;
    int button_width = BUTTON_MIN_WIDTH;
    int total_width = total_buttons * button_width + (total_buttons - 1) * mgr->button_spacing;

    // Center buttons horizontally
    int start_x = (window_width - total_width) / 2;

    // Position buttons at the bottom of the window
    int start_y = window_height - mgr->button_height - CONTROLS_BOTTOM_OFFSET + mgr->controls_y_offset;

    // Layout buttons horizontally
    for (int i = 0; i < BUTTON_ID_COUNT; i++) {
        mgr->buttons[i].rect.x = start_x + i * (button_width + mgr->button_spacing);
        mgr->buttons[i].rect.y = start_y;
        mgr->buttons[i].rect.w = button_width;
        mgr->buttons[i].rect.h = mgr->button_height;
    }
}

void buttons_render(ButtonManager *mgr, SDL_Renderer *renderer)
{
    if (!mgr->controls_visible)
        return;

    for (int i = 0; i < BUTTON_ID_COUNT; i++) {
        Button *btn = &mgr->buttons[i];
        if (!btn->visible)
            continue;

        // Choose color based on state
        SDL_Color *color;
        switch (btn->state) {
            case BUTTON_STATE_PRESSED:
                color = &btn->press_color;
                break;
            case BUTTON_STATE_HOVER:
                color = &btn->hover_color;
                break;
            default:
                color = &btn->bg_color;
                break;
        }

        // Draw button background
        draw_filled_rect(renderer, &btn->rect, color);

        // Draw button border
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &btn->rect);

        // Draw button label (centered)
        int text_x = btn->rect.x + btn->rect.w / 2;
        int text_y = btn->rect.y + btn->rect.h / 2;
        draw_simple_text(renderer, btn->label, text_x, text_y, &btn->text_color);

        // For now, draw a simple indicator of the button label using lines
        // This is a placeholder - in full implementation we'd use SDL_ttf
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // Draw different patterns for different buttons
        switch (btn->id) {
            case BUTTON_ID_PLAY_PAUSE:
                // Draw play triangle or pause bars
                {
                    int cx = btn->rect.x + btn->rect.w / 2;
                    int cy = btn->rect.y + btn->rect.h / 2;
                    int size = 10;
                    // Simple triangle for play
                    SDL_RenderDrawLine(renderer, cx - size/2, cy - size, cx - size/2, cy + size);
                    SDL_RenderDrawLine(renderer, cx - size/2, cy - size, cx + size/2, cy);
                    SDL_RenderDrawLine(renderer, cx - size/2, cy + size, cx + size/2, cy);
                }
                break;

            case BUTTON_ID_STOP:
                // Draw stop square
                {
                    int cx = btn->rect.x + btn->rect.w / 2;
                    int cy = btn->rect.y + btn->rect.h / 2;
                    int size = 12;
                    SDL_Rect stop_rect = {cx - size/2, cy - size/2, size, size};
                    SDL_RenderFillRect(renderer, &stop_rect);
                }
                break;

            case BUTTON_ID_SEEK_BACKWARD:
                // Draw backward arrows
                {
                    int cx = btn->rect.x + btn->rect.w / 2;
                    int cy = btn->rect.y + btn->rect.h / 2;
                    int size = 8;
                    // Left arrow
                    SDL_RenderDrawLine(renderer, cx - 2, cy, cx - size - 2, cy - size);
                    SDL_RenderDrawLine(renderer, cx - 2, cy, cx - size - 2, cy + size);
                    // Second left arrow
                    SDL_RenderDrawLine(renderer, cx + 4, cy, cx - 4, cy - size);
                    SDL_RenderDrawLine(renderer, cx + 4, cy, cx - 4, cy + size);
                }
                break;

            case BUTTON_ID_SEEK_FORWARD:
                // Draw forward arrows
                {
                    int cx = btn->rect.x + btn->rect.w / 2;
                    int cy = btn->rect.y + btn->rect.h / 2;
                    int size = 8;
                    // Right arrow
                    SDL_RenderDrawLine(renderer, cx - 4, cy, cx + 4, cy - size);
                    SDL_RenderDrawLine(renderer, cx - 4, cy, cx + 4, cy + size);
                    // Second right arrow
                    SDL_RenderDrawLine(renderer, cx + 2, cy, cx + size + 2, cy - size);
                    SDL_RenderDrawLine(renderer, cx + 2, cy, cx + size + 2, cy + size);
                }
                break;
        }
    }
}

int buttons_handle_mouse_motion(ButtonManager *mgr, int x, int y)
{
    if (!mgr->controls_visible)
        return 0;

    int hovering = 0;

    for (int i = 0; i < BUTTON_ID_COUNT; i++) {
        Button *btn = &mgr->buttons[i];
        if (!btn->visible)
            continue;

        if (point_in_rect(x, y, &btn->rect)) {
            if (btn->state == BUTTON_STATE_NORMAL) {
                btn->state = BUTTON_STATE_HOVER;
            }
            hovering = 1;
        } else {
            if (btn->state == BUTTON_STATE_HOVER) {
                btn->state = BUTTON_STATE_NORMAL;
            }
        }
    }

    return hovering;
}

int buttons_handle_mouse_button(ButtonManager *mgr, int x, int y, int pressed)
{
    if (!mgr->controls_visible)
        return -1;

    for (int i = 0; i < BUTTON_ID_COUNT; i++) {
        Button *btn = &mgr->buttons[i];
        if (!btn->visible)
            continue;

        if (point_in_rect(x, y, &btn->rect)) {
            if (pressed) {
                btn->state = BUTTON_STATE_PRESSED;
            } else {
                // Button released - this is a click
                btn->state = BUTTON_STATE_HOVER;
                return btn->id;
            }
        }
    }

    return -1;
}

void buttons_set_visible(ButtonManager *mgr, int visible)
{
    mgr->controls_visible = visible;
}

void buttons_update_play_pause_label(ButtonManager *mgr, int is_paused)
{
    Button *btn = &mgr->buttons[BUTTON_ID_PLAY_PAUSE];
    btn->label = is_paused ? "Play" : "Pause";
}
