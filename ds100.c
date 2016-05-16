
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>

#include "hidapi.h"

//#define DEBUG

#define MIN(A, B) ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __a : __b; })
#define MAX(A, B) ({ __typeof__(A) __a = (A); __typeof__(B) __b = (B); __a < __b ? __b : __a; })

/* USB IDs for MSI Interceptor DS100 */
#define ID_VENDOR 0x04D9
#define ID_PRODUCT 0xFA52

#define BUFFER_SIZE 16

/* Sets the delay in microseconds between two consecutive calls of change_color() */
#define DELAY 5000

/* The absolute change of RGB values in each iteration of either rainbow_loop() or grayscale_loop() demos */
#define STEPS 5

typedef enum FlashSpeed {
    NONE, SLOW, MEDIUM, FAST
} FlashSpeed;

typedef struct Prefs {
    hid_device *handle;
    int lights_on;
    FlashSpeed flash_speed;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Prefs;

/*
 * Used to store device preferences which will be later used to save
 * Important ones are the ability to disable flashing and setting a custom RGB color since
 * they are not available in the original program
 */
static Prefs prefs = {
    .handle = NULL,
    .lights_on = 1,
    .flash_speed = NONE,
    .r = 0xFF,
    .g = 0,
    .b = 0
};

void change_color()
{
    // the device uses reverted values for RGB color space
    uint8_t r = 0xFF - prefs.r;
    uint8_t g = 0xFF - prefs.g;
    uint8_t b = 0xFF - prefs.b;

#ifdef DEBUG
    printf("r: %3d g: %3d b: %3d\n", r, g, b);
#endif

    // notice that the order is gbr instead of rgb for some reason
    uint8_t buf[BUFFER_SIZE] = {0x02, 0x04, g, b, r, prefs.lights_on, prefs.flash_speed};

    int res = hid_send_feature_report(prefs.handle, buf, BUFFER_SIZE);
    if (res == -1) {
        printf("[!] Could not send packet...\n");
        exit(-1);
    }

    usleep(DELAY);
}

/*
 * Deactivates the mouse (i.e you won't be able use your mouse until you quit the program or re-plug the device in)
 * Used in vendor's control panel before changing the color of the device
 * The device works fine without using this call, however if you do so you will see
 * your cursor moving randomly probably because of the frequent color changes
 */
void stop_mouse()
{
    uint8_t buf[BUFFER_SIZE] = {0x02, 0x06, 0x00};

    int res = hid_send_feature_report(prefs.handle, buf, BUFFER_SIZE);
    if (res == -1) {
        printf("[!] Could not send packet...\n");
        exit(-1);
    }
}

void activate_mouse()
{
    uint8_t buf[BUFFER_SIZE] = {0x02, 0x06, 0x01};

    int res = hid_send_feature_report(prefs.handle, buf, BUFFER_SIZE);
    if (res == -1) {
        printf("[!] Could not send packet...\n");
        exit(-1);
    }
}


void save_color_prefs()
{
    // the device uses reverted values for RGB color space
    uint8_t r = 0xFF - prefs.r;
    uint8_t g = 0xFF - prefs.g;
    uint8_t b = 0xFF - prefs.b;

    uint8_t buf[BUFFER_SIZE] = {0x02, 0x02, 0x81, 0x08, 0x06, 0x00, 0xFA, 0xFA, g, b, r, prefs.lights_on, prefs.flash_speed};

    int res = hid_send_feature_report(prefs.handle, buf, BUFFER_SIZE);
    if (res == -1) {
        printf("[!] Could not send packet...\n");
        exit(-1);
    }
}

/* Used to send the activate signal to the mouse and save new colors when Ctrl-C is used */
void sigint_callback(int sig)
{
    activate_mouse();
    save_color_prefs();

    exit(0);
}

void rainbow_loop()
{
    while (1) {
        while (prefs.r > 0 && prefs.g < 0xFF) {
            // prevent overflow when a STEPS constant which doesn't divide 255 is used
            prefs.r = MAX(prefs.r - STEPS, 0);
            prefs.g = MIN(prefs.g + STEPS, 0xFF);

            change_color();
        }

        while (prefs.g > 0 && prefs.b < 0xFF) {
            prefs.g = MAX(prefs.g - STEPS, 0);
            prefs.b = MIN(prefs.b + STEPS, 0xFF);

            change_color();
        }

        while (prefs.b > 0 && prefs.r < 0xFF) {
            prefs.b = MAX(prefs.b - STEPS, 0);
            prefs.r = MIN(prefs.r + STEPS, 0xFF);

            change_color();
        }
    }
}

void grayscale_loop()
{
    while (1) {
        while (prefs.r < 0xFF & prefs.g < 0xFF & prefs.b < 0xFF) {
            prefs.r = MIN(prefs.r + STEPS, 0xFF);
            prefs.g = MIN(prefs.g + STEPS, 0xFF);
            prefs.b = MIN(prefs.b + STEPS, 0xFF);

            change_color();
        }

        while (prefs.r > 0 && prefs.g > 0 & prefs.b > 0) {
            prefs.r = MAX(prefs.r - STEPS, 0);
            prefs.g = MAX(prefs.g - STEPS, 0);
            prefs.b = MAX(prefs.b - STEPS, 0);

            change_color();
        }
    }
}

int main(int argc, char **argv)
{
    prefs.handle = hid_open(ID_VENDOR, ID_PRODUCT, NULL);
    if (prefs.handle == NULL) {
        printf("[!] Could not connect to the device...\n");
        return -1;
    }

    printf("Use Control-C to take the control of the device back or unplug the device to stop...\n");
    signal(SIGINT, sigint_callback);

    stop_mouse();

    rainbow_loop();
    //grayscale_loop();

    return 0;
}
