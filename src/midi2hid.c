#include <stdio.h>
#include <memory.h>
#include <ctype.h>
#include <alsa/asoundlib.h>
#include <pthread.h>

static snd_seq_t *seq_handle;
static int in_port;
static int in_client_id;

static __uint8_t BLANK_REPORT[8] = {0, 0, 0, 0, 0, 0, 0, 0};

#define CHK(stmt, msg) if((stmt) < 0) {puts("ERROR: "#msg); exit(1);}

struct mapping_t {
    /**
     * MIDI note to map from
     */
    const __uint8_t note;

    /**
     * Keyboard mapping
     */
    const char *key;

    /**
     * Compiled report
     */
    __uint8_t report[8];
};

/**
 * Defines the note to keystroke mapping.
 * |----------------|--------|
 * | Pad            | Note   |
 * +----------------+--------+
 * | Kick           | `0x24` |
 * | Snare Head     | `0x26` |
 * | Snare Rim      | `0x28` |
 * | Tom 1          | `0x30` |
 * | Tom 2          | `0x2d` |
 * | Tom 3          | `0x2b` |
 * | HH Open Bow    | `0x2e` |
 * | HH Open Edge   | `0x1a` |
 * | HH Closed Bow  | `0x2a` |
 * | HH Closed Edge | `0x16` |
 * | HH foot closed | `0x2c` |
 * | Crash 1 (Bow)  | `0x31` |
 * | Crash 1 (Edge) | `0x37` |
 * | Crash 2 (Bow)  | `0x00` |
 * | Crash 2 (Edge) | `0x00` |
 * | Ride  2 (Bow)  | `0x33` |
 * | Ride  2 (Edge) | `0x3b` |
 */
static struct mapping_t mapping[] = {
        {.note = 0x24, .key = "--spacebar"}, // kick
        {.note = 0x2e, .key = "w"}, // high hat (yellow)
        {.note = 0x1a, .key = "w"}, // high hat (yellow)
        {.note = 0x2a, .key = "w"}, // high hat (yellow)
        {.note = 0x16, .key = "w"}, // high hat (yellow)
        {.note = 0x30, .key = "y"}, // blue tom
        {.note = 0x31, .key = "y"}, // crash (orange?)
        {.note = 0x37, .key = "y"}, // crash (orange?)
        {.note = 0x2d, .key = "h"}, // green tom
        {.note = 0x33, .key = "y"}, // ride (orange?)
        {.note = 0x3b, .key = "y"}, // ride (orange?)
        {.note = 0x26, .key = "s"}, // snare (red)
        {.note = 0x28, .key = "s"}, // snare (red)
        {.key = NULL}
};

struct options {
    const char *opt;
    __uint8_t val;
};

static struct options kmod[] = {
        {.opt = "--left-ctrl", .val = 0x01},
        {.opt = "--right-ctrl", .val = 0x10},
        {.opt = "--left-shift", .val = 0x02},
        {.opt = "--right-shift", .val = 0x20},
        {.opt = "--left-alt", .val = 0x04},
        {.opt = "--right-alt", .val = 0x40},
        {.opt = "--left-meta", .val = 0x08},
        {.opt = "--right-meta", .val = 0x80},
        {.opt = NULL}
};

static struct options kval[] = {
        {.opt = "--return", .val = 0x28},
        {.opt = "--esc", .val = 0x29},
        {.opt = "--bckspc", .val = 0x2a},
        {.opt = "--tab", .val = 0x2b},
        {.opt = "--spacebar", .val = 0x2c},
        {.opt = "--caps-lock", .val = 0x39},
        {.opt = "--f1", .val = 0x3a},
        {.opt = "--f2", .val = 0x3b},
        {.opt = "--f3", .val = 0x3c},
        {.opt = "--f4", .val = 0x3d},
        {.opt = "--f5", .val = 0x3e},
        {.opt = "--f6", .val = 0x3f},
        {.opt = "--f7", .val = 0x40},
        {.opt = "--f8", .val = 0x41},
        {.opt = "--f9", .val = 0x42},
        {.opt = "--f10", .val = 0x43},
        {.opt = "--f11", .val = 0x44},
        {.opt = "--f12", .val = 0x45},
        {.opt = "--insert", .val = 0x49},
        {.opt = "--home", .val = 0x4a},
        {.opt = "--pageup", .val = 0x4b},
        {.opt = "--del", .val = 0x4c},
        {.opt = "--end", .val = 0x4d},
        {.opt = "--pagedown", .val = 0x4e},
        {.opt = "--right", .val = 0x4f},
        {.opt = "--left", .val = 0x50},
        {.opt = "--down", .val = 0x51},
        {.opt = "--kp-enter", .val = 0x58},
        {.opt = "--up", .val = 0x52},
        {.opt = "--num-lock", .val = 0x53},
        {.opt = NULL}
};

/**
 * Finds the option in the given list.
 * @param opts Options list
 * @param tok Token to find
 * @return the value of the option or 0.
 */
__uint8_t findOption(const struct options *opts, const char *tok) {
    for (int i = 0; opts[i].opt != NULL; i++) {
        if (strcmp(tok, opts[i].opt) == 0) {
            return opts[i].val;
        }
    }
    return 0;
}

/**
 * Creates the keyboard HID report.
 * @param report The report buffer
 * @param keys The keys information
 * @param hold
 * @return
 */
int keyboard_fill_report(__uint8_t *report, const char *keys) {
    memset(report, 0x0, 8);
    char buffer[1024];
    strcpy(buffer, keys);
    char *tok = strtok(buffer, " ");
    int key = 0;
    int i = 0;
    for (; tok != NULL; tok = strtok(NULL, " ")) {
        // set modifiers
        unsigned char mod = findOption(kmod, tok);
        if (mod) {
            report[0] = report[0] | kmod[i].val;
            continue;
        }

        // we can _press_ down 6 keys in maximum
        if (key < 6) {
            // check for any 'special' key.
            unsigned char val = findOption(kval, tok);
            if (val) {
                report[2 + key++] = val;
                continue;
            }

            const char t = tok[0];

            if (t >= 'a' && t <= 'z') {
                report[2 + key++] = (unsigned char) (tok[0] - ('a' - 0x04));
                continue;
            } else if (t >= '1' && t <= '9') {
                report[2 + key++] = (unsigned char) (tok[0] - ('1' - 0x1e));
                continue;
            } else if (t == '0') {
                report[2 + key++] = (unsigned char) (0x27);
                continue;
            }

            fprintf(stderr, "unknown option: %s\n", tok);
        }
    }
    return 8;
}

void initMap() {
    printf("Mapping\n");
    for (int i = 0; mapping[i].key; i++) {
        struct mapping_t* map = &mapping[i];
        keyboard_fill_report(map->report, map->key);
        printf("├── Note: %02x\n", map->note);
        printf("│   ├── Keys: %s\n", map->key);
        printf("│   └── Report:");
        for (int k = 0; k < 8; k++) {
            printf(" %02x", map->report[k]);
        }
        printf("\n│\n");
    }
}

int send_report(int fd, __uint8_t* report) {
    printf("sending report: ");
    for (int k = 0; k < 8; k++) {
        printf(" %02x", report[k]);
    }
    printf("\n");

    if (write(fd, report, 8) != 8) {
        perror("hid");
        return 5;
    }
    if (write(fd, BLANK_REPORT, 8) != 8) {
        perror("hid");
        return 6;
    }
    return 0;
}

struct mapping_t* findMap(__uint8_t note) {
    for (int i = 0; mapping[i].key; i++) {
        if (mapping[i].note == note) {
            return &mapping[i];
        }
    }
    return NULL;
}

void midi_open(void) {
    CHK(snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0), "Could not open sequencer");
    CHK(snd_seq_set_client_name(seq_handle, "midi2hid"), "Could not set client name");
    CHK(in_port = snd_seq_create_simple_port(seq_handle, "listen:in",
                                             SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                             SND_SEQ_PORT_TYPE_APPLICATION),
        "Could not open port");
    in_client_id = snd_seq_client_id(seq_handle);
    printf("Started client on %d:%d\n", in_client_id, in_port);
}

void midi_capture(snd_seq_t *seq, int client, int port) {
    snd_seq_addr_t sender, dest;
    snd_seq_port_subscribe_t *subs;
    sender.client = (__uint8_t) client;
    sender.port = (__uint8_t) port;
    dest.client = (__uint8_t) in_client_id;
    dest.port = (__uint8_t) in_port;
    snd_seq_port_subscribe_alloca(&subs);
    snd_seq_port_subscribe_set_sender(subs, &sender);
    snd_seq_port_subscribe_set_dest(subs, &dest);
    snd_seq_port_subscribe_set_queue(subs, 1);
    snd_seq_port_subscribe_set_time_update(subs, 1);
    snd_seq_port_subscribe_set_time_real(subs, 1);
    if (snd_seq_subscribe_port(seq, subs) < 0) {
        fprintf(stderr, "Could not subscribe to %d:%d.\n", dest.client, dest.port);
    } else {
        printf("Subscribed to %d:%d\n", client, port);
    }
}

snd_seq_event_t *midi_read(void) {
    snd_seq_event_t *ev = NULL;
    snd_seq_event_input(seq_handle, &ev);
    return ev;
}

__uint8_t midi_process(const snd_seq_event_t *ev, __uint8_t minVelocity) {
    if (ev->type == SND_SEQ_EVENT_NOTEON) {
        printf("[%d] Note on: %2x vel(%2x)\n", ev->time.tick, ev->data.note.note, ev->data.note.velocity);
        if (ev->data.note.velocity >= minVelocity) {
            return ev->data.note.note;
        }
    } else if (ev->type == SND_SEQ_EVENT_NOTEOFF) {
        printf("[%d] Note off: %2x vel(%2x)\n", ev->time.tick, ev->data.note.note, ev->data.note.velocity);
    } else if (ev->type == SND_SEQ_EVENT_CONTROLLER) {
        printf("[%d] Control:  %2x val(%2x)\n", ev->time.tick, ev->data.control.param, ev->data.control.value);
    } else {
        printf("[%d] Unknown:  Unhandled Event Received\n", ev->time.tick);
    }
    return 0;
}

void *consumeHID(void *vargp) {
    int fd = *(int*) vargp;
    fd_set rfds;
    char buf[512];

    printf("starting HID consumer with fd = %d\n", fd);
    while (fd) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        int retval = select(fd + 1, &rfds, NULL, NULL, NULL);
        if (retval == -1 && errno == EINTR)
            continue;
        if (retval < 0) {
            perror("select()");
            fd = 0;
            continue;
        }
        if (FD_ISSET(fd, &rfds)) {
            ssize_t cmd_len = read(fd, buf, 512 - 1);
            printf("recv report:");
            for (int i = 0; i < cmd_len; i++) {
                printf(" %02x", buf[i]);
            }
            printf("\n");
        }
    }
    printf("Stopping HID consumer\n");
    return 0;
}

int main(int argc, const char *argv[]) {
    __uint8_t minVelocity = 0x30;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s usb-devname\n", argv[0]);
        return 1;
    }
    int fd;
    if ((fd = open(argv[1], O_RDWR, 0666)) == -1) {
        perror(argv[1]);
        return 3;
    }

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, consumeHID, &fd);

    printf("MIDI-2-HiD Adapter\n");
    printf("------------------\n\n");
    initMap();
    midi_open();
    midi_capture(seq_handle, 20, 0);
    printf("listening to midi\n");
    int running = 1;
    while(running) {
        __uint8_t note = midi_process(midi_read(), minVelocity);
        if (note) {
            struct mapping_t* map = findMap(note);
            if (map) {
                printf("note %02x maps to %s\n", note, map->key);
                printf("pre-sending report: ");
                if (send_report(fd, map->report)) {
                    exit(-1);
                }
            } else {
                printf("note %02x is not mapped\n", note);
            }
        }
    }
}