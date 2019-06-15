#ifndef _WAYTEXT_H
#define _WAYTEXT_H

#include <stdbool.h>
#include <stdint.h>
#include <wayland-client.h>

#include "pool-buffer.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

struct waytext_state {
  bool running;

  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_shm *shm;
  struct wl_compositor *compositor;
  struct zwlr_layer_shell_v1 *layer_shell;
  struct zxdg_output_manager_v1 *xdg_output_manager;
  struct wl_list outputs; // waytext_output::link
  struct wl_list seats;   // waytext_seat::link

  struct {
    uint32_t background;
    uint32_t foreground;
  } colors;

  const char *text;
  uint32_t font_weight;
};

struct waytext_output {
  struct wl_output *wl_output;
  struct waytext_state *state;
  struct wl_list link; // waytext_state::outputs

  int32_t scale;

  struct wl_surface *surface;
  struct zwlr_layer_surface_v1 *layer_surface;

  struct zxdg_output_v1 *xdg_output;

  struct wl_callback *frame_callback;
  bool configured;
  bool dirty;
  int32_t width, height;
  struct pool_buffer buffers[2];
  struct pool_buffer *current_buffer;

  struct wl_cursor_theme *cursor_theme;
  struct wl_cursor_image *cursor_image;
};

struct waytext_seat {
  struct wl_surface *cursor_surface;
  struct waytext_state *state;
  struct wl_seat *wl_seat;
  struct wl_list link; // waytext_state::seats

  // keyboard:
  struct wl_keyboard *wl_keyboard;

  // pointer:
  struct wl_pointer *wl_pointer;
  enum wl_pointer_button_state button_state;
  struct waytext_output *current_output;
};

#endif
