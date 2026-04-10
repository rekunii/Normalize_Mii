/* Normalize_Mii - Fix your Mii
 *
 * This program baased on SpecializeMii
 * (https://github.com/元リポジトリURL)
 * Copyright (C) 2016 phijor
 *
 * Modifications:
 * Copyright (C) 2026 reku
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>

#include "cfldb.h"
#include "crc.h"
#include "mii.h"

// #define PAGELEN 25

// #define COLORIZE(color, str) "\e[" color "m" str "\e[0m"

// void print_usage()
// {
//     PrintConsole *prev = consoleSelect(&c_info);
//     // clang-format off
//     printf(APPLICATION_NAME " - " APPLICATION_REV ":\n"
//             "\n"
//            "[A]      - Toggle special/non-special\n"
//            "[<>^v]   - Navigate Mii list\n"
//            "[SELECT] - Save Changes\n"
//            "[START]  - Exit\n"
//            "\n"
//            "page:slot refers to a Mii's position\n"
//            "in MiiMaker.\n"
//            "\n"
//            "Note that this application currently\n"
//            "does *not* support  UTF16-symbols in\n"
//            "names, such as Hiragana, Kanji, etc.\n"
//            "\n"
//            COLORIZE("1;31", "Important:\n")
//            "Setting a special Mii "COLORIZE("33", "*SHAREABLE*")" or\n"
//            COLORIZE("33", "*COPYABLE*")" in MiiMaker "COLORIZE("33", "*CRASHES*")" the\n"
//            "system and is generally a bad idea.\n"
//            "\n"
//            "I AM NOT RESPONSIBLE FOR EVENTUAL\n"
//            "DAMAGES TO YOUR SYSTEM OR DATA.\n"
//            );
//     // clang-format on
//     consoleSelect(prev);
// }

// bool prompt(char const *const msg, char const *const yes, char const *const no)
// {
//     printf("\033[2J"
//            "%s\n"
//            "[A] - %s\n"
//            "[B] - %s\n",
//            msg,
//            yes,
//            no);
//     u32 key_down = 0;
//     while (aptMainLoop()) {
//         gspWaitForVBlank();
//         hidScanInput();
//         key_down = hidKeysDown();

//         if (key_down & KEY_A) {
//             return true;
//         } else if (key_down & KEY_B) {
//             return false;
//         }
//     }
//     return false;
// }


// char miiname[36];
// char miiauthor[30];
u64 system_id = 0;

u8 mii_pos;
u8 mii_slots[8];

CFL_DB db;
Result res        = 0;
Mii *miis         = NULL;
int mii_count     = 0;

MiiSelectorConf msConf;
MiiSelectorReturn msRet;

void __attribute__((noreturn))
hang(char const *const desc, Result const errcode)
{
    // consoleSelect(&c_info);
    fprintf(stderr,
            "\n"
            "\033[31mAn error occured!\033[0m\n"
            "  Description: %s\n"
            "  Error-Code:  0x%lx\n"
            "  Error-Info:  %s\n"
            "\n"
            "Press START to exit the application.\n",
            desc,
            (long int) errcode,
            osStrError(errcode));

    while (aptMainLoop() && !(hidKeysDown() & KEY_START)) {
        hidScanInput();
    }
    gfxExit();
    exit(errcode);
}

void setmiiblacklist() {
    res = cfldb_open(&db);
    if (R_FAILED(res)) {
        hang("Failed to open CFL_DB.dat", res);
    }

    res = cfldb_read(&db);
    if (R_FAILED(res)) {
        hang("Failed to read CFL_DB.dat", res);
    }

    miis = cfldb_get_mii_array(&db);
    mii_count = cfldb_get_last_mii_index(&db) + 1;
    for (u8 i = 0; i < mii_count; i++) {
        // check all Miis, user's Mii unselectable
        if (miis[i].system_id == system_id) {
            // miiSelectorBlacklistUserMii(&msConf, miis[i].position.page*10 + miis[i].position.slot);
            miiSelectorBlacklistUserMii(&msConf, i);
        }
    }
    
    res = cfldb_close(&db);
    if (R_FAILED(res)) {
        hang("Failed to close CFL_DB.dat", res);
    }
}

void print_usage()
{
    // printf("\033[2J");
	printf("Press START to exit.\n"
        "SELECT to save.\n");
    if (system_id == 0) {
        printf("Press X to select Mii has black pants.\n");
    } else {
        printf("Press Y to select Mii has blue pants.\n");
        printf("3ds system_id: %llx\n", system_id);
    }
    printf(":%u\n", msConf.mii_whitelist[0]);
    printf(":%u\n", msConf.mii_whitelist[1]);
    printf(":%u\n", msConf.mii_whitelist[2]);
    printf(":%u\n", msConf.mii_whitelist[3]);
    printf(":%u\n", msConf.mii_whitelist[4]);
}

int main(void)
{
    // get 3ds system_id
    cfguInit();
    CFGU_GenHashConsoleUnique(0x0, &system_id);

    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

	miiSelectorInit(&msConf);
	miiSelectorSetOptions(&msConf, MIISELECTOR_CANCEL);

    // char **miistrings = NULL;
    
    // setmiiblacklist();

    print_usage();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START) {
            break;
        }
        // if (kDown & KEY_X)
        if (system_id == 0 && (kDown & KEY_X))
		{
			miiSelectorSetTitle(&msConf, "select your Mii");
			miiSelectorLaunch(&msConf, &msRet);
            if (miiSelectorChecksumIsValid(&msRet)) {
                if (!msRet.no_mii_selected)
                {
                    system_id = msRet.mii.system_id;
                    printf("%llx\n", msRet.mii.system_id);
                } else {
                    system_id = 0;
                }

            }
            print_usage();
		}

		// if (kDown & KEY_Y)
		if (system_id != 0 && (kDown & KEY_Y))
		{
			miiSelectorSetTitle(&msConf, "select Mii not yours");
			miiSelectorLaunch(&msConf, &msRet);
			if (miiSelectorChecksumIsValid(&msRet)) {
                // if (system_id == 0)
                // {
                //     printf("please select your Mii first.");
                // } else
                if (!msRet.no_mii_selected)
                {
                    if (msRet.mii.system_id == system_id) {
                        printf("This Mii is already yours.\n");
                    } else {
                        mii_pos = msRet.mii.mii_pos.page_index*10 + msRet.mii.mii_pos.slot_index+1;
                        for (u8 i = 0; i < 8; i++) {
                            if (mii_slots[i] == 0) {
                                mii_slots[i] = mii_pos;
                                break;
                            } else if (mii_slots[i] == mii_pos) {
                                break;
                            }
                        }
                    }
                }
            }
		}

        if (kDown & KEY_SELECT) {
            bool choice = true;
            res = cfldb_open(&db);
            if (R_FAILED(res)) {
                hang("Failed to open CFL_DB.dat", res);
            }

            res = cfldb_read(&db);
            if (R_FAILED(res)) {
                hang("Failed to read CFL_DB.dat", res);
            }
            miis = cfldb_get_mii_array(&db);
            for(u8 i = 0; i < 8; i++) {
                if (mii_slots[i] != 0) {
                    miis[mii_slots[i]-1].sys_id = system_id;
                    mii_slots[i] = 0;
                } else {
                    break;
                }
            }
            if (choice) {                
                printf("Writing database... ");
                cfldb_fix_checksum(&db);
                Result res = cfldb_write(&db);
                if (R_FAILED(res)) {
                    hang("Failed to write database", res);
                }
                printf("done.");
                res = cfldb_close(&db);
                if (R_FAILED(res)) {
                    hang("Failed to close CFL_DB.dat", res);
                }
            }
        }
        gfxFlushBuffers();
		// gfxSwapBuffers();
		gspWaitForVBlank();
    }


    gfxExit();
    return 0;
}
