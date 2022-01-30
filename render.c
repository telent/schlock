#include <math.h>
#include <stdlib.h>
#include <wayland-client.h>
#include "cairo.h"
#include "background-image.h"
#include "swaylock.h"


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

	int buffer_width = surface->indicator_width;
	int buffer_height = surface->indicator_height;

	int new_width = surface->scale * pinpad_width;
	int new_height = surface->scale * pinpad_height;


	int subsurf_xpos;
	int subsurf_ypos;

	// centre on the display
	subsurf_xpos = (surface->width - new_width/surface->scale) / 2;
	subsurf_ypos = (surface->height - new_height/surface->scale) / 2;

	wl_subsurface_set_position(surface->subsurface, subsurf_xpos, subsurf_ypos);

	surface->current_buffer = get_next_buffer(state->shm,
						  surface->indicator_buffers, buffer_width, buffer_height);
	if (surface->current_buffer == NULL) {
	    return;
	}

	// Hide subsurface until we want it visible
	wl_surface_attach(surface->child, NULL, 0, 0);
	wl_surface_commit(surface->child);

	render_pinentry_pad(surface->current_buffer->cairo, surface);

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
