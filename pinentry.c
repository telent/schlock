char *digits[] = {
    "0", "1", "2", "3",
    "4", "5", "6", "7",
    "8", "9", "A", "B",
    "C", "D", "E", "F"
};

/* these numbers are unscaled, multiply by surface->scale for px */

const int button_radius = 34;
const int cols = 4;
const int thickness = 4;
const int padding_x = 45;
const int padding_y = 45;
const int rows = sizeof(digits) / sizeof(digits[0]) / cols ;

/* the '+ 2' here is because we draw with a 2 pixel pen, so the
 * dimensions of the stroke centres is less than the size of the
 * drawn area
 */
int pinpad_width =  (2 +
		     cols * (2 * button_radius) +
		     (cols - 1) * padding_x);

int pinpad_height =  (2 +
		      rows * (2 * button_radius) +
		      (rows - 1) * padding_y);


/* button 0 centre is at offset (button_radius)
 *              right edge at 2* (button_radius)
 * button 1 left edge at 2* (button_radius) + padding
 *             centre  at 2* (button_radius) + padding
 *                       + (button_radius)
 * => centres distance is 2* (button_radius) + padding
 * => width = button 0 centre + (rows-1)*(centres distance) + radius
 */

static int x_for_col(int col)
{
    return 1 + button_radius + col * (2 * button_radius + padding_x);
}
static int y_for_row(int row)
{
    return 1 + button_radius + row * (2 * button_radius + padding_y);
}

void render_pinentry_pad(cairo_t *cairo, struct swaylock_surface *surface)
{
    int sc = surface->scale;
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

    cairo_select_font_face(cairo, "sans-serif",
			   CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo, sc * button_radius * 1.2f);

    for(int c = 0; c < cols; c ++) {
	for(int r = 0; r < rows; r ++) {
	    cairo_set_source_u32(cairo, 0x2020ff60);
	    cairo_set_line_width(cairo, 3.0 * surface->scale);

	    int x = x_for_col(c) * surface->scale;
	    int y = y_for_row(r) * surface->scale;

	    cairo_arc(cairo, x, y,
		      sc * button_radius, 0, 2 * M_PI);
	    cairo_stroke(cairo);

	    cairo_set_line_width(cairo, 3.0 * surface->scale);
	    cairo_arc(cairo, x,y,
		      sc * (button_radius - thickness), 0, 2 * M_PI);
	    cairo_stroke(cairo);

	    char *text = digits[c + cols * r];

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

	}
    }
}
#if 0

static char button_for_xy(int x, int y)
{
    return '\0';
}
#endif
