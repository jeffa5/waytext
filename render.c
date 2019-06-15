#include <cairo/cairo.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/pool-buffer.h"
#include "include/render.h"
#include "include/waytext.h"

static void set_source_u32(cairo_t *cairo, uint32_t color) {
  cairo_set_source_rgba(cairo, (color >> (3 * 8) & 0xFF) / 255.0,
                        (color >> (2 * 8) & 0xFF) / 255.0,
                        (color >> (1 * 8) & 0xFF) / 255.0,
                        (color >> (0 * 8) & 0xFF) / 255.0);
}

void render(struct waytext_output *output) {
  struct waytext_state *state = output->state;
  struct pool_buffer *buffer = output->current_buffer;
  cairo_t *cairo = buffer->cairo;
  int32_t scale = output->scale;

  // Clear
  cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
  set_source_u32(cairo, state->colors.background);
  cairo_paint(cairo);

  set_source_u32(cairo, state->colors.foreground);
  cairo_select_font_face(cairo, "Sans", CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cairo, state->font_weight * scale);

  cairo_text_extents_t extents;
  cairo_text_extents(cairo, state->text, &extents);
  cairo_move_to(
      cairo,
      ((output->width / 2) - (extents.width / 2 + extents.x_bearing)) * scale,
      ((output->height / 2) - (extents.height / 2 + extents.y_bearing)) *
          scale);
  cairo_show_text(cairo, state->text);
}
