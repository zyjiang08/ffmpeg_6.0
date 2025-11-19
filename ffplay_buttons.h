/*
 * FFplay Simple Button UI
 * Copyright (c) 2025
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#ifndef FFPLAY_BUTTONS_H
#define FFPLAY_BUTTONS_H

#include <SDL.h>

// Button states
typedef enum {
    BUTTON_STATE_NORMAL = 0,
    BUTTON_STATE_HOVER,
    BUTTON_STATE_PRESSED
} ButtonState;

// Button IDs for event handling
typedef enum {
    BUTTON_ID_PLAY_PAUSE = 0,
    BUTTON_ID_STOP,
    BUTTON_ID_SEEK_BACKWARD,
    BUTTON_ID_SEEK_FORWARD,
    BUTTON_ID_SWITCH_CHANNEL,      // Channel switch button
    BUTTON_ID_STRESS_TEST,         // Stress test toggle button
    BUTTON_ID_COUNT
} ButtonID;

// Button structure
typedef struct {
    ButtonID id;
    SDL_Rect rect;           // Button position and size
    ButtonState state;       // Current state (normal/hover/pressed)
    const char *label;       // Button text label
    SDL_Color bg_color;      // Background color
    SDL_Color hover_color;   // Hover state color
    SDL_Color press_color;   // Pressed state color
    SDL_Color text_color;    // Text color
    int visible;             // Visibility flag
} Button;

// Button manager structure
typedef struct {
    Button buttons[BUTTON_ID_COUNT];
    int window_width;
    int window_height;
    int button_height;
    int button_spacing;
    int controls_visible;
    int controls_y_offset;    // Y position offset for animation
} ButtonManager;

/**
 * Initialize button manager
 * @param mgr Button manager to initialize
 * @param window_width Initial window width
 * @param window_height Initial window height
 */
void buttons_init(ButtonManager *mgr, int window_width, int window_height);

/**
 * Update button positions when window is resized
 * @param mgr Button manager
 * @param window_width New window width
 * @param window_height New window height
 */
void buttons_update_layout(ButtonManager *mgr, int window_width, int window_height);

/**
 * Render all buttons
 * @param mgr Button manager
 * @param renderer SDL renderer
 */
void buttons_render(ButtonManager *mgr, SDL_Renderer *renderer);

/**
 * Handle mouse motion event
 * @param mgr Button manager
 * @param x Mouse X position
 * @param y Mouse Y position
 * @return 1 if hovering over any button, 0 otherwise
 */
int buttons_handle_mouse_motion(ButtonManager *mgr, int x, int y);

/**
 * Handle mouse button event
 * @param mgr Button manager
 * @param x Mouse X position
 * @param y Mouse Y position
 * @param pressed 1 if button pressed, 0 if released
 * @return Button ID if clicked, -1 otherwise
 */
int buttons_handle_mouse_button(ButtonManager *mgr, int x, int y, int pressed);

/**
 * Show/hide controls
 * @param mgr Button manager
 * @param visible 1 to show, 0 to hide
 */
void buttons_set_visible(ButtonManager *mgr, int visible);

/**
 * Update play/pause button label
 * @param mgr Button manager
 * @param is_paused 1 if video is paused, 0 if playing
 */
void buttons_update_play_pause_label(ButtonManager *mgr, int is_paused);

#endif // FFPLAY_BUTTONS_H
