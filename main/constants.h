#ifndef CONSTANTS_H
#define CONSTANTS_H

extern const char *SENSORS[];
extern const char *MENU_STRING[];
extern const char *SETTINGS_1[];
extern const char *SETTINGS_2[];
extern const char *TEST_SETTINGS[];
extern const char *MEASURE_TYPES[];
extern const char *TEST_LENGTH[];
extern const int TEST_LENGTH_VALUES[];
extern uint16_t DISPLAY_SETTINGS[];

int wait_enter(float wait_time);

#define REPEAT_CHAR(ch, count) for(int i = 0; i < (count); i++) printf("%s", ch)

#define HOR_SEP_UP(width)    printf("┌"); REPEAT_CHAR("─", width); printf("┐\n")
#define HOR_SEP_MID(width)   printf("├"); REPEAT_CHAR("─", width); printf("┤\n")
#define HOR_SEP_DW(width)    printf("└"); REPEAT_CHAR("─", width); printf("┘\n")
#define HOR_SEP_TABLE_UP(width) printf("┌"); REPEAT_CHAR("─", (width/2)-1); printf("┬"); REPEAT_CHAR("─", (width/2)); printf("┐\n");
#define HOR_SEP_TABLE_MID(width) printf("├"); REPEAT_CHAR("─", (width/2)-1); printf("┼"); REPEAT_CHAR("─", (width/2)); printf("┤\n");
#define HOR_SEP_TABLE_DW(width) printf("└"); REPEAT_CHAR("─", (width/2)-1); printf("┴"); REPEAT_CHAR("─", (width/2)); printf("┘\n");

// Doesn't work for some reason
//#define HOR_SEP_UP(width)    printf("┌%-*c┐\n", width, 126)
//#define HOR_SEP_MID(width)   printf("├%-*c┤\n", width, 196)
//#define HOR_SEP_DW(width)    printf("└%-*c┘\n", width, 196)

#define CLEAR_SCREEN printf("\e[1;1H\e[2J");

int wait_enter(float wait_time);

#endif

