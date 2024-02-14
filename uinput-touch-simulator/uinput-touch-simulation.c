#include <linux/uinput.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

const char *uinput_deivce_path = "/dev/uinput";

void gets(char *buf) {
        fgets(buf, 10, stdin);
}

int open_uinput_device(bool act_as_mouse){
    struct uinput_user_dev ui_dev;
    int uinp_fd = open(uinput_deivce_path, O_WRONLY | O_NDELAY);
    if (uinp_fd <= 0) {
        printf("could not open %s, %s\n", uinput_deivce_path, strerror(errno));
        return -1;
    }

    memset(&ui_dev, 0, sizeof(ui_dev));
    strncpy(ui_dev.name, "VirtualTouch", UINPUT_MAX_NAME_SIZE);
    ui_dev.id.bustype = BUS_USB;
    ui_dev.id.vendor = 0x1341;
    ui_dev.id.product = 0x0001;
    ui_dev.id.version = 4;

    ui_dev.absmin[ABS_X] = 0;
    ui_dev.absmax[ABS_X] = 1023;
    ui_dev.absmin[ABS_Y] = 0;
    ui_dev.absmax[ABS_Y] = 1023;


    //enable direct
    ioctl(uinp_fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    //enable touch event
    ioctl(uinp_fd, UI_SET_EVBIT, EV_ABS);
    ioctl(uinp_fd, UI_SET_ABSBIT, ABS_X);
    ioctl(uinp_fd, UI_SET_ABSBIT, ABS_Y);

    ioctl(uinp_fd, UI_SET_EVBIT, EV_SYN);
    ioctl(uinp_fd, UI_SET_EVBIT, EV_KEY);

    if (act_as_mouse)
        ioctl(uinp_fd, UI_SET_KEYBIT, BTN_MOUSE);
    else
        ioctl(uinp_fd, UI_SET_KEYBIT, BTN_TOUCH);

    write(uinp_fd, &ui_dev, sizeof(ui_dev));
    if (ioctl(uinp_fd, UI_DEV_CREATE)) {
        printf("Unable to create UINPUT device.\n");
        return -1;
    }
    return uinp_fd;
}

void emit(int uinp_fd, int type, int code, int value) {
    struct input_event event;

    // Move pointer to (100,100) location
    memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, NULL);
    event.type = type;
    event.code = code;
    event.value = value;
    write(uinp_fd, &event, sizeof(event));
    //printf("SendEventToUinput done:%d %d %d\n",type, code, value);
}

void move_and_press(int fd, int x, int y) {
    printf("Moving to %d,%d\n",x,y);
    emit(fd, EV_ABS, ABS_X, x);
    emit(fd, EV_ABS, ABS_Y, y);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    emit(fd, EV_KEY, BTN_TOUCH, 1);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    usleep(1000*300);

    emit(fd, EV_KEY, BTN_TOUCH, 0);
    emit(fd, EV_SYN, SYN_REPORT, 0);
    usleep(1000*100);


}

void usage(const char *prgname) {
    fprintf(stderr, "usage %s [--help|-h][--mouse][<points>]\n"
        "--help|-h     show this help\n"
        "--mouse       act as 'calibratable' mouse\n"
        "--extreme     point at extremities, not 1/8th in\n"
        "<points>      chars sequence in the range '0'..'3' where\n"
        "              each char is a point in the screen as the table below\n"
        "\n"
        "              char         point (x, y)\n"
        "              ------       -------------------\n"
        "               0           (width/8, height/8)\n"
        "               1           (width*7/8, height/8)\n"
        "               2           (width/8, height*7/8)\n"
        "               3           (width*7/8, height*7/8)\n"
        "\n"
        "When the program is started, it creates a virtual touch screen\n"
        "called 'VirtualTouch'. Then it ask a <points> set; if no <points>\n"
        "set is passed, the default one ('0123') or the one passed via\n"
        "the command line is used. Then it waits 3 seconds (so the user can\n"
        "starts xinput_calibrator. After that the program 'emits' the\n"
        "touches following the <points> set .\n"
        "\n",
        prgname);
}

int main(int argc, char *argv[]) {
    char buf[] = "0123", *p;
    int fd;
    int i;
    bool act_as_mouse = false;
    bool extremes = false;

    for (i = 1 ; i < argc ; i++) {
        if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {
            usage(argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "--mouse")) {
            act_as_mouse = true;
        } else if(!strcmp(argv[i], "--extreme")) {
            extremes = true;
        } else {
            strncpy(buf, argv[i], 4);
        }
    }

    fd = open_uinput_device(act_as_mouse);
    assert(fd >= 0);
    printf("Device opened\n");

    int yu,yb,xl,xr;
    if(extremes) {
        yu = xl = 0;
        yb = xr = 1023;
    } else {
        yu = xl = 1024*1/8;
        yb = xr = 1024*7/8;
    }

    for(;;) {
        char buf1[100];
        int r;

        printf("Insert pattern (default '%s') >", buf);
        assert(fgets(buf1, 99, stdin));
        r = strlen(buf1);
        assert(r > 0);

        if (r > 1) {
            buf1[r-1] = 0;
            strcpy(buf, buf1);
        }
        p = buf;

        printf("sleep 3s\n"); sleep(3);
        while (*p) {
            switch (*p) {
                case '0':
                    printf("upper left\n");
                    move_and_press(fd, xl, yu);   /* upper left */
                    break;
                case '1':
                    printf("upper right\n");
                    move_and_press(fd, xr, yu);   /* upper right */
                    break;
                case '2':
                    printf("bottom left\n");
                    move_and_press(fd, xl, yb);   /* bottom left */
                    break;
                case '3':
                    printf("bottom right\n");
                    move_and_press(fd, xr, yb);   /* bottom right */
                    break;
                default:
                    printf("Unknown command '%c'\n", *p);
                    break;
            }
            ++p;
        }
        printf("Clicks emitted\n");

    }




    close(fd);

    return 0;

}
