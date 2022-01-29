#include <math.h>
#include <stdlib.h>
#include <wayland-client.h>
#include "cairo.h"
#include "background-image.h"
#include "swaylock.h"

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

char *digits[] = {
    "0", "1", "2", "3",
    "4", "5", "6", "7",
    "8", "9", "A", "B",
    "C", "D", "E", "F"
};

void render_frame(struct swaylock_surface *surface) {
	struct swaylock_state *state = surface->state;

	int button_rows = 4;
	int button_cols = 4;

	int arc_radius = 30 * surface->scale;
	int arc_thickness = 4 * surface->scale;
	int padding = 25 * surface->scale;

	int buffer_width = surface->indicator_width;
	int buffer_height = surface->indicator_height;

	int new_width = (arc_radius + arc_thickness + padding) * 2 * button_cols;
	int new_height = (arc_radius + arc_thickness + padding) * 2 * button_rows;

	int first_x = (arc_radius + arc_thickness) + padding;
	int first_y = (arc_radius + arc_thickness) + padding;

	int subsurf_xpos;
	int subsurf_ypos;

	// centre on the display
	subsurf_xpos = (surface->width - new_width) / 2;
	subsurf_ypos = (surface->height - new_height) / 2;

	fprintf(stderr, "offset %d %d\n", subsurf_xpos, subsurf_xpos);


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

		int x = first_x +
		    c * ((arc_radius + arc_thickness + padding) * 2);
		int y = first_y +
		    r * ((arc_radius + arc_thickness + padding) * 2);
		cairo_arc(cairo, x, y,
			  arc_radius - arc_thickness / 3, 0, 2 * M_PI);
		cairo_stroke(cairo);

		cairo_set_line_width(cairo, 3.0 * surface->scale);
		cairo_arc(cairo, x,y,
			  arc_radius + arc_thickness / 2, 0, 2 * M_PI);
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

#if 0
	if (state->args.show_indicator && (state->auth_state != AUTH_STATE_IDLE ||
			state->args.indicator_idle_visible)) {
		// Fill inner circle
		cairo_set_line_width(cairo, 0);
		cairo_arc(cairo, buffer_width / 2, buffer_diameter / 2,
				arc_radius - arc_thickness / 2, 0, 2 * M_PI);
		set_color_for_state(cairo, state, &state->args.colors.inside);
		cairo_fill_preserve(cairo);
		cairo_stroke(cairo);

		cairo_stroke(cairo);


		if (new_width < extents.width) {
		    new_width = extents.width;
		}

		// Typing indicator: Highlight random part on keypress
		if (state->auth_state == AUTH_STATE_INPUT
				|| state->auth_state == AUTH_STATE_BACKSPACE) {
			static double highlight_start = 0;
			highlight_start +=
				(rand() % (int)(M_PI * 100)) / 100.0 + M_PI * 0.5;
			cairo_arc(cairo, buffer_width / 2, buffer_diameter / 2,
					arc_radius, highlight_start,
					highlight_start + TYPE_INDICATOR_RANGE);
			if (state->auth_state == AUTH_STATE_INPUT) {
				if (state->xkb.caps_lock && state->args.show_caps_lock_indicator) {
					cairo_set_source_u32(cairo, state->args.colors.caps_lock_key_highlight);
				} else {
					cairo_set_source_u32(cairo, state->args.colors.key_highlight);
				}
			} else {
				if (state->xkb.caps_lock && state->args.show_caps_lock_indicator) {
					cairo_set_source_u32(cairo, state->args.colors.caps_lock_bs_highlight);
				} else {
					cairo_set_source_u32(cairo, state->args.colors.bs_highlight);
				}
			}
			cairo_stroke(cairo);

			// Draw borders
			cairo_set_source_u32(cairo, state->args.colors.separator);
			cairo_arc(cairo, buffer_width / 2, buffer_diameter / 2,
					arc_radius, highlight_start,
					highlight_start + type_indicator_border_thickness);
			cairo_stroke(cairo);

			cairo_arc(cairo, buffer_width / 2, buffer_diameter / 2,
					arc_radius, highlight_start + TYPE_INDICATOR_RANGE,
					highlight_start + TYPE_INDICATOR_RANGE +
						type_indicator_border_thickness);
			cairo_stroke(cairo);
		}

		// Draw inner + outer border of the circle
		set_color_for_state(cairo, state, &state->args.colors.line);
		cairo_set_line_width(cairo, 2.0 * surface->scale);
		cairo_arc(cairo, buffer_width / 2, buffer_diameter / 2,
				arc_radius - arc_thickness / 2, 0, 2 * M_PI);
		cairo_stroke(cairo);
		cairo_arc(cairo, buffer_width / 2, buffer_diameter / 2,
				arc_radius + arc_thickness / 2, 0, 2 * M_PI);
		cairo_stroke(cairo);

		// display layout text separately
		if (layout_text) {
			cairo_text_extents_t extents;
			cairo_font_extents_t fe;
			double x, y;
			double box_padding = 4.0 * surface->scale;
			cairo_text_extents(cairo, layout_text, &extents);
			cairo_font_extents(cairo, &fe);
			// upper left coordinates for box
			x = (buffer_width / 2) - (extents.width / 2) - box_padding;
			y = buffer_diameter;

			// background box
			cairo_rectangle(cairo, x, y,
				extents.width + 2.0 * box_padding,
				fe.height + 2.0 * box_padding);
			cairo_set_source_u32(cairo, state->args.colors.layout_background);
			cairo_fill_preserve(cairo);
			// border
			cairo_set_source_u32(cairo, state->args.colors.layout_border);
			cairo_stroke(cairo);

			// take font extents and padding into account
			cairo_move_to(cairo,
				x - extents.x_bearing + box_padding,
				y + (fe.height - fe.descent) + box_padding);
			cairo_set_source_u32(cairo, state->args.colors.layout_text);
			cairo_show_text(cairo, layout_text);
			cairo_new_sub_path(cairo);

			new_height += fe.height + 2 * box_padding;
			if (new_width < extents.width + 2 * box_padding) {
				new_width = extents.width + 2 * box_padding;
			}
		}
	}
#endif
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
