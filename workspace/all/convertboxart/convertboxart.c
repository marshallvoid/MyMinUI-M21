#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h>
#include <string.h>
#if defined(USE_SDL2)
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL2_rotozoom.h"
#else
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "SDL_rotozoom.h"
#endif

#include <msettings.h>

#include "defines.h"
#include "utils.h"
#include "api.h"

typedef struct Array {
	int count;
	int capacity;
	void** items;
} Array;

static Array* Array_new(void) {
	Array* self = malloc(sizeof(Array));
	self->count = 0;
	self->capacity = 8;
	self->items = malloc(sizeof(void*) * self->capacity);
	return self;
}

static void Array_push(Array* self, void* item) {
	if (self->count >= self->capacity) {
		self->capacity *= 2;
		self->items = realloc(self->items, sizeof(void*) * self->capacity);
	}
	self->items[self->count++] = item;
}

static int StringArray_indexOf(Array* self, char* str) {
	for (int i = 0; i < self->count; i++) {
		if (exactMatch((char*)self->items[i], str)) return i;
	}
	return -1;
}

static void StringArray_free(Array* self) {
	for (int i = 0; i < self->count; i++) {
		free(self->items[i]);
	}
	free(self->items);
	free(self);
}

static void Array_free(Array* self) {
	free(self->items);
	free(self);
}

typedef struct BoxartTask {
	char src[512];
	char dst[512];
	char display_name[256];
} BoxartTask;

int makeBoxart(char *, char *, myBoxartData);

static int isImageExt(const char *name) {
	return suffixMatch(".png", (char*)name) || suffixMatch(".jpg", (char*)name) || suffixMatch(".jpeg", (char*)name);
}

static int taskExists(Array *tasks, const char *dst) {
	for (int i = 0; i < tasks->count; i++) {
		BoxartTask *t = (BoxartTask*)tasks->items[i];
		if (exactMatch(t->dst, (char*)dst)) return 1;
	}
	return 0;
}

static void addImgDirIfExist(const char *path, Array *img_dirs) {
	struct stat st;
	if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
		if (StringArray_indexOf(img_dirs, (char*)path) == -1) {
			Array_push(img_dirs, strdup(path));
		}
	}
}

static void scanForImgDirs(const char *path, Array *img_dirs) {
	DIR *dh = opendir(path);
	if (!dh) return;
	struct dirent *dp;
	char full_path[512];
	while ((dp = readdir(dh)) != NULL) {
		if (dp->d_name[0] == '.') continue;
		snprintf(full_path, sizeof(full_path), "%s/%s", path, dp->d_name);
		struct stat st;
		if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
			if (strcasecmp(dp->d_name, "Imgs") == 0 || strcasecmp(dp->d_name, "srcimgdir") == 0) {
				if (StringArray_indexOf(img_dirs, full_path) == -1) {
					Array_push(img_dirs, strdup(full_path));
				}
			} else {
				scanForImgDirs(full_path, img_dirs);
			}
		}
	}
	closedir(dh);
}

static void collectTasksInDir(const char *dir_path, Array *tasks) {
	DIR *dh = opendir(dir_path);
	if (!dh) return;
	struct dirent *dp;
	char full_src[512];
	char full_dst[512];
	char bak_path[512];

	while ((dp = readdir(dh)) != NULL) {
		if (dp->d_name[0] == '.') continue;

		// 1) If file ends with .bak (e.g. image.png.bak)
		if (suffixMatch(".bak", dp->d_name)) {
			snprintf(full_src, sizeof(full_src), "%s/%s", dir_path, dp->d_name);
			snprintf(full_dst, sizeof(full_dst), "%s/%s", dir_path, dp->d_name);
			char *bak_ext = strrchr(full_dst, '.');
			if (bak_ext && exactMatch(bak_ext, ".bak")) {
				*bak_ext = '\0'; // strip .bak to get original target filename
			}

			if (!taskExists(tasks, full_dst)) {
				BoxartTask *task = malloc(sizeof(BoxartTask));
				strncpy(task->src, full_src, sizeof(task->src));
				strncpy(task->dst, full_dst, sizeof(task->dst));
				snprintf(task->display_name, sizeof(task->display_name), "%s", strrchr(full_dst, '/') + 1);
				Array_push(tasks, task);
			}
		}
		// 2) If file is .png, .jpg, .jpeg
		else if (isImageExt(dp->d_name)) {
			snprintf(bak_path, sizeof(bak_path), "%s/%s.bak", dir_path, dp->d_name);
			snprintf(full_dst, sizeof(full_dst), "%s/%s", dir_path, dp->d_name);

			if (taskExists(tasks, full_dst)) continue;

			// If .bak already exists on disk, Case 1 handles it
			if (access(bak_path, F_OK) == 0) continue;

			// Rename original image to .bak
			if (rename(full_dst, bak_path) == 0) {
				BoxartTask *task = malloc(sizeof(BoxartTask));
				strncpy(task->src, bak_path, sizeof(task->src));
				strncpy(task->dst, full_dst, sizeof(task->dst));
				snprintf(task->display_name, sizeof(task->display_name), "%s", dp->d_name);
				Array_push(tasks, task);
			}
		}
	}
	closedir(dh);
}

int main(int argc, char* argv[]) {
    PWR_setCPUSpeed(CPU_SPEED_PERFORMANCE);

    SDL_Surface* screen = GFX_init(MODE_MAIN);
    PWR_init();
    InitSettings();
    PAD_init();

    myBoxartData boxartdata = {
        .bX = 256,
        .bY = 0,
        .bW = 384,
        .bH = 480,
        .sW = 640,
        .sH = 480,
        .aspect = 0,
        .gradient = "NONE"
    };
    readBoxartcfg(TOOLBOXART_CFGFILE, &boxartdata);

    Array *img_dirs = Array_new();
    addImgDirIfExist(SDCARD_PATH "/Imgs", img_dirs);
    scanForImgDirs(SDCARD_PATH "/Roms", img_dirs);
    scanForImgDirs(SDCARD_PATH "/Tools", img_dirs);
    addImgDirIfExist(SDCARD_PATH "/Tools/" THISPLATFORM "/Convert BoxArt.pak/srcimgdir", img_dirs);

    Array *tasks = Array_new();
    for (int i = 0; i < img_dirs->count; i++) {
        collectTasksInDir(img_dirs->items[i], tasks);
    }

    int total = tasks->count;
    char progressstr[512];

    if (total == 0) {
        GFX_clear(screen);
        GFX_blitMessage(font.large, "No images found to convert\nPress A to Exit", screen, &(SDL_Rect){0, 0, screen->w, screen->h});
        GFX_flip(screen);
    } else {
        for (int i = 0; i < total; i++) {
            BoxartTask *task = (BoxartTask*)tasks->items[i];

            makeBoxart(task->src, task->dst, boxartdata);

            snprintf(progressstr, sizeof(progressstr), "Converted %d/%d\n%s", i + 1, total, task->display_name);

            GFX_clear(screen);
            GFX_blitMessage(font.large, progressstr, screen, &(SDL_Rect){0, 0, screen->w, screen->h});
            GFX_flip(screen);

            PAD_poll();
        }

        GFX_clear(screen);
        snprintf(progressstr, sizeof(progressstr), "Converted %d images!\nPress A to Exit", total);
        GFX_blitMessage(font.large, progressstr, screen, &(SDL_Rect){0, 0, screen->w, screen->h});
        GFX_flip(screen);
    }

    int quit = 0;
    while (!quit) {
        PAD_poll();
        if (PAD_justPressed(BTN_A)) {
            quit = 1;
        } else {
            GFX_sync();
        }
    }

    for (int i = 0; i < tasks->count; i++) {
        free(tasks->items[i]);
    }
    Array_free(tasks);
    StringArray_free(img_dirs);

    PWR_setCPUSpeed(CPU_SPEED_MENU);
    QuitSettings();
    PWR_quit();
    GFX_quit();
    PAD_quit();
    return EXIT_SUCCESS;
}

int makeBoxart(char *infilename, char *outfilename, myBoxartData mydata) {
    SDL_Surface *image = IMG_Load(infilename);
    SDL_Surface *mysurface = SDL_CreateRGBSurface(0, mydata.sW , mydata.sH ,16,0,0,0,0);
    //SDL_Surface *unscaled_myimg = IMG_Load(BACKGROUND);
    if (image == NULL){
        LOG_info("error loading the image %s", infilename);
        return -1;
    }
    double xfactr, yfactr;
    double myaspect = 1.0 * image->w / image->h;
    int localX = mydata.bX;
    int localY = mydata.bY;
    switch (mydata.aspect) {
        case ASPECT:
            LOG_info("ASPECT = ASPECT\n");
            //first calculate othe original aspectratio
            if (myaspect > 1.0){
                //the W is bigger than H
                    //resize to fit W
                    xfactr = 1.0 * mydata.bW / image->w;
                    yfactr = xfactr;
                    localY += (mydata.bH - (image->h * yfactr))/2;
            } else { //image is higher than wider
                    //resize to fit H
                    yfactr = 1.0 * mydata.bH / image->h;
                    xfactr = yfactr;
                    localX += (mydata.bW - (image->w * xfactr))/2;
            }
            break;
        case NATIVE:
            LOG_info("ASPECT = NATIVE\n");
            if ((mydata.bW > image->w) && (mydata.bH > image->h)){
                //image is smaller than target box
                xfactr = 1.0;
                yfactr = 1.0;
                //change x y to place the image in the center of the target box
                localX += (mydata.bW - image->w)/2;
                localY += (mydata.bH - image->h)/2;
            } else {
                //image is bigger than target box so apply ASPECT rule
                if (myaspect > 1.0){
                    //the W is bigger than H
                        //resize to fit W
                        xfactr = 1.0 * mydata.bW / image->w;
                        yfactr = xfactr;
                        localY += (mydata.bH - (image->h * yfactr))/2;
                } else { //image is higher than wider
                        //resize to fit H
                        yfactr = 1.0 * mydata.bH / image->h;
                        xfactr = yfactr;
                        localX += (mydata.bW - (image->w * xfactr))/2;
                }
            }
            break;
        case FULL:
            //fill target box by shrinking/streching the original image
            LOG_info("ASPECT = FULL\n");
            xfactr = 1.0 * mydata.bW / image->w;
            yfactr = 1.0 * mydata.bH / image->h;
            break;
    }
    SDL_Surface *scaled_myimg = zoomSurface(image, xfactr , yfactr, 0);
    SDL_BlitSurface(scaled_myimg,NULL,mysurface,&(SDL_Rect){localX,localY});
    //char mygradient[256];
    //sprintf(mygradient,"%s",mydata.gradient);

    if (strncmp(mydata.gradient,"NONE",4) != 0){
        SDL_Surface *blackgradient = IMG_Load(mydata.gradient);
        if (blackgradient == NULL){
            LOG_info("Failed loading Gradient: %s\n", IMG_GetError());
            return -1;
        }
#if defined (USE_SDL2)
		SDL_SetSurfaceBlendMode(blackgradient,SDL_BLENDMODE_BLEND);
#else
		SDL_SetColorKey(blackgradient, SDL_TRUE, SDL_MapRGB(blackgradient->format, 0, 0, 0));
#endif
        SDL_BlitSurface(blackgradient,NULL,mysurface,NULL);
        SDL_FreeSurface(blackgradient);
    }

    SDL_RWops* out = SDL_RWFromFile(outfilename, "wb");
#if defined (USE_SDL2)
    IMG_SavePNG_RW(mysurface, out, 1);
#else
    SDL_SaveBMP_RW(mysurface, out, 1);
    bmp2png(outfilename);
#endif
    SDL_FreeSurface(scaled_myimg);
    SDL_FreeSurface(mysurface);
    SDL_FreeSurface(image);
    return 1;
}
