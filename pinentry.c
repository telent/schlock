#include "cairo.h"
#include "swaylock.h"
#include "loop.h"
#include <sys/param.h>
#include <sodium.h>
#include <fcntl.h>

char *digits[] = {
    "0", "1", "2", "3",
    "4", "5", "6", "7",
    "8", "9",
 /* "A", "B", */
 /*    "C", "D", "E", "F" */
};

char * backspace = "⌫";
char * submit = "\u263a";

char entered_pin[24] = { '\0' };

/* these numbers are unscaled, multiply by surface->scale for px */

const int button_radius = 35;
const int cols = 3;
const int thickness = 2;
const int padding_x = 30;
const int padding_y = 35;
const int num_digits = sizeof(digits) / sizeof(digits[0]);
const int rows = 1 + (num_digits / cols) ;

const int feedback_height = 100;

/* the '+ 2' here is because we draw with a 2 pixel pen, so the
 * dimensions of the stroke centres is less than the size of the
 * drawn area
 */
const int pinpad_width =  (2 +
			   cols * (2 * button_radius) +
			   (cols - 1) * padding_x);

/* the 8 is a fudge, need to find out whh the bottom edge is being
   clipped */
const int pinpad_height = (8 +
			   rows * (2 * button_radius) +
			   (rows - 1) * padding_y +
			   feedback_height);

const int feedback_width = pinpad_width - button_radius - 15;

#define M_PI 3.14159265358979323846

static int x_for_col(int col)
{
    return 1 + button_radius + col * (2 * button_radius + padding_x);
}
static int y_for_row(int row)
{
    return 40 + 1 +
	feedback_height + row * (2 * button_radius + padding_y);
}

void render_centered_text(cairo_t *cairo, int cx, int cy, char *text)
{
    cairo_text_extents_t extents;
    cairo_font_extents_t fe;
    double text_x, text_y;
    cairo_text_extents(cairo, text, &extents);
    cairo_font_extents(cairo, &fe);
    text_x = cx -
	(extents.width / 2 + extents.x_bearing);
    text_y = cy +
	(fe.height / 2 - fe.descent);

    cairo_move_to(cairo, text_x, text_y);
    cairo_show_text(cairo, text);

    cairo_stroke(cairo);
}

static time_t allow_next_attempt = 0;
static int delay_time = 0;
static bool checking = false;
bool in_timeout() {
    return (allow_next_attempt > time(NULL));
}

float frand(float min, float max)
{
    float r = (float) rand();
    return (r * (max-min) / RAND_MAX) + min;
}

static inline unsigned rol(unsigned r, int k) {
    return (r << k) | (r >> (32 - k));
}

void squiggle(cairo_t * cairo, struct swaylock_surface *surface)
{
    unsigned int height = feedback_height * surface->scale;
    unsigned int width = feedback_width * surface->scale;

    /* This is probably quite silly. The goal here is that within
     * a single pin entry attempt ("submit" not pressed) the same
     * curve should be drawn for the same entered_pin, but that
     * different curves are drawn on subsequent attempts
     */

    unsigned int seed = (unsigned int) allow_next_attempt;

    for(size_t i=0; i < strlen(entered_pin); i++) {
	seed ^= entered_pin[i];
	seed = rol(seed, 5);
    }
    srand(seed);


    cairo_set_source_rgba(cairo,
			  frand(0.4,1),
			  frand(0.4,1),
			  frand(0.4,1),
			  0.8);

    cairo_set_line_width(cairo, 6.0 * surface->scale);
    cairo_set_line_cap(cairo, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cairo, 10 * surface->scale, height/2);
    int phase = 1 - (rand() & 2);
    cairo_curve_to(cairo,
		   frand(width * 0.33 - 40,
			 width * 0.33 + 40),
		   height/2 + phase * frand(height * 0.6, height),

		   frand(width * 0.67 - 40,
			 width * 0.67 + 40),
		   height/2 - phase * frand(height * 0.6, height),

		   width - 10,
		   height/2);
    cairo_stroke(cairo);
}

void render_pinentry_pad(cairo_t *cairo, struct swaylock_surface *surface)
{
    if(allow_next_attempt == 0)
	allow_next_attempt = time(NULL)-1;

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
    if(checking) {
	cairo_set_source_rgba(cairo, 1, 0, 0, 0.3);
    } else {
	cairo_set_source_rgba(cairo, 0, 0, 0, 0);
    }
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cairo);
    cairo_restore(cairo);

    cairo_select_font_face(cairo, "sans-serif",
			   CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cairo, sc * button_radius * 1.2f);

    if(in_timeout()) {
	cairo_set_source_u32(cairo, 0x000000ff);
	render_centered_text(cairo,
			     pinpad_width * surface->scale / 2,
			     feedback_height * surface->scale / 2,
			     "wrong");
	cairo_set_font_size(cairo, sc * button_radius * 1.1f);
	cairo_set_source_u32(cairo, 0xc02020ff);
	render_centered_text(cairo,
			     pinpad_width * surface->scale / 2,
			     feedback_height * surface->scale / 2,
			     "wrong");
    }

    if(entered_pin[0]) {
	squiggle(cairo, surface);
    }

    for(int i = 0; i < num_digits; i ++) {
	int r = i / cols;
	int c = i % cols;

	cairo_set_source_u32(cairo, 0x2020ff60);
	cairo_set_line_width(cairo, 3.0 * surface->scale);

	int x = x_for_col(c) * surface->scale;
	int y = y_for_row(r) * surface->scale;

	cairo_arc(cairo, x, y,
		  sc * button_radius, 0, 2 * M_PI);
	cairo_stroke(cairo);

	cairo_set_source_u32(cairo, 0x2020ff70);
	cairo_set_line_width(cairo, 3.0 * surface->scale);
	cairo_arc(cairo, x,y,
		  sc * (button_radius - thickness), 0, 2 * M_PI);
	cairo_fill(cairo);

	char *text = digits[c + cols * r];
	cairo_set_source_u32(cairo, 0x000077);
	render_centered_text(cairo, x + 2, y + 2, text);
	render_centered_text(cairo, x - 1, y - 1, text);

	cairo_set_source_u32(cairo, 0xddddffdd);
	render_centered_text(cairo, x, y, text);
    }
    cairo_set_source_u32(cairo, 0xc02020ff);

    cairo_text_extents_t extents;
    cairo_font_extents_t fe;
    double text_x, text_y;
    cairo_set_font_size(cairo, sc * button_radius);
    cairo_text_extents(cairo, backspace, &extents);
    cairo_font_extents(cairo, &fe);
    text_x = (pinpad_width * surface->scale) -
	(extents.width + extents.x_bearing);

    text_y = (feedback_height/2 * surface->scale) +
	(extents.height / 2);

    cairo_move_to(cairo, text_x, text_y);
    cairo_show_text(cairo, backspace);

    cairo_stroke(cairo);

    cairo_set_font_size(cairo, sc * button_radius * 2.20f);
    cairo_set_source_u32(cairo, 0x000000a0);
    render_centered_text(cairo,
			 x_for_col(cols-1) * surface->scale,
			 y_for_row(rows-1) * surface->scale,
			 submit);
    cairo_set_font_size(cairo, sc * button_radius * 2.10f);
    cairo_set_source_u32(cairo, 0x40c040a0);
    render_centered_text(cairo,
			 x_for_col(cols-1) * surface->scale,
			 y_for_row(rows-1) * surface->scale,
			 submit);
}

static int button_for_xy(int x, int y)
{
    if((x > 215) && (y > 20) && (y< 80))
	return 99;

    y -= feedback_height;
    float colf = 1.0 * (x - button_radius) /
	(button_radius * 2 + padding_x);
    int col = (int) (floor(colf + 0.5));
    if(fabs(colf - col) > 0.4)
	return -1;

    float rowf  = 1.0 * (y - button_radius) /
	(button_radius * 2 + padding_y);
    int row = (int) (floor(rowf + 0.5));
    if(fabs(rowf - row) > 0.4)
	return -1;

    return col + row * cols;
}

void delete_digit()
{
    int offset = strlen(entered_pin);
    if(offset > 0)
	entered_pin[offset - 1] = '\0';
}

bool is_wrong_pin(char *entered, char *expected)
{
    return
	crypto_pwhash_str_verify(expected, entered, strlen(entered));
}

void submit_pin(char * pin_file_path)
{
    char expected[crypto_pwhash_STRBYTES];

    int pw_file = open(pin_file_path, O_RDONLY);

    // need a better way to report missing file
    if(pw_file < 0) return;
    read(pw_file, expected, sizeof expected);
    char *p = strchr(expected, '\n');
    if(p) *p = '\0';

    if(is_wrong_pin(entered_pin, expected)) {
	delay_time = MAX(delay_time * 2, 1);
	allow_next_attempt = time(NULL) + delay_time;
	fprintf(stderr, "submit, times = %ld %ld %d\n",
		time(NULL), allow_next_attempt, delay_time);
	entered_pin[0] = '\0';
    } else {
	exit(0);
    }
}

void enter_pin_digit(char * digit)
{
    if(! in_timeout()) {
	int offset = strlen(entered_pin);
	if(offset + strlen(digit) < sizeof entered_pin) {
	    strcpy(entered_pin + offset, digit);
	}
    }
}

void clear_timeout(void * data)
{
    struct swaylock_state *state = (struct swaylock_state *) data;
    damage_state(state);
}


void perform_pin_check(void * data)
{
        struct swaylock_state *state = (struct swaylock_state *) data;

	submit_pin(state->args.pin_file);
	checking = false;
	damage_state(state);
	loop_add_timer(state->eventloop,
		       1000 * delay_time,
		       clear_timeout,
		       state);
}

void action_for_xy(struct swaylock_state *state, int x, int y)
{
    int b = button_for_xy(x,y);

    if((b >= 0) && (b < num_digits)) {
	enter_pin_digit(digits[b]);
    } else if(b==99) {
	delete_digit();
    } else if(b == 11) {
	checking = true;
	damage_state(state);
	/* we want to provide visual feedback that a check is in
	 * progress, which means we need to let the  main loop
	 * run here. Fire a timer to do the actual checking
	 */
	loop_add_timer(state->eventloop,
		       100,
		       perform_pin_check,
		       state);
    }
}
