#include "../sdk/sdk.h"

#define MAP_WIDTH 8
#define MAP_HEIGHT 8
#define COL_GREY 0x8410
// 0 = Empty Space, 1 = Wall Block
static const int map_grid[MAP_WIDTH * MAP_HEIGHT] = {
    1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,1,
    1,0,1,1,0,1,0,1,
    1,0,1,0,0,1,0,1,
    1,0,0,0,0,0,0,1,
    1,0,1,1,1,1,0,1,
    1,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1
};

// --- FIXED POINT CONFIGURATION ---
// We scale numbers up by 256 (shift left by 8 bits) to emulate decimals using integers
#define FP_SHIFT 8
#define FP_ONE   (1 << FP_SHIFT)

// --- TRIGONOMETRIC LOOKUP TABLES ---
// A 360-degree table mapped out in integers scaled by 256. 
// Covers angles from 0 to 359 degrees.
static const int16_t sin_LUT[360] = {
    0, 4, 8, 13, 17, 22, 26, 31, 35, 40, 44, 48, 53, 57, 61, 66, 70, 74, 79, 83, 87, 91, 96, 100, 104, 108, 112, 116, 120, 124, 128, 131, 135, 139, 143, 146, 150, 154, 157, 161, 164, 167, 171, 174, 177, 181, 184, 187, 190, 193, 196, 199, 201, 204, 207, 209, 212, 214, 217, 219, 221, 223, 226, 228, 230, 232, 233, 235, 237, 238, 240, 241, 243, 244, 246, 247, 248, 249, 250, 251, 252, 253, 253, 254, 254, 255, 255, 255, 255, 255, 256, 255, 255, 255, 255, 255, 254, 254, 253, 253, 252, 251, 250, 249, 248, 247, 246, 244, 243, 241, 240, 238, 237, 235, 233, 232, 230, 228, 226, 223, 221, 219, 217, 214, 212, 209, 207, 204, 201, 199, 196, 193, 190, 187, 184, 181, 177, 174, 171, 167, 164, 161, 157, 154, 150, 146, 143, 139, 135, 131, 128, 124, 120, 116, 112, 108, 104, 100, 96, 91, 87, 83, 79, 74, 70, 66, 61, 57, 53, 48, 44, 40, 35, 31, 26, 22, 17, 13, 8, 4, 0, -4, -8, -13, -17, -22, -26, -31, -35, -40, -44, -48, -53, -57, -61, -66, -70, -74, -79, -83, -87, -91, -96, -100, -104, -108, -112, -116, -120, -124, -128, -131, -135, -139, -143, -146, -150, -154, -157, -161, -164, -167, -171, -174, -177, -181, -184, -187, -190, -193, -196, -199, -201, -204, -207, -209, -212, -214, -217, -219, -221, -223, -226, -228, -230, -232, -233, -235, -237, -238, -240, -241, -243, -244, -246, -247, -248, -249, -250, -251, -252, -253, -253, -254, -254, -255, -255, -255, -255, -255, -256, -255, -255, -255, -255, -255, -254, -254, -253, -253, -252, -251, -250, -249, -248, -247, -246, -244, -243, -241, -240, -238, -237, -235, -233, -232, -230, -228, -226, -223, -221, -219, -217, -214, -212, -209, -207, -204, -201, -199, -196, -193, -190, -187, -184, -181, -177, -174, -171, -167, -164, -161, -157, -154, -150, -146, -143, -139, -135, -131, -128, -124, -120, -116, -112, -108, -104, -100, -96, -91, -87, -83, -79, -74, -70, -66, -61, -57, -53, -48, -44, -40, -35, -31, -26, -22, -17, -13, -8, -4
};

static const int16_t cos_LUT[360] = {
    256, 255, 255, 255, 255, 255, 254, 254, 253, 253, 252, 251, 250, 249, 248, 247, 246, 244, 243, 241, 240, 238, 237, 235, 233, 232, 230, 228, 226, 223, 221, 219, 217, 214, 212, 209, 207, 204, 201, 199, 196, 193, 190, 187, 184, 181, 177, 174, 171, 167, 164, 161, 157, 154, 150, 146, 143, 139, 135, 131, 128, 124, 120, 116, 112, 108, 104, 100, 96, 91, 87, 83, 79, 74, 70, 66, 61, 57, 53, 48, 44, 40, 35, 31, 26, 22, 17, 13, 8, 4, 0, -4, -8, -13, -17, -22, -26, -31, -35, -40, -44, -48, -53, -57, -61, -66, -70, -74, -79, -83, -87, -91, -96, -100, -104, -108, -112, -116, -120, -124, -128, -131, -135, -139, -143, -146, -150, -154, -157, -161, -164, -167, -171, -174, -177, -181, -184, -187, -190, -193, -196, -199, -201, -204, -207, -209, -212, -214, -217, -219, -221, -223, -226, -228, -230, -232, -233, -235, -237, -238, -240, -241, -243, -244, -246, -247, -248, -249, -250, -251, -252, -253, -253, -254, -254, -255, -255, -255, -255, -255, -256, -255, -255, -255, -255, -255, -254, -254, -253, -253, -252, -251, -250, -249, -248, -247, -246, -244, -243, -241, -240, -238, -237, -235, -233, -232, -230, -228, -226, -223, -221, -219, -217, -214, -212, -209, -207, -204, -201, -199, -196, -193, -190, -187, -184, -181, -177, -174, -171, -167, -164, -161, -157, -154, -150, -146, -143, -139, -135, -131, -128, -124, -120, -116, -112, -108, -104, -100, -96, -91, -87, -83, -79, -74, -70, -66, -61, -57, -53, -48, -44, -40, -35, -31, -26, -22, -17, -13, -8, -4, 0, 4, 8, 13, 17, 22, 26, 31, 35, 40, 44, 48, 53, 57, 61, 66, 70, 74, 79, 83, 87, 91, 96, 100, 104, 108, 112, 116, 120, 124, 128, 131, 135, 139, 143, 146, 150, 154, 157, 161, 164, 167, 171, 174, 177, 181, 184, 187, 190, 193, 196, 199, 201, 204, 207, 209, 212, 214, 217, 219, 221, 223, 226, 228, 230, 232, 233, 235, 237, 238, 240, 241, 243, 244, 246, 247, 248, 249, 250, 251, 252, 253, 253, 254, 254, 255, 255, 255, 255, 255
};

// Player vectors scaled into Fixed Point (2.5 coordinates * 256)
int32_t player_x = (2 * FP_ONE) + (FP_ONE / 2);
int32_t player_y = (2 * FP_ONE) + (FP_ONE / 2);
int32_t player_angle = 0; // Stored natively in degrees (0 - 359)
const int fov = 60;        // 60-degree Field of View

void main(void) {
    int screen_width = 240;
    int screen_height = 360;
    
    // Shut off watchdog during full display frames
    watchdog_disable();

    for(;;) {
        watchdog_feed();
        fb_clear(COL_BLACK);
        
        // Auto-rotation engine preview
        player_angle = (player_angle + 1) % 360;

        // Trace across screen columns
        for (int x = 0; x < screen_width; x++) {
            if ((x % 16) == 0) watchdog_feed(); 

            // Calculate current ray degrees relative to target viewing direction
            int ray_angle = player_angle - (fov / 2) + (x * fov / screen_width);
            
            // Normalize layout boundaries to 0-359 range loop
            while (ray_angle < 0)   ray_angle += 360;
            while (ray_angle >= 360) ray_angle -= 360;
            
            int32_t distance_to_wall = 0;
            int hit_wall = 0;
            
            // Pull raw directional steps out of fixed point tables
            int32_t eye_x = sin_LUT[ray_angle]; // Already scaled by 256
            int32_t eye_y = cos_LUT[ray_angle]; // Already scaled by 256

            // Trace ray increments forward using pure integer calculations
            while (!hit_wall && distance_to_wall < (16 * FP_ONE)) {
                // Advance ray out by a small 1/16th chunk (16 units in fixed-point)
                distance_to_wall += 16; 
                
                // Shift bits right by 8 to convert scaled vector back into raw map coordinates
                int test_x = (player_x + ((eye_x * distance_to_wall) >> FP_SHIFT)) >> FP_SHIFT;
                int test_y = (player_y + ((eye_y * distance_to_wall) >> FP_SHIFT)) >> FP_SHIFT;
                
                if (test_x < 0 || test_x >= MAP_WIDTH || test_y < 0 || test_y >= MAP_HEIGHT) {
                    hit_wall = 1;
                    distance_to_wall = 16 * FP_ONE;
                } else if (map_grid[test_y * MAP_WIDTH + test_x] > 0) {
                    hit_wall = 1; 
                }
            }

            // Fish-eye lens depth perspective correction calculation
            int diff_angle = ray_angle - player_angle;
            while (diff_angle < 0)   diff_angle += 360;
            while (diff_angle >= 360) diff_angle -= 360;
            
            int32_t corrected_dist = (distance_to_wall * cos_LUT[diff_angle]) >> FP_SHIFT;
            if (corrected_dist < 10) corrected_dist = 10; // Prevent divide-by-zero crashes

            // Calculate vertical slice block limits using integer scaling ratios
            int ceiling = (screen_height / 2) - ((screen_height * FP_ONE) / corrected_dist);
            if (ceiling < 0) ceiling = 0;
            
            int floor = screen_height - ceiling;

            // Paint the dynamic vertical rendering slice column
            for (int y = 0; y < screen_height; y++) {
                if (y < ceiling) {
                    fb_pixel(x, y, COL_BLUE); // Ceiling
                } else if (y >= ceiling && y <= floor) {
                    // Make distant objects automatically drop to grey shade depth layout
                    uint16_t wall_color = (distance_to_wall > (6 * FP_ONE)) ? COL_GREY : COL_WHITE;
                    fb_pixel(x, y, wall_color); // Wall
                } else {
                    fb_pixel(x, y, COL_GREEN); // Floor
                }
            }
        }
        fb_flush();
    }
}

