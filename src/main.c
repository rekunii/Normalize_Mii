/* Normalize_Mii - Everyone your Mii
 *
 * This program based on SpecializeMii
 * (https://github.com/phijor/SpecializeMii)
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

#include <stdio.h>
#include <stdlib.h>
 
//  #include <3ds.h>
#include <3ds/applets/miiselector.h>
#include <3ds/services/apt.h>
#include <3ds/services/cfgu.h>
#include <3ds/services/hid.h>

#include <3ds/console.h>
#include <3ds/gfx.h>
#include <3ds/os.h>
#include <3ds/types.h>

#include "cfldb.h"
#include "crc.h"
#include "mii.h"

#define MII_HOLDER_SIZE 8

static CFL_DB db;
static Mii *miis;
static int mii_count;
static MiiSelectorConf msConf;
static u64 system_id;

static void res_hang(Result res, const char* action){
    if (res < 0) {
        printf(
            "\n"
            "\x1b[31mAn error occured!\x1b[0m\n"
            "  Description: Failed to %s CFL_DB.dat"
            "  Code:  0x%lx\n"
            "\nPress START to exit the application.",
            action,
            res
        );
        while (aptMainLoop() && !(hidKeysDown() & KEY_START)) {
            hidScanInput();
        }
        gfxExit();
        exit(res);
    }
    return;
}

static inline void cfldb_open_read(void){
    res_hang(cfldb_open(&db), "open");
    res_hang(cfldb_read(&db), "read");
}

static inline void setmiiblacklist(void) {
    cfldb_open_read();
    miis = cfldb_get_mii_array(&db);
    mii_count = cfldb_get_last_mii_index(&db) + 1;
    for (u8 i = 0; i < mii_count; i++) { // check all Miis, user's Mii unselectable
        if (miis[i].system_id == system_id) {
            msConf.mii_whitelist[i] = 0; // unselectable, user can't choose this Mii. same as miiSelectorBlacklistUserMii(&msConf, i);
        } else {
            msConf.mii_whitelist[i] = 1; // selectable
        }
    }
    res_hang(cfldb_close(&db), "close");
}

int main(void)
{
    MiiSelectorReturn msRet;
    MiiPosition mii_slots[MII_HOLDER_SIZE];
    MiiPosition temp_mii_pos;
    bool need_blacklist_update = true;
    bool need_display_slots_refresh = false;

    cfguInit();
    CFGU_GenHashConsoleUnique(0x0, &system_id);
    // system_id = 0x123456789abcdef0; // 動作確認用

    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

	miiSelectorInit(&msConf);
	miiSelectorSetOptions(&msConf, MIISELECTOR_CANCEL);
    
    puts("Press Y to select Mii has blue pants.");
    printf("3DS system_id: %llx\n\n", system_id);
    for (u8 i = 0; i < MII_HOLDER_SIZE; i++) {
        printf("SLOT [%u/%u]\n", i+1, MII_HOLDER_SIZE);
    }
    printf("\nSTART to exit.\nSELECT to save.");

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break;

		if (system_id != 0 && (kDown & KEY_Y))
		{
            if (need_blacklist_update) {
                setmiiblacklist();
                need_blacklist_update = false;
            }
            miiSelectorSetTitle(&msConf, "select Mii not yours.");
			miiSelectorLaunch(&msConf, &msRet);

            if (need_display_slots_refresh) {
                printf("\x1b[3;12H"); // 行3列12に移動
                for (u8 _ = 0; _ < MII_HOLDER_SIZE; _++) {
                    printf("\x1b[1B\x1b[K"); // 1行下に移動、カーソルより列後ろ(右側すべて)を削除
                }
                need_display_slots_refresh = false;
            }

            printf("\x1b[15;1H\x1b[2K"); // 行15を削除 ("Writing database..."のところ)
            if (!msRet.no_mii_selected)
            {
                if (msRet.mii.system_id != system_id) {
                    temp_mii_pos.page = msRet.mii.mii_pos.page_index;
                    temp_mii_pos.slot = msRet.mii.mii_pos.slot_index+1;
                    for (u8 i = 0; i < MII_HOLDER_SIZE; i++) {
                        if (mii_slots[i].raw == 0) {
                            printf("\x1b[%u;1HSLOT [%u/%u] = position: %u\n",i+4, i+1, MII_HOLDER_SIZE, temp_mii_pos.page*10+temp_mii_pos.slot);
                            mii_slots[i] = temp_mii_pos;
                            break;
                        } else if (mii_slots[i].raw == temp_mii_pos.raw) {
                            break;
                        }
                    }
                    // printf("\x1b[15;H%llx", msRet.mii.system_id);
                }
                // else {
                //     printf("\x1b[15;1HThis Mii is already yours.");
                // }
            }
		}

        if (kDown & KEY_SELECT) {
            if (system_id != 0) {
                cfldb_open_read();
                // miis = cfldb_get_mii_array(&db);
                for(u8 i = 0; i < MII_HOLDER_SIZE; i++) {
                    if (mii_slots[i].raw != 0) {
                        mii_slots[i].slot -= 1;
                        for (u8 j = 0; j < mii_count; j++) {
                            if (miis[j].position.raw == mii_slots[i].raw) {
                                miis[j].system_id = system_id;
                                break;
                            }
                        }
                        mii_slots[i].raw = 0;
                    } else {
                        break;
                    }
                }
                printf("\x1b[15;1HWriting database...");
                cfldb_fix_checksum(&db);
                res_hang(cfldb_write(&db), "write");
                res_hang(cfldb_close(&db), "close");
                printf(" done.");
                need_blacklist_update = true;
                need_display_slots_refresh = true;
            }
        }
        gfxFlushBuffers(); // 画面の更新用
        gfxSwapBuffers(); // 画面の更新用
		gspWaitForVBlank(); // スリープからの復帰に必要っぽい
    }

    gfxExit();
    return 0;
}
