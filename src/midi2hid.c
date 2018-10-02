#include <stdio.h>
#include <memory.h>
#include <ctype.h>

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
 */
static struct mapping_t mapping[] = {
        {.note = 0x81, .key = "a"},
        {.note = 0x82, .key = "--left-shift a"},
        {.note = 0x83, .key = "--left-alt 6"},
        {.note = 0x84, .key = "--spacebar"},
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
int keyboard_fill_report(__uint8_t report[8], const char *keys) {
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
    for (int i=0; mapping[i].key; i++) {
        struct mapping_t map = mapping[i];
        keyboard_fill_report(map.report, map.key);
        printf("├── Note: %02x\n", map.note);
        printf("│   ├── Keys: %s\n", map.key);
        printf("│   └── Report:");
        for (int k=0; k<8; k++) {
            printf(" %02x", map.report[k]);
        }
        printf("\n│\n");
    }

}

int main() {
    printf("MIDI-2-HiD Adapter\n");
    printf("------------------\n\n");
    initMap();
}