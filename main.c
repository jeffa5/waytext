#define _POSIX_C_SOURCE 2
#include <errno.h>
#include <linux/input-event-codes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-cursor.h>

#include "include/render.h"
#include "include/waytext.h"

static void noop() {
  // This space intentionally left blank
}

static struct waytext_output *output_from_surface(struct waytext_state *state,
                                                  struct wl_surface *surface);

static void pointer_handle_enter(void *data, struct wl_pointer *wl_pointer,
                                 uint32_t serial, struct wl_surface *surface,
                                 wl_fixed_t surface_x, wl_fixed_t surface_y) {
  struct waytext_seat *seat = data;
  struct waytext_output *output = output_from_surface(seat->state, surface);
  if (output == NULL) {
    return;
  }
  // TODO: handle multiple overlapping outputs
  seat->current_output = output;
}

static void pointer_handle_leave(void *data, struct wl_pointer *wl_pointer,
                                 uint32_t serial, struct wl_surface *surface) {
  struct waytext_seat *seat = data;

  // TODO: handle multiple overlapping outputs
  seat->current_output = NULL;
}

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t time, wl_fixed_t surface_x,
                                  wl_fixed_t surface_y) {}

static void pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t serial, uint32_t time,
                                  uint32_t button, uint32_t button_state) {
  struct waytext_seat *seat = data;
  struct waytext_state *state = seat->state;

  switch (button_state) {
  case WL_POINTER_BUTTON_STATE_RELEASED:
    state->running = false;
    break;
  }
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_handle_enter,
    .leave = pointer_handle_leave,
    .motion = pointer_handle_motion,
    .button = pointer_handle_button,
    .axis = noop,
};

static void keyboard_handle_key(void *data, struct wl_keyboard *wl_keyboard,
                                uint32_t serial, uint32_t time, uint32_t key,
                                uint32_t key_state) {
  struct waytext_seat *seat = data;
  struct waytext_state *state = seat->state;
  if (key_state == WL_KEYBOARD_KEY_STATE_RELEASED) {
    if (key == KEY_ESC) {
      state->running = false;
    }
  }
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = noop,
    .enter = noop,
    .leave = noop,
    .key = keyboard_handle_key,
    .modifiers = noop,
};

static void seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
                                     uint32_t capabilities) {
  struct waytext_seat *seat = data;

  if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
    seat->wl_pointer = wl_seat_get_pointer(wl_seat);
    wl_pointer_add_listener(seat->wl_pointer, &pointer_listener, seat);
  }
  if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
    seat->wl_keyboard = wl_seat_get_keyboard(wl_seat);
    wl_keyboard_add_listener(seat->wl_keyboard, &keyboard_listener, seat);
  }
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
};

static void create_seat(struct waytext_state *state, struct wl_seat *wl_seat) {
  struct waytext_seat *seat = calloc(1, sizeof(struct waytext_seat));
  if (seat == NULL) {
    fprintf(stderr, "allocation failed\n");
    return;
  }
  seat->state = state;
  seat->wl_seat = wl_seat;
  wl_list_insert(&state->seats, &seat->link);
  wl_seat_add_listener(wl_seat, &seat_listener, seat);
}

static void destroy_seat(struct waytext_seat *seat) {
  wl_list_remove(&seat->link);
  wl_surface_destroy(seat->cursor_surface);
  if (seat->wl_pointer) {
    wl_pointer_destroy(seat->wl_pointer);
  }
  if (seat->wl_keyboard) {
    wl_keyboard_destroy(seat->wl_keyboard);
  }
  wl_seat_destroy(seat->wl_seat);
  free(seat);
}

static void output_handle_geometry(void *data, struct wl_output *wl_output,
                                   int32_t x, int32_t y, int32_t physical_width,
                                   int32_t physical_height, int32_t subpixel,
                                   const char *make, const char *model,
                                   int32_t transform) {}

static void output_handle_mode(void *data, struct wl_output *wl_output,
                               uint32_t flags, int32_t width, int32_t height,
                               int32_t refresh) {
  if ((flags & WL_OUTPUT_MODE_CURRENT) == 0) {
    return;
  }
}

static void output_handle_scale(void *data, struct wl_output *wl_output,
                                int32_t scale) {
  struct waytext_output *output = data;

  output->scale = scale;
}

static const struct wl_output_listener output_listener = {
    .geometry = output_handle_geometry,
    .mode = output_handle_mode,
    .done = noop,
    .scale = output_handle_scale,
};

static void xdg_output_handle_logical_position(
    void *data, struct zxdg_output_v1 *xdg_output, int32_t x, int32_t y) {}

static void xdg_output_handle_logical_size(void *data,
                                           struct zxdg_output_v1 *xdg_output,
                                           int32_t width, int32_t height) {}

static const struct zxdg_output_v1_listener xdg_output_listener = {
    .logical_position = xdg_output_handle_logical_position,
    .logical_size = xdg_output_handle_logical_size,
    .done = noop,
    .name = noop,
    .description = noop,
};

static void create_output(struct waytext_state *state,
                          struct wl_output *wl_output) {
  struct waytext_output *output = calloc(1, sizeof(struct waytext_output));
  if (output == NULL) {
    fprintf(stderr, "allocation failed\n");
    return;
  }
  output->wl_output = wl_output;
  output->state = state;
  output->scale = 1;
  wl_list_insert(&state->outputs, &output->link);

  wl_output_add_listener(wl_output, &output_listener, output);
}

static void destroy_output(struct waytext_output *output) {
  if (output == NULL) {
    return;
  }
  wl_list_remove(&output->link);
  finish_buffer(&output->buffers[0]);
  finish_buffer(&output->buffers[1]);
  wl_cursor_theme_destroy(output->cursor_theme);
  zwlr_layer_surface_v1_destroy(output->layer_surface);
  if (output->xdg_output) {
    zxdg_output_v1_destroy(output->xdg_output);
  }
  wl_surface_destroy(output->surface);
  if (output->frame_callback) {
    wl_callback_destroy(output->frame_callback);
  }
  wl_output_destroy(output->wl_output);
  free(output);
}

static const struct wl_callback_listener output_frame_listener;

static void send_frame(struct waytext_output *output) {
  struct waytext_state *state = output->state;

  if (!output->configured) {
    return;
  }

  int32_t buffer_width = output->width * output->scale;
  int32_t buffer_height = output->height * output->scale;

  output->current_buffer =
      get_next_buffer(state->shm, output->buffers, buffer_width, buffer_height);
  if (output->current_buffer == NULL) {
    return;
  }

  render(output);

  // Schedule a frame in case the output becomes dirty again
  output->frame_callback = wl_surface_frame(output->surface);
  wl_callback_add_listener(output->frame_callback, &output_frame_listener,
                           output);

  wl_surface_attach(output->surface, output->current_buffer->buffer, 0, 0);
  wl_surface_damage(output->surface, 0, 0, output->width, output->height);
  wl_surface_set_buffer_scale(output->surface, output->scale);
  wl_surface_commit(output->surface);
  output->dirty = false;
}

static void output_frame_handle_done(void *data, struct wl_callback *callback,
                                     uint32_t time) {
  struct waytext_output *output = data;

  wl_callback_destroy(callback);
  output->frame_callback = NULL;

  if (output->dirty) {
    send_frame(output);
  }
}

static const struct wl_callback_listener output_frame_listener = {
    .done = output_frame_handle_done,
};

static struct waytext_output *output_from_surface(struct waytext_state *state,
                                                  struct wl_surface *surface) {
  struct waytext_output *output;
  wl_list_for_each(output, &state->outputs, link) {
    if (output->surface == surface) {
      return output;
    }
  }
  return NULL;
}

static void layer_surface_handle_configure(
    void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial,
    uint32_t width, uint32_t height) {
  struct waytext_output *output = data;

  output->configured = true;
  output->width = width;
  output->height = height;

  zwlr_layer_surface_v1_ack_configure(surface, serial);
  send_frame(output);
}

static void layer_surface_handle_closed(void *data,
                                        struct zwlr_layer_surface_v1 *surface) {
  struct waytext_output *output = data;
  destroy_output(output);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_handle_configure,
    .closed = layer_surface_handle_closed,
};

static void handle_global(void *data, struct wl_registry *registry,
                          uint32_t name, const char *interface,
                          uint32_t version) {
  struct waytext_state *state = data;

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compositor =
        wl_registry_bind(registry, name, &wl_compositor_interface, 4);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
    state->layer_shell =
        wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
  } else if (strcmp(interface, wl_seat_interface.name) == 0) {
    struct wl_seat *wl_seat =
        wl_registry_bind(registry, name, &wl_seat_interface, 1);
    create_seat(state, wl_seat);
  } else if (strcmp(interface, wl_output_interface.name) == 0) {
    struct wl_output *wl_output =
        wl_registry_bind(registry, name, &wl_output_interface, 3);
    create_output(state, wl_output);
  } else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
    state->xdg_output_manager =
        wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 1);
  }
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = noop,
};

static const char usage[] = "Usage: waytext [options...]\n"
                            "\n"
                            "  -h           Show help message and quit.\n"
                            "  -b #rrggbbaa Set background color.\n"
                            "  -f #rrggbbaa Set foreground color.\n"
                            "  -w n         Set font weight.\n"
                            "  -t s         Set text to display.\n";

uint32_t parse_color(const char *color) {
  if (color[0] == '#') {
    ++color;
  }

  int len = strlen(color);
  if (len != 6 && len != 8) {
    fprintf(stderr,
            "Invalid color %s, "
            "defaulting to color 0xFFFFFFFF\n",
            color);
    return 0xFFFFFFFF;
  }
  uint32_t res = (uint32_t)strtoul(color, NULL, 16);
  if (strlen(color) == 6) {
    res = (res << 8) | 0xFF;
  }
  return res;
}

int main(int argc, char *argv[]) {
  struct waytext_state state = {
      .colors =
          {
              .background = 0xFFFFFF80,
              .foreground = 0x000000FF,
          },
      .font_weight = 100,
      .text = "Your Text Here",
  };

  int opt;
  while ((opt = getopt(argc, argv, "hdb:f:w:t:")) != -1) {
    switch (opt) {
    case 'h':
      printf("%s", usage);
      return EXIT_SUCCESS;
    case 'b':
      state.colors.background = parse_color(optarg);
      break;
    case 'f':
      state.colors.foreground = parse_color(optarg);
      break;
    case 't':
      state.text = optarg;
      break;
    case 'w': {
      errno = 0;
      char *endptr;
      state.font_weight = strtol(optarg, &endptr, 10);
      if (*endptr || errno) {
        fprintf(stderr, "Error: expected numeric argument for -w\n");
        exit(EXIT_FAILURE);
      }
      break;
    }
    default:
      printf("%s", usage);
      return EXIT_FAILURE;
    }
  }

  wl_list_init(&state.outputs);
  wl_list_init(&state.seats);

  state.display = wl_display_connect(NULL);
  if (state.display == NULL) {
    fprintf(stderr, "failed to create display\n");
    return EXIT_FAILURE;
  }

  state.registry = wl_display_get_registry(state.display);
  wl_registry_add_listener(state.registry, &registry_listener, &state);
  wl_display_dispatch(state.display);
  wl_display_roundtrip(state.display);

  if (state.compositor == NULL) {
    fprintf(stderr, "compositor doesn't support wl_compositor\n");
    return EXIT_FAILURE;
  }
  if (state.shm == NULL) {
    fprintf(stderr, "compositor doesn't support wl_shm\n");
    return EXIT_FAILURE;
  }
  if (state.layer_shell == NULL) {
    fprintf(stderr, "compositor doesn't support zwlr_layer_shell_v1\n");
    return EXIT_FAILURE;
  }
  if (state.xdg_output_manager == NULL) {
    fprintf(stderr, "compositor doesn't support xdg-output. Guessing geometry "
                    "from physical output size.\n");
  }
  if (wl_list_empty(&state.outputs)) {
    fprintf(stderr, "no wl_output\n");
    return EXIT_FAILURE;
  }

  struct waytext_output *output;
  wl_list_for_each(output, &state.outputs, link) {
    output->surface = wl_compositor_create_surface(state.compositor);
    // TODO: wl_surface_add_listener(output->surface, &surface_listener,
    // output);

    output->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        state.layer_shell, output->surface, output->wl_output,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "selection");
    zwlr_layer_surface_v1_add_listener(output->layer_surface,
                                       &layer_surface_listener, output);

    if (state.xdg_output_manager) {
      output->xdg_output = zxdg_output_manager_v1_get_xdg_output(
          state.xdg_output_manager, output->wl_output);
      zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener,
                                  output);
    }

    zwlr_layer_surface_v1_set_anchor(output->layer_surface,
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
    zwlr_layer_surface_v1_set_keyboard_interactivity(output->layer_surface,
                                                     true);
    zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface, -1);
    wl_surface_commit(output->surface);

    output->cursor_theme =
        wl_cursor_theme_load(NULL, 24 * output->scale, state.shm);
    if (output->cursor_theme == NULL) {
      fprintf(stderr, "failed to load cursor theme\n");
      return EXIT_FAILURE;
    }
    struct wl_cursor *cursor =
        wl_cursor_theme_get_cursor(output->cursor_theme, "left_ptr");
    if (cursor == NULL) {
      fprintf(stderr, "failed to load cursor\n");
      return EXIT_FAILURE;
    }
  }
  // second roundtrip for xdg-output
  wl_display_roundtrip(state.display);

  struct waytext_seat *seat;
  wl_list_for_each(seat, &state.seats, link) {
    seat->cursor_surface = wl_compositor_create_surface(state.compositor);
  }

  state.running = true;
  while (state.running && wl_display_dispatch(state.display) != -1) {
    // This space intentionally left blank
  }

  struct waytext_output *output_tmp;
  wl_list_for_each_safe(output, output_tmp, &state.outputs, link) {
    destroy_output(output);
  }
  struct waytext_seat *seat_tmp;
  wl_list_for_each_safe(seat, seat_tmp, &state.seats, link) {
    destroy_seat(seat);
  }

  // Make sure the compositor has unmapped our surfaces by the time we exit
  wl_display_roundtrip(state.display);

  zwlr_layer_shell_v1_destroy(state.layer_shell);
  if (state.xdg_output_manager != NULL) {
    zxdg_output_manager_v1_destroy(state.xdg_output_manager);
  }
  wl_compositor_destroy(state.compositor);
  wl_shm_destroy(state.shm);
  wl_registry_destroy(state.registry);
  wl_display_disconnect(state.display);

  return EXIT_SUCCESS;
}
