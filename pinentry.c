char *digits[] = {
    "0", "1", "2", "3",
    "4", "5", "6", "7",
    "8", "9", "A", "B",
    "C", "D", "E", "F"
};

/* these numbers are unscaled, multiply by surface->scale for px */

int button_radius = 34;
const int cols = 4;
#define rows ((sizeof(digits) / sizeof(digits[0]))/cols)
int thickness = 4;
int padding_x = 45;
int padding_y = 45;

/* the '+ 2' here is because we draw with a 2 pixel pen, so the
 * dimensions of the stroke centres is less than the size of the
 * drawn area
 */

#define pinpad_width (2+(cols * (2*button_radius) + (cols - 1)*padding_x))
#define pinpad_height (2+(rows * (2*button_radius) + (rows - 1)*padding_y))

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
#if 0

static char button_for_xy(int x, int y)
{
    return '\0';
}
#endif
