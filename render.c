#include <math.h>
#include <stdlib.h>
#include <wayland-client.h>
#include "cairo.h"
#include "background-image.h"
#include "swaylock.h"

#include "pinentry.c"

#define M_PI 3.14159265358979323846
const float TYPE_INDICATOR_RANGE = M_PI / 3.0f;
const float TYPE_INDICATOR_BORDER_THICKNESS = M_PI / 128.0f;


void render_frame_background(struct swaylock_surface *surface) {
	struct swaylock_state *state = surface->state;

	int buffer_width = surface->width * surface->scale;
	int buffer_height = surface->height * surface->scale;
	if (buffer_width == 0 || buffer_height == 0) {
		return; // not yet configured
	}

	surface->current_buffer = get_next_buffer(state->shm,
			surface->buffers, buffer_width, buffer_height);
	if (surface->current_buffer == NULL) {
		return;
	}

	cairo_t *cairo = surface->current_buffer->cairo;
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);

	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_u32(cairo, state->args.colors.background);
	cairo_paint(cairo);
	if (surface->image && state->args.mode != BACKGROUND_MODE_SOLID_COLOR) {
		cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
		render_background_image(cairo, surface->image,
			state->args.mode, buffer_width, buffer_height);
	}
	cairo_restore(cairo);
	cairo_identity_matrix(cairo);

	wl_surface_set_buffer_scale(surface->surface, surface->scale);
	wl_surface_attach(surface->surface, surface->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(surface->surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(surface->surface);
}

void render_frame(struct swaylock_surface *surface) {
	struct swaylock_state *state = surface->state;

	int button_rows = rows;
	int button_cols = cols;

	int arc_radius = button_radius * surface->scale;
	int arc_thickness = thickness * surface->scale;

	int buffer_width = surface->indicator_width;
	int buffer_height = surface->indicator_height;

	int new_width = pinpad_width;
	int new_height = pinpad_height;


	int subsurf_xpos;
	int subsurf_ypos;

	// centre on the display
	subsurf_xpos = (surface->width - new_width) / 2;
	subsurf_ypos = (surface->height - new_height) / 2;


	wl_subsurface_set_position(surface->subsurface, subsurf_xpos, subsurf_ypos);

	surface->current_buffer = get_next_buffer(state->shm,
			surface->indicator_buffers, buffer_width, buffer_height);
	if (surface->current_buffer == NULL) {
		return;
	}

	// Hide subsurface until we want it visible
	wl_surface_attach(surface->child, NULL, 0, 0);
	wl_surface_commit(surface->child);

	cairo_t *cairo = surface->current_buffer->cairo;
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);
	cairo_font_options_t *fo = cairo_font_options_create();
	cairo_font_options_set_hint_style(fo, CAIRO_HINT_STYLE_FULL);
	cairo_font_options_set_antialias(fo, CAIRO_ANTIALIAS_SUBPIXEL);
	cairo_font_options_set_subpixel_order(fo, to_cairo_subpixel_order(surface->subpixel));
	cairo_set_font_options(cairo, fo);
	cairo_font_options_destroy(fo);
	cairo_identity_matrix(cairo);

	// Clear
	cairo_save(cairo);
	cairo_set_source_rgba(cairo, 0, 0, 0, 0);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cairo);
	cairo_restore(cairo);

	cairo_select_font_face(cairo, state->args.font,
			       CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cairo, arc_radius * 1.2f);

	for(int c = 0; c < button_cols; c ++) {
	    for(int r = 0; r < button_rows; r ++) {
		cairo_set_source_u32(cairo, 0x2020ff60);
		cairo_set_line_width(cairo, 3.0 * surface->scale);

		int x = x_for_col(c) * surface->scale;
		int y = y_for_row(r) * surface->scale;

		cairo_arc(cairo, x, y,
			  arc_radius, 0, 2 * M_PI);
		cairo_stroke(cairo);

		cairo_set_line_width(cairo, 3.0 * surface->scale);
		cairo_arc(cairo, x,y,
			  arc_radius - arc_thickness, 0, 2 * M_PI);
		cairo_stroke(cairo);

		char *text = digits[c + button_cols * r];

		cairo_text_extents_t extents;
		cairo_font_extents_t fe;
		double text_x, text_y;
		cairo_text_extents(cairo, text, &extents);
		cairo_font_extents(cairo, &fe);
		text_x = x -
		    (extents.width / 2 + extents.x_bearing);
		text_y = y +
		    (fe.height / 2 - fe.descent);

		cairo_set_source_u32(cairo, 0x000077);
		cairo_move_to(cairo, text_x + 2, text_y + 2);
		cairo_show_text(cairo, text);
		cairo_move_to(cairo, text_x - 1, text_y - 1);
		cairo_show_text(cairo, text);

		cairo_set_source_u32(cairo, 0xddddffdd);
		cairo_move_to(cairo, text_x, text_y);
		cairo_show_text(cairo, text);

		cairo_stroke(cairo);
		// cairo_close_path(cairo);
		// cairo_new_sub_path(cairo);
	    }
	}

	// Ensure buffer size is multiple of buffer scale - required by protocol
	new_height += surface->scale - (new_height % surface->scale);
	new_width += surface->scale - (new_width % surface->scale);

	if (buffer_width != new_width || buffer_height != new_height) {
		destroy_buffer(surface->current_buffer);
		surface->indicator_width = new_width;
		surface->indicator_height = new_height;
		render_frame(surface);
		return;
	}

	wl_surface_set_buffer_scale(surface->child, surface->scale);
	wl_surface_attach(surface->child, surface->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(surface->child, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(surface->child);

	wl_surface_commit(surface->surface);
}

void render_frames(struct swaylock_state *state) {
	struct swaylock_surface *surface;
	wl_list_for_each(surface, &state->surfaces, link) {
		render_frame(surface);
	}
}
