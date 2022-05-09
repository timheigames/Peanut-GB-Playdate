#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rom_list.h"

#if defined(ENABLE_SOUND_MINIGB)
#include "minigb_apu/minigb_apu.h"
#endif

#include "pd_api.h"
#include "peanut_gb.h"

PlaydateAPI *pd;
static uint8_t *FrameBuffer;
int is_rom_chosen = 0;
int startSelectDelay = 30;
SoundSource *gameSoundSource;

struct priv_t
{
    /* Pointer to allocated memory holding GB file. */
    uint8_t *rom;
    /* Pointer to allocated memory holding save file. */
    uint8_t *cart_ram;

    uint8_t fb[LCD_HEIGHT][LCD_WIDTH];
};

struct gb_s gb;
struct priv_t priv =
    {
        .rom = NULL,
        .cart_ram = NULL
    };
const double target_speed_ms = 1000.0 / VERTICAL_SYNC;
double speed_compensation = 0.0;
uint_fast32_t new_ticks, old_ticks;
enum gb_init_error_e gb_ret;
/* Must be freed */
char *rom_file_name = NULL;
char *save_file_name = NULL;

static int update(void* userdata);
void quit();
void out();

/**
 * Returns a byte from the ROM file at the given address.
 */
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_t * const p = gb->direct.priv;
	return p->rom[addr];
}

/**
 * Returns a byte from the cartridge RAM at the given address.
 */
uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct priv_t * const p = gb->direct.priv;
	return p->cart_ram[addr];
}

/**
 * Writes a given byte to the cartridge RAM at the given address.
 */
void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr,
		       const uint8_t val)
{
	const struct priv_t * const p = gb->direct.priv;
	p->cart_ram[addr] = val;
}

/**
 * Returns a pointer to the allocated space containing the ROM. Must be freed.
 */
uint8_t *read_rom_to_ram(const char *file_name)
{
	SDFile *rom_file = pd->file->open(file_name, kFileReadData);
	size_t rom_size;
	uint8_t *rom = NULL;

	if(rom_file == NULL)
		return NULL;

	pd->file->seek(rom_file, 0, SEEK_END);
	rom_size = pd->file->tell(rom_file);
    pd->file->seek(rom_file, 0, SEEK_SET);
	rom = pd->system->realloc(NULL, rom_size);

	//if(fread(rom, sizeof(uint8_t), rom_size, rom_file) != rom_size)
	if(pd->file->read(rom_file, rom, rom_size) != rom_size)
	{
		if(rom != NULL) pd->system->realloc(rom, 0);
		pd->file->close(rom_file);
		return NULL;
	}

    pd->file->close(rom_file);
	return rom;
}

void read_cart_ram_file(const char *save_file_name, uint8_t **dest,
			const size_t len)
{
	SDFile *f;

	/* If save file not required. */
	if(len == 0)
	{
		*dest = NULL;
		return;
	}

	/* Allocate enough memory to hold save file. */
	if((*dest = pd->system->realloc(NULL, len)) == NULL)
	{
        pd->system->logToConsole("Unable to allocate save file memory.");
	}

    f = pd->file->open(save_file_name, kFileReadData);
	/* It doesn't matter if the save file doesn't exist. We initialise the
	 * save memory allocated above. The save file will be created on exit. */
	if(f == NULL)
	{
		memset(*dest, 0, len);
		return;
	}

	/* Read save file to allocated memory. */
    pd->file->read(f, *dest, len);
	pd->file->close(f);
}

void write_cart_ram_file(const char *save_file_name, uint8_t **dest,
			 const size_t len)
{
	SDFile *f;

	if(len == 0 || *dest == NULL)
		return;

	if((f = pd->file->open(save_file_name, kFileWrite)) == NULL)
	{
        pd->system->logToConsole("Unable to open save file.");
	}

	/* Record save file. */
	pd->file->write(f, *dest, len);
	pd->file->close(f);
}

/**
 * Handles an error reported by the emulator. The emulator context may be used
 * to better understand why the error given in gb_err was reported.
 */
void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t val)
{
	switch(gb_err)
	{
	case GB_INVALID_OPCODE:
		/* We compensate for the post-increment in the __gb_step_cpu
		 * function. */
        pd->system->logToConsole("Invalid opcode");
		break;

	/* Ignoring non fatal errors. */
	case GB_INVALID_WRITE:
	case GB_INVALID_READ:
		return;

	default:
        pd->system->logToConsole("Unknown error");
		break;
	}

	return;
}

#if ENABLE_LCD
/**
 * Draws scanline into framebuffer.
 */
void lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160],
		   const uint_least8_t line)
{
	struct priv_t *priv = gb->direct.priv;

	for(unsigned int x = 0; x < LCD_WIDTH; x++)
	{
		priv->fb[line][x] = pixels[x] & 3;
	}
}
#endif

void saveGame() {
    if(!is_rom_chosen) return;
    write_cart_ram_file(save_file_name, &priv.cart_ram, gb_get_save_size(&gb));
}

void reset() {
    if(!is_rom_chosen) return;
    gb_reset(&gb);
}

void loadRom() {
    /* Copy input ROM file to allocated memory. */
    if((priv.rom = read_rom_to_ram(rom_file_name)) == NULL)
    {
        pd->system->logToConsole("Error loading ROM");
        out();
    }

    /* If no save file is specified, copy save file (with specific name) to
     * allocated memory. */
    if(save_file_name == NULL)
    {
        char *str_replace;
        const char extension[] = ".sav";

        /* Allocate enough space for the ROM file name, for the "sav"
         * extension and for the null terminator. */
        save_file_name = pd->system->realloc(NULL, strlen(rom_file_name) + strlen(extension) + 1);

        if(save_file_name == NULL)
        {
            pd->system->logToConsole("Error with save file name");
            out();
        }

        /* Copy the ROM file name to allocated space. */
        strcpy(save_file_name, rom_file_name);

        /* If the file name does not have a dot, or the only dot is at
         * the start of the file name, set the pointer to begin
         * replacing the string to the end of the file name, otherwise
         * set it to the dot. */
        if((str_replace = strrchr(save_file_name, '.')) == NULL ||
           str_replace == save_file_name)
            str_replace = save_file_name + strlen(save_file_name);

        /* Copy extension to string including terminating null byte. */
        for(unsigned int i = 0; i <= strlen(extension); i++)
            *(str_replace++) = extension[i];
    }

    /* Initialise emulator context. */
    gb_ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write,
                     &gb_error, &priv);

    switch(gb_ret)
    {
        case GB_INIT_NO_ERROR:
            break;

        case GB_INIT_CARTRIDGE_UNSUPPORTED:
            pd->system->logToConsole("Unsupported cartridge.");
            out();

        case GB_INIT_INVALID_CHECKSUM:
            pd->system->logToConsole("Invalid ROM: Checksum failure.");
            out();

        default:
            pd->system->logToConsole("Unknown error: %d\n", gb_ret);
            out();
    }

    /* Load Save File. */
    read_cart_ram_file(save_file_name, &priv.cart_ram, gb_get_save_size(&gb));
}

void chooseRom() {
    if((rom_file_name = rom_list_update()) == NULL) return;
    else {
        is_rom_chosen = 1;

        pd->graphics->clear(kColorBlack);

        loadRom();

    #if defined(ENABLE_SOUND_MINIGB)
            gameSoundSource = pd->sound->addSource(audio_callback_playdate, NULL, 1);

            audio_init();
    #endif

        gb_init_lcd(&gb, &lcd_draw_line);
    }
}

void loadNewRom() {
    if(!is_rom_chosen) return;
    saveGame();
    gb_reset(&gb);
    if(priv.rom != NULL) pd->system->realloc(priv.rom, 0);
    if(priv.cart_ram != NULL) pd->system->realloc(priv.cart_ram, 0);
    if(save_file_name != NULL) pd->system->realloc(save_file_name, 0);
    if(rom_file_name != NULL) pd->system->realloc(rom_file_name, 0);
    rom_file_name = NULL;
    save_file_name = NULL;
#if defined(ENABLE_SOUND_MINIGB)
    pd->sound->channel->removeSource(pd->sound->getDefaultChannel(), gameSoundSource);
#endif
    redraw_menu_screen();
    is_rom_chosen = 0;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pdapi, PDSystemEvent event, uint32_t arg)
{
    (void)arg; // arg is currently only used for event = kEventKeyPressed
    if ( event == kEventInit ) {
        pd = pdapi;
        pd->display->setRefreshRate(50.0f);

        rom_list_init();

        FrameBuffer = pd->graphics->getFrame();
        pd->graphics->clear(kColorBlack);

        pd->system->addMenuItem("save", saveGame, NULL);
        pd->system->addMenuItem("reset", reset, NULL);
        pd->system->addMenuItem("choose rom", loadNewRom, NULL);

        redraw_menu_screen();

        pd->system->setUpdateCallback(update, pd);
    }
    if ( event == kEventTerminate ) {
        quit();
    }

    return 0;
}

static int update(void* userdata)
{
    if(!is_rom_chosen) {
        chooseRom();
        return 1;
    }

    gb.direct.frame_skip = 1;

    gb.direct.joypad = 255; // Reset all joypad buttons

    startSelectDelay -= 1; // This is a timer that prevents multiple presses of start/select with crank.
    if(startSelectDelay < 0) startSelectDelay = 0;

    float crankChange = pd->system->getCrankChange();

    if (startSelectDelay == 0 && crankChange > 2.0f) {
        gb.direct.joypad_bits.start = 0;
        startSelectDelay = 30;
    }
    else {
        gb.direct.joypad_bits.start = 1;
    }

    if (startSelectDelay == 0 && crankChange < -2.0f) {
        gb.direct.joypad_bits.select = 0;
        startSelectDelay = 30;
    }
    else {
        gb.direct.joypad_bits.select = 1;
    }

    PDButtons current;
    pd->system->getButtonState(&current, NULL, NULL);

    if(current) {
        if ( current & kButtonA ) {
            gb.direct.joypad_bits.a = 0;
        }
        if ( current & kButtonB ) {
            gb.direct.joypad_bits.b = 0;
        }
        if ( current & kButtonUp ) {
            gb.direct.joypad_bits.up = 0;
        }
        if ( current & kButtonDown ) {
            gb.direct.joypad_bits.down = 0;
        }
        if ( current & kButtonLeft ) {
            gb.direct.joypad_bits.left = 0;
        }
        if ( current & kButtonRight ) {
            gb.direct.joypad_bits.right = 0;
        }
    }

    /* Execute CPU cycles until the screen has to be redrawn. */
    gb_run_frame(&gb);

    if(gb.direct.frame_skip && gb.display.frame_skip_count) {
        uint16_t xx = 40;
        uint16_t yy = -1;
        uint8_t yyRowSkip = 0;
        uint16_t fbIndex = 0;
        uint16_t fbIndex2 = 0;
        for (uint8_t y = 0; y < LCD_HEIGHT - 1; ++y) {
            if ((y << 1) % 6 == 0) {
                yyRowSkip = 1;
            }
            xx = 40;
            yy += 2 - yyRowSkip;
            fbIndex = yy * LCD_ROWSIZE + (xx >> 3);
            fbIndex2 = fbIndex + 52;
            for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
                switch (priv.fb[y][x]) {
                    case 0: // white
                        FrameBuffer[fbIndex] |= (3 << (6 - (xx & 7)));
                        FrameBuffer[fbIndex2] |= (3 << (6 - (xx & 7)));
                        break;
                    case 1: // light gray
                            if ((yy & 1) == 0) {
                                FrameBuffer[fbIndex] |= (1 << (7 - (xx & 7)));
                                FrameBuffer[fbIndex] &= ~(1 << (7 - ((xx+1) & 7)));
                                FrameBuffer[fbIndex2] |= (3 << (6 - (xx & 7)));
                            }
                            else {
                                FrameBuffer[fbIndex] |= (3 << (6 - (xx & 7)));
                                FrameBuffer[fbIndex2] |= (1 << (7 - (xx & 7)));
                                FrameBuffer[fbIndex2] &= ~(1 << (7 - ((xx+1) & 7)));
                            }
                        break;
                    case 2: // dark gray
                            if ((yy & 1) == 0) {
                                FrameBuffer[fbIndex] |= (1 << (7 - (xx & 7)));
                                FrameBuffer[fbIndex] &= ~(1 << (7 - ((xx+1) & 7)));
                                FrameBuffer[fbIndex2] &= ~(1 << (7 - (xx & 7)));
                                FrameBuffer[fbIndex2] |= (1 << (7 - ((xx+1) & 7)));

                            }
                            else {
                                FrameBuffer[fbIndex] &= ~(1 << (7 - (xx & 7)));
                                FrameBuffer[fbIndex] |= (1 << (7 - ((xx+1) & 7)));
                                FrameBuffer[fbIndex2] |= (1 << (7 - (xx & 7)));
                                FrameBuffer[fbIndex2] &= ~(1 << (7 - ((xx+1) & 7)));
                            }
                        break;
                    case 3: // black
                        FrameBuffer[fbIndex] &= ~(3 << (6 - (xx & 7)));
                        FrameBuffer[fbIndex2] &= ~(3 << (6 - (xx & 7)));
                        break;
                }

                xx += 2;
                if((xx & 7) == 0) { fbIndex += 1; fbIndex2 += 1; };
            }
            yyRowSkip = 0;
        }

        pd->graphics->markUpdatedRows(0, LCD_ROWS - 1);
    }

    pd->system->drawFPS(0,0);
    return 1;
}

void quit() {
    /* Record save file. */
    saveGame();
    out();
}

void out() {
    if(priv.rom != NULL) pd->system->realloc(priv.rom, 0);
    if(priv.cart_ram != NULL) pd->system->realloc(priv.cart_ram, 0);
    if(save_file_name != NULL) pd->system->realloc(save_file_name, 0);
    if(rom_file_name != NULL) pd->system->realloc(rom_file_name, 0);

    rom_list_cleanup();
}
