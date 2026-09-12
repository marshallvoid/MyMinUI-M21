#include <stdio.h>
#include <stdlib.h>
#include <msettings.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#include "defines.h"
#include "api.h"
#include "utils.h"

#if defined(USE_SDL2)
#include "SDL2_rotozoom.h"
#else
#include "SDL_rotozoom.h"
#endif
///////////////////////////////////////

typedef struct Array {
	int count;
	int capacity;
	void** items;
} Array;

static int selected_modifier;

static Array* Array_new(void) {
	Array* self = malloc(sizeof(Array));
	self->count = 0;
	self->capacity = 8;
	self->items = malloc(sizeof(void*) * self->capacity);
	return self;
}
static void Array_push(Array* self, void* item) {
	if (self->count>=self->capacity) {
		self->capacity *= 2;
		self->items = realloc(self->items, sizeof(void*) * self->capacity);
	}
	self->items[self->count++] = item;
}
static void Array_unshift(Array* self, void* item) {
	if (self->count==0) return Array_push(self, item);
	Array_push(self, NULL); // ensures we have enough capacity
	for (int i=self->count-2; i>=0; i--) {
		self->items[i+1] = self->items[i];
	}
	self->items[0] = item;
}
static void* Array_pop(Array* self) {
	if (self->count==0) return NULL;
	return self->items[--self->count];
}
static void Array_reverse(Array* self) {
	int end = self->count-1;
	int mid = self->count/2;
	for (int i=0; i<mid; i++) {
		void* item = self->items[i];
		self->items[i] = self->items[end-i];
		self->items[end-i] = item;
	}
}
static void Array_free(Array* self) {
	free(self->items);
	free(self);
}

static int StringArray_indexOf(Array* self, char* str) {
	for (int i=0; i<self->count; i++) {
		if (exactMatch(self->items[i], str)) return i;
	}
	return -1;
}
static void StringArray_free(Array* self) {
	for (int i=0; i<self->count; i++) {
		free(self->items[i]);
	}
	Array_free(self);
}

///////////////////////////////////////

typedef struct Hash {
	Array* keys;
	Array* values;
} Hash; // not really a hash

static Hash* Hash_new(void) {
	Hash* self = malloc(sizeof(Hash));
	self->keys = Array_new();
	self->values = Array_new();
	return self;
}
static void Hash_free(Hash* self) {
	StringArray_free(self->keys);
	StringArray_free(self->values);
	free(self);
}
static void Hash_set(Hash* self, char* key, char* value) {
	Array_push(self->keys, strdup(key));
	Array_push(self->values, strdup(value));
}
static char* Hash_get(Hash* self, char* key) {
	int i = StringArray_indexOf(self->keys, key);
	if (i==-1) return NULL;
	return self->values->items[i];
}

///////////////////////////////////////

enum EntryType {
	ENTRY_DIR,
	ENTRY_PAK,
	ENTRY_ROM,
};
typedef struct Entry {
	char* path;
	char* name;
	char* fullname;
	char* unique;
	int type;
	int alpha; // index in parent Directory's alphas Array, which points to the index of an Entry in its entries Array :sweat_smile:
} Entry;

static int hasEmu(char* emu_name);

static Entry* Entry_new(char* path, int type) {
	char display_name[256];
	getDisplayName(path, display_name);
	Entry* self = malloc(sizeof(Entry));
	self->path = strdup(path);
	self->name = strdup(display_name);
	self->unique = NULL;
	self->type = type;
	self->alpha = 0;
	return self;
}
static void Entry_free(Entry* self) {
	free(self->path);
	free(self->name);
	if (self->unique) free(self->unique);
	free(self);
}

static int EntryArray_indexOf(Array* self, char* path) {
	for (int i=0; i<self->count; i++) {
		Entry* entry = self->items[i];
		if (exactMatch(entry->path, path)) return i;
	}
	return -1;
}
static int EntryArray_sortEntry(const void* a, const void* b) {
	Entry* item1 = *(Entry**)a;
	Entry* item2 = *(Entry**)b;
	return strcasecmp(item1->name, item2->name);
}
static void EntryArray_sort(Array* self) {
	qsort(self->items, self->count, sizeof(void*), EntryArray_sortEntry);
}

static void EntryArray_free(Array* self) {
	for (int i=0; i<self->count; i++) {
		Entry_free(self->items[i]);
	}
	Array_free(self);
}

///////////////////////////////////////

#define INT_ARRAY_MAX 27
typedef struct IntArray {
	int count;
	int items[INT_ARRAY_MAX];
} IntArray;
static IntArray* IntArray_new(void) {
	IntArray* self = malloc(sizeof(IntArray));
	self->count = 0;
	memset(self->items, 0, sizeof(int) * INT_ARRAY_MAX);
	return self;
}
static void IntArray_push(IntArray* self, int i) {
	self->items[self->count++] = i;
}
static void IntArray_free(IntArray* self) {
	free(self);
}

///////////////////////////////////////

typedef struct Directory {
	char* path;
	char* name;
	Array* entries;
	IntArray* alphas;
	// rendering
	int selected;
	int start;
	int end;
} Directory;

static int getIndexChar(char* str) {
	char i = 0;
	char c = tolower(str[0]);
	if (c>='a' && c<='z') i = (c-'a')+1;
	return i;
}

static void getUniqueName(Entry* entry, char* out_name) {
	char* filename = strrchr(entry->path, '/')+1;
	char emu_tag[256];
	getEmuName(entry->path, emu_tag);

	char *tmp;
	strcpy(out_name, entry->name);
	tmp = out_name + strlen(out_name);
	strcpy(tmp, " (");
	tmp = out_name + strlen(out_name);
	strcpy(tmp, emu_tag);
	tmp = out_name + strlen(out_name);
	strcpy(tmp, ")");
}

static void Directory_index(Directory* self) {
	int skip_index = exactMatch(FAUX_HIDDEN_PATH, self->path) || exactMatch(FAUX_RECENT_PATH, self->path) || exactMatch(FAUX_FAVORITE_PATH, self->path) || prefixMatch(COLLECTIONS_PATH, self->path); // not alphabetized

	Hash* map = NULL;
	char map_path[256];
	sprintf(map_path, "%s/map.txt", self->path);
	if (exists(map_path)) {
		FILE* file = fopen(map_path, "r");
		if (file) {
			map = Hash_new();
			char line[256];
			int resort = 0;
			while (fgets(line,256,file)!=NULL) {
				normalizeNewline(line);
				trimTrailingNewlines(line);
				if (strlen(line)==0) continue; // skip empty lines

				char* tmp = strchr(line,'\t');
				if (tmp) {
					tmp[0] = '\0';
					char* key = line;
					char* value = tmp+1;
					Hash_set(map, key, value);
				}
			}
			fclose(file);

			// need to
			for (int i=0; i<self->entries->count; i++) {
				Entry* entry = self->entries->items[i];
				char* filename = strrchr(entry->path, '/')+1;
				char* alias = Hash_get(map, filename);
				if (alias) {
					free(entry->name);
					entry->name = strdup(alias);
					resort = 1;
				}
			}

			// oof, double s

			// TODO: maybe map.txt logic should be moved to EntryArray_sort()
			// or another precursor?

			if (resort) EntryArray_sort(self->entries);
		}
	}

	Entry* prior = NULL;
	int alpha = -1;
	int index = 0;
	for (int i=0; i<self->entries->count; i++) {
		Entry* entry = self->entries->items[i];
		if (map) {
			char* filename = strrchr(entry->path, '/')+1;
			char* alias = Hash_get(map, filename);
			if (alias) {
				free(entry->name);
				entry->name = strdup(alias);
			}
		}

		if (prior!=NULL && exactMatch(prior->name, entry->name)) {
			if (prior->unique) free(prior->unique);
			if (entry->unique) free(entry->unique);

			char* prior_filename = strrchr(prior->path, '/')+1;
			char* entry_filename = strrchr(entry->path, '/')+1;
			if (exactMatch(prior_filename, entry_filename)) {
				char prior_unique[256];
				char entry_unique[256];
				getUniqueName(prior, prior_unique);
				getUniqueName(entry, entry_unique);

				prior->unique = strdup(prior_unique);
				entry->unique = strdup(entry_unique);
			}
			else {
				prior->unique = strdup(prior_filename);
				entry->unique = strdup(entry_filename);
			}
		}

		if (!skip_index) {
			int a = getIndexChar(entry->name);
			if (a!=alpha) {
				index = self->alphas->count;
				IntArray_push(self->alphas, i);
				alpha = a;
			}
			entry->alpha = index;
		}

		prior = entry;
	}

	if (map) Hash_free(map);
}


///////////////////////////////////////
// fuzzy search across all rom folders (Y options popup + full 640px keyboard)

static Array* search_results = NULL; // SearchResultArray
static char search_query[256] = "";
static int show_search = 0; // 0=off, 1=options menu with search results
static int show_search_menu = 0; // 0=options list on top, 1=search results on top
static int search_selected = 0;
static int search_start = 0;
static int search_option = 0; // options list selection (currently only Search)
static int tools_start = 0; // visible window for menu 0 (own var, menu 2 reuses search_start)
// keyboard state
static char kb_text[256] = "";
static int kb_col = 0;
static int kb_row = 0;
static int kb_upper = 0;

typedef struct SearchResult {
	char* path; // full SDCARD path of the rom
	char* name; // display name (sorted, searchable)
	char* emu;  // system folder name shown as subtitle
	int type;
	int score;
} SearchResult;

static SearchResult* SearchResult_new(char* path, char* name, char* emu, int type, int score) {
	SearchResult* self = malloc(sizeof(SearchResult));
	self->path = strdup(path);
	self->name = strdup(name);
	self->emu = strdup(emu);
	self->type = type;
	self->score = score;
	return self;
}
static void SearchResult_free(SearchResult* self) {
	free(self->path);
	free(self->name);
	free(self->emu);
	free(self);
}
static int SearchResultArray_sortResult(const void* a, const void* b) {
	SearchResult* item1 = *(SearchResult**)a;
	SearchResult* item2 = *(SearchResult**)b;
	if (item1->score!=item2->score) return item1->score-item2->score;
	return strcasecmp(item1->name, item2->name);
}
static void SearchResultArray_free(Array* self) {
	for (int i=0; i<self->count; i++) {
		SearchResult_free(self->items[i]);
	}
	Array_free(self);
}

// fuzzy subsequence match, lower score = better match
// -1 means no match, otherwise counts skipped chars (0 for substring/prefix)
static int fuzzyScore(char* needle, char* haystack) {
	if (!needle[0]) return 0;
	int n = strlen(needle);
	int h = strlen(haystack);
	int i = 0, j = 0, skipped = 0, first = -1;
	while (i<n && j<h) {
		if (tolower((unsigned char)needle[i])==tolower((unsigned char)haystack[j])) {
			if (first<0) first = j;
			i++;
		}
		else skipped++;
		j++;
	}
	if (i<n) return -1; // not all chars matched in order
	if (first>0) skipped += first; // prefer prefix matches
	return skipped;
}

// (isMetadataFile / boxart helpers are defined further below in this single
// .c file; forward-declare them HERE (before first use) so the compiler
// doesn't create implicit-int declarations -> conflicting types)
int isMetadataFile(char* name);
static int getBoxartPath(Entry* entry, char* out);
static SDL_Surface* BoxartCache_get(char* path);
static SDL_Surface* loadBoxart(const char* path);
static void BoxartCache_put(char* path, SDL_Surface* surface);
static void MenuOpt_boxartPath(int i, char* out);
static void MenuOpt_preloadAt(int i);

static void SearchResults_clear(void) {
	if (search_results) SearchResultArray_free(search_results);
	search_results = NULL;
}

// ---- Y/Options menu model ----
// Layout: Search first, then *.pak folders from Tools/PLATFORM sorted A-Z,
// then Shutdown always last. No Back item (B already goes back).
// All menu-0 code goes through MenuOpt_at() so adding a new virtual option
// only touches one place instead of scattered i==0 / i==topts-1 checks.
typedef enum { OPT_SEARCH, OPT_PAK, OPT_SHUTDOWN } OptKind;
static Array* tools_entries = NULL;
static int MenuOpt_count(void) {
	return 2 + (tools_entries ? tools_entries->count : 0);
}
// Resolve item i -> kind + display name (+ pak entry for OPT_PAK).
// Returns 1 on success, 0 if i is out of range.
static int MenuOpt_at(int i, OptKind* kind, char* name, Entry** pak) {
	int t = tools_entries ? tools_entries->count : 0;
	if (i<0 || i>=t+2) return 0;
	if (i==0) {
		if (kind) *kind = OPT_SEARCH;
		if (name) strcpy(name, "Search");
		if (pak) *pak = NULL;
		return 1;
	}
	if (i==t+1) {
		if (kind) *kind = OPT_SHUTDOWN;
		if (name) strcpy(name, "Shutdown");
		if (pak) *pak = NULL;
		return 1;
	}
	Entry* tool = (tools_entries && i-1>=0 && i-1<t) ? tools_entries->items[i-1] : NULL;
	if (!tool) return 0;
	if (kind) *kind = OPT_PAK;
	if (name) strcpy(name, tool->name);
	if (pak) *pak = tool;
	return 1;
}
static void Tools_clear(void) {
	if (tools_entries) EntryArray_free(tools_entries);
	tools_entries = NULL;
}
static int Tools_count(void) {
	return tools_entries ? tools_entries->count : 0;
}
static void Tools_refresh(void) {
	Tools_clear();
	tools_entries = Array_new();
	char tools_path[256];
	sprintf(tools_path, "%s/Tools/%s", SDCARD_PATH, PLATFORM);
	DIR* dh = opendir(tools_path);
	if (dh==NULL) return;
	struct dirent* dp;
	char full_path[512];
	while ((dp = readdir(dh))!=NULL) {
		if (hide(dp->d_name)) continue;
		if (dp->d_type!=DT_DIR) continue;
		if (!suffixMatch(".pak", dp->d_name)) continue;
		sprintf(full_path, "%s/%s", tools_path, dp->d_name);
		char launch[512];
		sprintf(launch, "%s/launch.sh", full_path);
		if (!exists(launch)) continue;
		Array_push(tools_entries, Entry_new(full_path, ENTRY_PAK));
	}
	closedir(dh);
	EntryArray_sort(tools_entries);
}
// Resolve the display boxart for Y/Options item i into out (empty if none).
// Virtual items (Search/Shutdown/...): /Imgs/<name>.png, then default.png.
// Pak items: via getBoxartPath().
static void MenuOpt_boxartPath(int i, char* out) {
	out[0] = '\0';
	OptKind kind;
	char name[256];
	Entry* tool = NULL;
	if (!MenuOpt_at(i, &kind, name, &tool)) return;
	if (kind==OPT_PAK && tool) {
		getBoxartPath(tool, out);
		return;
	}
	sprintf(out, SDCARD_PATH "/Imgs/%s.png", name);
	if (!exists(out)) {
		sprintf(out, SDCARD_PATH "/Imgs/default.png");
		if (!exists(out)) out[0] = '\0';
	}
}
static void MenuOpt_preloadAt(int i) { // decode + cache one Y/Options art if needed
	char path[512];
	MenuOpt_boxartPath(i, path);
	if (!path[0]) return;
	if (BoxartCache_get(path)) return;
	SDL_Surface* surface = loadBoxart(path);
	if (surface) BoxartCache_put(path, surface);
}

static void SearchResults_build(char* query) {
	SearchResults_clear();
	if (!query || !query[0]) return;
	search_results = Array_new();

	DIR* dh = opendir(ROMS_PATH);
	if (dh==NULL) return;
	struct dirent* dp;
	char dir_path[256];
	char full_path[512];
	while ((dp = readdir(dh))!=NULL) {
		if (hide(dp->d_name)) continue;
		if (dp->d_type!=DT_DIR) continue;
		sprintf(dir_path, "%s/%s", ROMS_PATH, dp->d_name);

		DIR* rh = opendir(dir_path);
		if (rh==NULL) continue;
		struct dirent* rp;
		while ((rp = readdir(rh))!=NULL) {
			if (hide(rp->d_name)) continue;
			if (isMetadataFile(rp->d_name)) continue;
			int is_dir = (rp->d_type==DT_DIR);
			if (is_dir && !suffixMatch(".pak", rp->d_name)) {
				// Folder game (multi-disc PS1 etc.): only keep it if it
				// actually contains a cue/m3u, otherwise it's just a subfolder.
				sprintf(full_path, "%s/%s", dir_path, rp->d_name);
				char cue_path[512];
				char m3u_path[512];
				char folder_name[256];
				strcpy(folder_name, rp->d_name);
				sprintf(cue_path, "%s/%s.cue", full_path, folder_name);
				sprintf(m3u_path, "%s/%s.m3u", full_path, folder_name);
				if (!exists(cue_path) && !exists(m3u_path)) continue;
			}
			else if (!is_dir && suffixMatch(".pak", rp->d_name)) {
				continue; // stray file, not a game
			}
			sprintf(full_path, "%s/%s", dir_path, rp->d_name);

			char display[256];
			getDisplayName(full_path, display);
			int score = fuzzyScore(query, display);
			if (score<0) continue;
			if (search_results->count>=256) continue; // cap results
			int rtype = suffixMatch(".pak", rp->d_name) ? ENTRY_PAK : ENTRY_ROM;
			Array_push(search_results, SearchResult_new(full_path, display, dp->d_name, rtype, score));
		}
		closedir(rh);
	}
	closedir(dh);

	qsort(search_results->items, search_results->count, sizeof(void*), SearchResultArray_sortResult);
}

static void Search_open(void) {
	show_search = 1;
	show_search_menu = 0;
	search_selected = 0;
	search_start = 0;
	tools_start = 0;
	search_option = 0; // 0=Search, 1..T=tools pak
	kb_text[0] = '\0';
	kb_col = 0;
	kb_row = 0;
	kb_upper = 0;
	SearchResults_clear();
	Tools_refresh(); // Search first, then Tools/*.pak sorted A-Z
}
static void Search_close(void) {
	show_search = 0;
	show_search_menu = 0;
	SearchResults_clear();
	Tools_clear();
}

// full-width 640px on-screen keyboard: 3x10 letters/digits + SPACE/DEL/ABC row
static void Search_drawKeyboard(SDL_Surface* screen, int sx, int qy, int sw) {
	static const char* kb_rows_lower[3] = {"qwertyuiop","asdfghjkl_","zxcvbnm123"};
	static const char* kb_rows_upper[3] = {"QWERTYUIOP","ASDFGHJKL_","ZXCVBNM123"};
	char* actions[3] = {"SPACE","DEL", kb_upper ? "abc" : "ABC"};
	int key_w = sw/10;
	int key_h = SCALE1(PILL_SIZE);
	int ky = qy + SCALE1(PILL_SIZE);
	for (int r=0; r<3; r++) {
		const char* row = kb_upper ? kb_rows_upper[r] : kb_rows_lower[r];
		for (int c=0; c<10; c++) {
			char key[2] = {row[c], '\0'};
			int cur = (show_search_menu==1 && r==kb_row && c==kb_col);
			SDL_Color tc = cur ? COLOR_BLACK : COLOR_WHITE;
			if (cur) {
				GFX_blitPill(ASSET_WHITE_PILL, screen, &(SDL_Rect){sx+c*key_w,ky+r*key_h,key_w,key_h});
			}
			SDL_Surface* text = TTF_RenderUTF8_Blended(font.large, key, tc);
			SDL_BlitSurface(text, NULL, screen, &(SDL_Rect){sx+c*key_w+(key_w-text->w)/2,ky+r*key_h+(key_h-text->h)/2});
			SDL_FreeSurface(text);
		}
	}
	int ay = ky + 3*key_h;
	for (int c=0; c<3; c++) {
		int aw = sw/3;
		int cur = (show_search_menu==1 && kb_row==3 && c==kb_col);
		SDL_Color tc = cur ? COLOR_BLACK : COLOR_WHITE;
		if (cur) {
			GFX_blitPill(ASSET_WHITE_PILL, screen, &(SDL_Rect){sx+c*aw,ay,aw,key_h});
		}
		SDL_Surface* text = TTF_RenderUTF8_Blended(font.large, actions[c], tc);
		SDL_BlitSurface(text, NULL, screen, &(SDL_Rect){sx+c*aw+(aw-text->w)/2,ay+(key_h-text->h)/2});
		SDL_FreeSurface(text);
	}
	// small live hint of match count
	int rtotal = search_results ? search_results->count : 0;
	char hint[64];
	sprintf(hint, "%d hit%s - START: results", rtotal, rtotal==1?"":"s");
	SDL_Surface* htext = TTF_RenderUTF8_Blended(font.small, hint, COLOR_GRAY);
	SDL_BlitSurface(htext, NULL, screen, &(SDL_Rect){sx,ay+key_h+SCALE1(4)});
	SDL_FreeSurface(htext);
}

// search UI: options popup (menu 0), full 640px keyboard (menu 1),
// paged result list (menu 2). drawn instead of the normal list.
// NOTE: Search_draw is called with the real fancy_mode from main(),
// the global fancy_mode is (re)declared later in this file.
static void Search_draw(SDL_Surface* screen, int show_setting, int fancy) {
	int rows = MAIN_ROW_COUNT + fancy;
	int sw = screen->w - SCALE1(PADDING*2);
	int sx = SCALE1(PADDING);

	if (show_search_menu==0) { // Y/Options menu: Search, Tools/*.pak A-Z, Shutdown (no Back)
		// Same geometry as search results (menu 2): a one-row GOLD header
		// on top, list starts at ry = qy + PILL_SIZE and shows rows-1
		// items so both screens share padding/centering.
		char oline[] = "Options";
		char odisp[256];
		GFX_truncateText(font.large, oline, odisp, sw, SCALE1(BUTTON_PADDING*2));
		SDL_Surface* otext = TTF_RenderUTF8_Blended(font.large, odisp, COLOR_GOLD);
		int qy = SCALE1(PADDING);
		SDL_BlitSurface(otext, &(SDL_Rect){0,0,sw,otext->h}, screen, &(SDL_Rect){sx,qy});
		SDL_FreeSurface(otext);
		int ry = qy + SCALE1(PILL_SIZE);
		// Same typography as the main rom list:
		// normal rows use font.medium + gray in fancy mode, only the
		// selected row grows to font.large + white + full width.
		int topts = MenuOpt_count();
		int orows = rows-1;
		if (orows<1) orows = 1;
		// Clamp selection in case Tools changed while open.
		// NOTE: menu 0 uses its own tools_start window (not search_start,
		// which belongs to menu 2 results) so going back and forth never
		// shows a stale page.
		if (search_option<0) search_option = 0;
		if (search_option>=topts) search_option = topts-1;
		if (search_option<tools_start) tools_start = search_option;
		if (search_option>=tools_start+orows) tools_start = search_option-orows+1;
		if (tools_start<0) tools_start = 0;
		if (tools_start>search_option) tools_start = search_option;
		// Fancy mode: paint the selected option's boxart behind the list,
		// same as search results (menu 0 has no background otherwise).
		// NOTE: cache-only here -- decoding happens in boxartPreload()
		// so D-pad never stalls on IMG_Load.
		if (fancy && search_option>=0 && search_option<topts) {
			char sel_boxart[512];
			MenuOpt_boxartPath(search_option, sel_boxart);
			if (sel_boxart[0]) {
				SDL_Surface* boxart = BoxartCache_get(sel_boxart);
				if (boxart) {
					SDL_BlitSurface(boxart, NULL, screen, &(SDL_Rect){0,0});
				}
			}
		}
		int odraw = orows;
		if (tools_start+odraw>topts) odraw = topts-tools_start;
		// Same geometry as search results: list always starts at
		// ry = qy + PILL_SIZE (no vertical centering), so both screens
		// share padding exactly.
		int row_h = SCALE1(PILL_SIZE-(5*fancy));
		for (int i=tools_start,j=0; i<tools_start+odraw; i++,j++) {
			char opt_name[256];
			OptKind opt_kind;
			Entry* opt_tool = NULL;
			if (!MenuOpt_at(i, &opt_kind, opt_name, &opt_tool)) continue;
			int is_sel = (i==search_option);
			TTF_Font* _font = font.large;
			SDL_Color tc = COLOR_WHITE;
			int available_width = sw;
			if (fancy) {
				_font = font.medium;
				tc = COLOR_GRAY;
				available_width = screen->w - (screen->w*3/5);
			}
			if (is_sel && fancy) {
				_font = font.large;
				tc = COLOR_WHITE;
				available_width = sw;
			}
			char display_name[256];
			int text_width = GFX_truncateText(_font, opt_name, display_name, available_width, SCALE1(BUTTON_PADDING*2));
			int max_width = MIN(available_width, text_width);
			int row_y = ry+j*row_h;
			if (is_sel && !fancy) {
				GFX_blitPill(ASSET_WHITE_PILL, screen, &(SDL_Rect){sx,row_y,max_width,SCALE1(PILL_SIZE)});
				tc = COLOR_BLACK;
			}
			SDL_Surface* text = TTF_RenderUTF8_Blended(_font, display_name, tc);
			SDL_BlitSurface(text, &(SDL_Rect){0,0,max_width-SCALE1(BUTTON_PADDING*2),text->h}, screen, &(SDL_Rect){sx+SCALE1(BUTTON_PADDING),row_y+SCALE1(4)});
			SDL_FreeSurface(text);
		}
	}
	else {
		// query line
		char qline[256+16];
		sprintf(qline, "> %s_", kb_text);
		char qdisp[256+16];
		GFX_truncateText(font.large, qline, qdisp, sw, SCALE1(BUTTON_PADDING*2));
		SDL_Surface* qtext = TTF_RenderUTF8_Blended(font.large, qdisp, COLOR_GOLD);
		int qy = SCALE1(PADDING);
		SDL_BlitSurface(qtext, &(SDL_Rect){0,0,sw,qtext->h}, screen, &(SDL_Rect){sx,qy});
		SDL_FreeSurface(qtext);

		if (show_search_menu==2) { // results fill the screen under the query
			int rtotal = search_results ? search_results->count : 0;
			// Reserve one row for the query line so the last result never
			// slides under the bottom button hints (menu uses full height,
			// search loses one row to "> query").
			int srows = rows-1;
			if (srows<1) srows = 1;
			// Fancy mode: paint the selected hit's boxart behind the list,
			// same as the normal browser (fixes "no boxart in search").
			if (fancy && rtotal>0 && search_selected>=0 && search_selected<rtotal) {
				SearchResult* sel_hit = search_results->items[search_selected];
				char sel_emu[256];
				char sel_rom[256];
				getParentFolderName(sel_hit->path, sel_emu);
				getDisplayNameParens(sel_hit->path, sel_rom);
				char sel_boxart[512];
				sel_boxart[0] = '\0';
				if (sel_hit->type==ENTRY_PAK) {
					sprintf(sel_boxart, "%s/Imgs/%s.png", sel_hit->path, sel_hit->name);
					if (!exists(sel_boxart)) sel_boxart[0] = '\0';
				}
				else {
					sprintf(sel_boxart, ROMS_PATH "/%s/Imgs/%s.png", sel_emu, sel_rom);
					if (!exists(sel_boxart)) sel_boxart[0] = '\0';
				}
				if (sel_boxart[0]) {
					SDL_Surface* boxart = BoxartCache_get(sel_boxart);
					if (!boxart) {
						boxart = loadBoxart(sel_boxart);
						if (boxart) BoxartCache_put(sel_boxart, boxart);
					}
					if (boxart) {
						SDL_BlitSurface(boxart, NULL, screen, &(SDL_Rect){0,0});
					}
				}
			}
			int ry = qy + SCALE1(PILL_SIZE);
			int rend = search_start+srows;
			if (rend>rtotal) rend = rtotal;
			for (int i=search_start,j=0; i<rend; i++,j++) {
				SearchResult* hit = search_results->items[i];
				int is_sel = (i==search_selected);
				// Same typography as the main rom list: normal rows use
				// font.medium + gray in fancy mode, only the selected row
				// grows to font.large + white (with black outline for ROMs).
				TTF_Font* _font = font.large;
				SDL_Color tc = COLOR_WHITE;
				int available_width = sw;
				if (fancy) {
					_font = font.medium;
					tc = COLOR_GRAY;
					available_width = screen->w - (screen->w*3/5);
				}
				if (is_sel && fancy) {
					_font = font.large;
					tc = COLOR_WHITE;
					available_width = sw;
				}
				// Y step matches the main list (compressed in fancy mode).
				int row_h = SCALE1(PILL_SIZE-(5*fancy));
				int row_y = ry+j*row_h;
				// Build the final "Name (emu)" string FIRST, then truncate
				// once so the pill width always matches the drawn text.
				char rawline[320];
				sprintf(rawline, "%s (%s)", hit->name, hit->emu);
				char lined[320];
				int text_width = GFX_truncateText(_font, rawline, lined, available_width, SCALE1(BUTTON_PADDING*2));
				int max_width = MIN(available_width, text_width);
				if (is_sel && !fancy) {
					GFX_blitPill(ASSET_WHITE_PILL, screen, &(SDL_Rect){sx,row_y,max_width,SCALE1(PILL_SIZE)});
					tc = COLOR_BLACK;
				}
				SDL_Surface* text = TTF_RenderUTF8_Blended(_font, lined, tc);
				if (is_sel && fancy && hit->type==ENTRY_ROM) { // black outline, like the main list
					SDL_Surface* textout = TTF_RenderUTF8_Blended(font.largeoutline, lined, COLOR_BLACK);
					SDL_BlitSurface(textout, &(SDL_Rect){0,0,max_width-SCALE1(BUTTON_PADDING*2),textout->h}, screen, &(SDL_Rect){sx+SCALE1(BUTTON_PADDING),row_y+SCALE1(4)});
					SDL_FreeSurface(textout);
				}
				SDL_BlitSurface(text, &(SDL_Rect){0,0,max_width-SCALE1(BUTTON_PADDING*2),text->h}, screen, &(SDL_Rect){sx+SCALE1(BUTTON_PADDING),row_y+SCALE1(4)});
				SDL_FreeSurface(text);
			}
			if (rtotal==0) {
				GFX_blitMessage(font.large, "No results", screen, &(SDL_Rect){0,ry,screen->w,screen->h-ry});
			}
		}
		else { // keyboard grid
			Search_drawKeyboard(screen, sx, qy, sw);
		}
	}

	// buttons
	if (show_setting && !GetHDMI()) GFX_blitHardwareHints(screen, show_setting, fancy);
	else if (show_search_menu==2) GFX_blitButtonGroup((char*[]){ "A","OPEN", "START","FAV", "B","BACK", NULL }, 1, screen, 1, fancy);
	else if (show_search_menu==1) GFX_blitButtonGroup((char*[]){ "A","TYPE", "L1","DEL", "R1","SPACE", "START","GO", "B","BACK", NULL }, 1, screen, 1, fancy);
	else GFX_blitButtonGroup((char*[]){ "A","OK", "B","BACK", NULL }, 1, screen, 1, fancy);
}

// (forward declarations for isMetadataFile / boxart helpers live above the
// search block, before first use)
static Array* getRoot(void);
static Array* getRecents(void);
static Array* getCollection(char* path);
//static Array* getDiscs(char* path);
static Array* getEntries(char* path);
static Array* getFavorites(void);

static Array* getHiddens(void);

static Directory* Directory_new(char* path, int selected) {
	char display_name[256];
	getDisplayName(path, display_name);

	Directory* self = malloc(sizeof(Directory));
	self->path = strdup(path);
	self->name = strdup(display_name);
	if (exactMatch(path, SDCARD_PATH)) {
		self->entries = getRoot();
	}
	else if (exactMatch(path, FAUX_RECENT_PATH)) {
		self->entries = getRecents();
	}
	else if (exactMatch(path, FAUX_FAVORITE_PATH)) {
		self->entries = getFavorites();
	}
	else if (exactMatch(path, FAUX_HIDDEN_PATH)) {
		self->entries = getHiddens();
	}
	else if (!exactMatch(path, COLLECTIONS_PATH) && prefixMatch(COLLECTIONS_PATH, path) && suffixMatch(".txt", path)) {
		self->entries = getCollection(path);
	}
//	else if (suffixMatch(".m3u", path)) {
//		self->entries = getDiscs(path);
//	}
	else {
		self->entries = getEntries(path);
	}
	self->alphas = IntArray_new();
	self->selected = selected;
	Directory_index(self);
	return self;
}
static void Directory_free(Directory* self) {
	free(self->path);
	free(self->name);
	EntryArray_free(self->entries);
	IntArray_free(self->alphas);
	free(self);
}

static void DirectoryArray_pop(Array* self) {
	Directory_free(Array_pop(self));
}
static void DirectoryArray_free(Array* self) {
	for (int i=0; i<self->count; i++) {
		Directory_free(self->items[i]);
	}
	Array_free(self);
}

typedef struct Favorite {
	char* path; // NOTE: this is without the SDCARD_PATH prefix!
	int available;
} Favorite;

static Favorite* Favorite_new(char* path) {
	Favorite* self = malloc(sizeof(Favorite));

	char sd_path[256]; // only need to get emu name
	sprintf(sd_path, "%s%s", SDCARD_PATH, path);

	char emu_name[256];
	getEmuName(sd_path, emu_name);

	self->path = strdup(path);
	self->available = hasEmu(emu_name);
	return self;
}
static void Favorite_free(Favorite* self) {
	free(self->path);
	free(self);
}

static int FavoriteArray_indexOf(Array* self, char* str) {
	for (int i=0; i<self->count; i++) {
		Favorite* item = self->items[i];
		if (exactMatch(item->path, str)) return i;
	}
	return -1;
}
static void FavoriteArray_clear(Array* self) {
	for (int i=0; i<self->count; i++) {
		Favorite_free(self->items[i]);
	}
	self->count = 0;
}
static void FavoriteArray_free(Array* self) {
	for (int i=0; i<self->count; i++) {
		Favorite_free(self->items[i]);
	}
	Array_free(self);
}

static int FavoriteArray_splice(Array* self, int index) {
	if (index != -1) {
		Favorite_free(self->items[index]);
		for(int i=index; i<self->count-1; i++) {
			self->items[i] = self->items[i+1];
		}
		self->items[self->count-1] = NULL;
		--self->count;
	}
	return  index;
}

///////////////////////////////////////

typedef struct Hidden {
	char* path; // NOTE: this is without the SDCARD_PATH prefix!
	int available;
} Hidden;

static Hidden* Hidden_new(char* path) {
	Hidden* self = malloc(sizeof(Hidden));

	char sd_path[256]; // only need to get emu name
	sprintf(sd_path, "%s%s", SDCARD_PATH, path);

	char emu_name[256];
	getEmuName(sd_path, emu_name);

	self->path = strdup(path);
	self->available = hasEmu(emu_name);
	return self;
}
static void Hidden_free(Hidden* self) {
	free(self->path);
	free(self);
}

static int HiddenArray_indexOf(Array* self, char* str) {
	for (int i=0; i<self->count; i++) {
		Hidden* item = self->items[i];
		if (exactMatch(item->path, str)) return i;
	}
	return -1;
}
static void HiddenArray_clear(Array* self) {
	for (int i=0; i<self->count; i++) {
		Hidden_free(self->items[i]);
	}
	self->count = 0;
}
static void HiddenArray_free(Array* self) {
	for (int i=0; i<self->count; i++) {
		Hidden_free(self->items[i]);
	}
	Array_free(self);
}

static int HiddenArray_splice(Array* self, int index) {
	if (index != -1) {
		Hidden_free(self->items[index]);
		for(int i=index; i<self->count-1; i++) {
			self->items[i] = self->items[i+1];
		}
		self->items[self->count-1] = NULL;
		--self->count;
	}
	return  index;
}

///////////////////////////////////////



typedef struct Recent {
	char* path; // NOTE: this is without the SDCARD_PATH prefix!
	char* alias;
	int available;
} Recent;
 // yiiikes
static char* recent_alias = NULL;


static Recent* Recent_new(char* path, char* alias) {
	Recent* self = malloc(sizeof(Recent));

	char sd_path[256]; // only need to get emu name
	sprintf(sd_path, "%s%s", SDCARD_PATH, path);

	char emu_name[256];
	getEmuName(sd_path, emu_name);

	self->path = strdup(path);
	self->alias = alias ? strdup(alias) : NULL;
	self->available = hasEmu(emu_name);
	return self;
}
static void Recent_free(Recent* self) {
	free(self->path);
	if (self->alias) free(self->alias);
	free(self);
}

static void RecentArray_clear(Array* self) {
	for (int i=0; i<self->count; i++) {
		Recent_free(self->items[i]);
	}
	self->count = 0;
}
static int RecentArray_indexOf(Array* self, char* str) {
	for (int i=0; i<self->count; i++) {
		Recent* item = self->items[i];
		if (exactMatch(item->path, str)) return i;
	}
	return -1;
}
static void RecentArray_free(Array* self) {
	for (int i=0; i<self->count; i++) {
		Recent_free(self->items[i]);
	}
	Array_free(self);
}

///////////////////////////////////////

static Directory* top;
static Array* stack; // DirectoryArray
static Array* recents; // RecentArray
static Array* favorites; // FavoriteArray
static Array* hiddens; // HiddenArray

static int quit = 0;
static int can_resume = 0;
static int should_resume = 0; // set to 1 on BTN_RESUME but only if can_resume==1
static int last_selected_slot = 0;
static char slot_path[256];
static char slot_path_rom[256];

static int restore_depth = -1;
static int restore_relative = -1;
static int restore_selected = 0;
static int restore_start = 0;
static int restore_end = 0;


// Fancy-only build: no simple/standard modes anymore.
int fancy_mode = 1;
static int hide_state = 0;
static int hide_boxartifstate = 0;


///////////////////////////////////////

#define MAX_RECENTS 24 // a multiple of all menu rows
static void saveRecents(void) {
	FILE* file = fopen(RECENT_PATH, "w");
	if (file) {
		for (int i=0; i<recents->count; i++) {
			Recent* recent = recents->items[i];
			fputs(recent->path, file);
			if (recent->alias) {
				fputs("\t", file);
				fputs(recent->alias, file);
			}
			putc('\n', file);
		}
		fclose(file);
	}
}
static void addRecent(char* path, char* alias) {
	path += strlen(SDCARD_PATH); // makes paths platform agnostic
	int id = RecentArray_indexOf(recents, path);
	if (id==-1) { // add
		while (recents->count>=MAX_RECENTS) {
			Recent_free(Array_pop(recents));
		}
		Array_unshift(recents, Recent_new(path, alias));
	}
	else if (id>0) { // bump to top
		for (int i=id; i>0; i--) {
			void* tmp = recents->items[i-1];
			recents->items[i-1] = recents->items[i];
			recents->items[i] = tmp;
		}
	}
	saveRecents();
}

static void saveFavorites(void) {
	FILE* file = fopen(FAVORITE_PATH, "w");
	if (file) {
		for (int i=0; i<favorites->count; i++) {
			Favorite* favorite = favorites->items[i];
			fputs(favorite->path, file);
			putc('\n', file);
		}
		fclose(file);
	}
}
static void toggleFavorite(char* path) {
	//printf("TEST FAV: %s\n",path);
	if (!prefixMatch(SDCARD_PATH, path)) return; // safety: favorites always live under SDCARD
	path += strlen(SDCARD_PATH); // makes paths platform agnostic
	//printf("TEST FAV: %s\n",path);
	int id = FavoriteArray_indexOf(favorites, path);
	//printf("TEST FAV: %s / ID = %d\n",path,id);
	if (id==-1) { // add
		Array_unshift(favorites, Favorite_new(path));
	}
	else { // remove
		FavoriteArray_splice(favorites, id);
	}
	saveFavorites();
	// If we are currently inside the Favorites folder the on-screen list is
	// now stale (it still holds the removed Entry). Rebuild it in place so
	// the selection can never point at a freed/out-of-range entry (which
	// used to grey-screen on the next enter/leave).
	if (top && exactMatch(top->path, FAUX_FAVORITE_PATH)) {
		int rows = MAIN_ROW_COUNT + fancy_mode;
		EntryArray_free(top->entries);
		top->entries = getFavorites();
		top->alphas->count = 0;
		Directory_index(top);
		int count = top->entries->count;
		if (count==0) {
			top->selected = 0;
			top->start = 0;
			top->end = 0;
		}
		else {
			if (top->selected>=count) top->selected = count-1;
			if (top->selected<0) top->selected = 0;
			if (top->selected<top->start) top->start = top->selected;
			if (top->selected>=top->start+rows) top->start = top->selected-rows+1;
			if (top->start<0) top->start = 0;
			top->end = top->start+rows;
			if (top->end>count) top->end = count;
		}
	}
}

// add/remove the Favorites entry in root on the fly, so the folder appears
// immediately when the first game is favorited (and disappears when the last
// favorite is removed) without needing a reload/reboot
static int countAvailableFavorites(void) {
	int count = 0;
	for (int i=0; i<favorites->count; i++) {
		Favorite* favorite = favorites->items[i];
		if (favorite->available) count += 1;
	}
	return count;
}
static void refreshRootEntries(void) {
	if (stack==NULL || stack->count==0) return;
	Directory* root = stack->items[0];
	if (!exactMatch(root->path, SDCARD_PATH)) return;

	int found = -1;
	for (int i=0; i<root->entries->count; i++) {
		Entry* entry = root->entries->items[i];
		if (exactMatch(entry->path, FAUX_FAVORITE_PATH)) {
			found = i;
			break;
		}
	}

	int rows = MAIN_ROW_COUNT + fancy_mode;
	if (countAvailableFavorites()>0 && found==-1) {
		// insert the Favorites folder after Recents (or at the top)
		int index = 0;
		for (int i=0; i<root->entries->count; i++) {
			Entry* entry = root->entries->items[i];
			if (exactMatch(entry->path, FAUX_RECENT_PATH)) {
				index = i+1;
				break;
			}
		}
		Array_push(root->entries, NULL); // grow the array
		for (int i=root->entries->count-1; i>index; i--) {
			root->entries->items[i] = root->entries->items[i-1];
		}
		root->entries->items[index] = Entry_new(FAUX_FAVORITE_PATH, ENTRY_DIR);
		if (index<=root->selected) root->selected += 1;
	}
	else if (countAvailableFavorites()==0 && found!=-1) {
		// remove the empty Favorites folder
		Entry_free(root->entries->items[found]);
		for (int i=found; i<root->entries->count-1; i++) {
			root->entries->items[i] = root->entries->items[i+1];
		}
		root->entries->count--;
		if (found<root->selected) root->selected -= 1;
	}
	else return; // nothing changed

	// keep the selection and the visible window valid
	if (root->entries->count==0) {
		root->selected = 0;
		root->start = 0;
		root->end = 0;
		return;
	}
	if (root->selected<0) root->selected = 0;
	if (root->selected>=root->entries->count) root->selected = root->entries->count-1;
	if (root->selected<root->start) root->start = root->selected;
	if (root->selected>=root->start+rows) root->start = root->selected-rows+1;
	if (root->start<0) root->start = 0;
	root->end = root->start+rows;
	if (root->end>root->entries->count) root->end = root->entries->count;

	// rebuild the alpha index for the updated root entries
	root->alphas->count = 0;
	Directory_index(root);
}

static int hasM3u(char* rom_path, char* m3u_path) { // NOTE: rom_path not dir_path
	char* tmp;

	strcpy(m3u_path, rom_path);
	tmp = strrchr(m3u_path, '/') + 1;
	tmp[0] = '\0';

	// path to parent directory
	char base_path[256];
	strcpy(base_path, m3u_path);

	tmp = strrchr(m3u_path, '/');
	tmp[0] = '\0';

	// get parent directory name
	char dir_name[256];
	tmp = strrchr(m3u_path, '/');
	strcpy(dir_name, tmp);

	// dir_name is also our m3u file name
	tmp = m3u_path + strlen(m3u_path);
	strcpy(tmp, dir_name);

	// add extension
	tmp = m3u_path + strlen(m3u_path);
	strcpy(tmp, ".m3u");

	return exists(m3u_path);
}



static int isFavorite(char *path) {
	if (!prefixMatch(SDCARD_PATH, path)) return 0; // faux dirs (Favorites/Recents/...) have no SDCARD prefix
	path += strlen(SDCARD_PATH); // makes paths platform agnostic
	int id = FavoriteArray_indexOf(favorites, path);
	return ++id;
}

static int hasEmu(char* emu_name) {
	char pak_path[256];
	sprintf(pak_path, "%s/Emus/%s.pak/launch.sh", PAKS_PATH, emu_name);
	if (exists(pak_path)) return 1;

	sprintf(pak_path, "%s/Emus/%s/%s.pak/launch.sh", SDCARD_PATH, PLATFORM, emu_name);
	return exists(pak_path);
}
static int hasCue(char* dir_path, char* cue_path) { // NOTE: dir_path not rom_path
	char* tmp = strrchr(dir_path, '/') + 1; // folder name
	sprintf(cue_path, "%s/%s.cue", dir_path, tmp);
	return exists(cue_path);
}

static int hasRecents(void) {
	LOG_info("hasRecents %s\n", RECENT_PATH);
	int has = 0;

	RecentArray_clear(recents); // getRoot() may run more than once; avoid duplicates
	Array* parent_paths = Array_new();
/*	if (exists(CHANGE_DISC_PATH)) {
		char sd_path[256];
		getFile(CHANGE_DISC_PATH, sd_path, 256);
		if (exists(sd_path)) {
			char* disc_path = sd_path + strlen(SDCARD_PATH); // makes path platform agnostic
			Recent* recent = Recent_new(disc_path, NULL);
			if (recent->available) has += 1;
			Array_push(recents, recent);

			char parent_path[256];
			strcpy(parent_path, disc_path);
			char* tmp = strrchr(parent_path, '/') + 1;
			tmp[0] = '\0';
			Array_push(parent_paths, strdup(parent_path));
		}
		unlink(CHANGE_DISC_PATH);
	} */

	FILE* file = fopen(RECENT_PATH, "r"); // newest at top
	if (file) {
		char line[256];
		while (fgets(line,256,file)!=NULL) {
			normalizeNewline(line);
			trimTrailingNewlines(line);
			if (strlen(line)==0) continue; // skip empty lines

			LOG_info("line: %s\n", line);

			char* path = line;
			char* alias = NULL;
			char* tmp = strchr(line,'\t');
			if (tmp) {
				tmp[0] = '\0';
				alias = tmp+1;
			}

			char sd_path[256];
			sprintf(sd_path, "%s%s", SDCARD_PATH, path);
			if (exists(sd_path)) {
				if (recents->count<MAX_RECENTS) {
					// this logic replaces an existing disc from a multi-disc game with the last used
					char m3u_path[256];
					if (hasM3u(sd_path, m3u_path)) { // TODO: this might tank launch speed
						char parent_path[256];
						strcpy(parent_path, path);
						char* tmp = strrchr(parent_path, '/') + 1;
						tmp[0] = '\0';

						int found = 0;
						for (int i=0; i<parent_paths->count; i++) {
							char* path = parent_paths->items[i];
							if (prefixMatch(path, parent_path)) {
								found = 1;
								break;
							}
						}
						if (found) continue;

						Array_push(parent_paths, strdup(parent_path));
					}

					LOG_info("path:%s alias:%s\n", path, alias);

					Recent* recent = Recent_new(path, alias);
					if (recent->available) has += 1;
					Array_push(recents, recent);
				}
			}
		}
		fclose(file);
	}

	saveRecents();

	StringArray_free(parent_paths);
	return has>0;
	//return 1;
}


static void saveHiddens(void) {
	FILE* file = fopen(HIDDEN_PATH, "w");
	if (file) {
		for (int i=0; i<hiddens->count; i++) {
			Hidden* hidden = hiddens->items[i];
			fputs(hidden->path, file);
			putc('\n', file);
		}
		fclose(file);
	}
}

static int hasHiddens(void) {
	int has = 0;

	HiddenArray_clear(hiddens); // getRoot() may run more than once; avoid duplicates
	FILE* file = fopen(HIDDEN_PATH, "r"); // newest at top
	if (file) {
		char line[256];
		while (fgets(line,256,file)!=NULL) {
			normalizeNewline(line);
			trimTrailingNewlines(line);
			if (strlen(line)==0) continue; // skip empty lines

			char sd_path[256];
			sprintf(sd_path, "%s%s", SDCARD_PATH, line);
			if (exists(sd_path)) {
					Hidden* hidden = Hidden_new(line);
					if (hidden->available) has += 1;
					Array_push(hiddens, hidden);
			}
		}
		fclose(file);
	}

	saveHiddens();

	//return has>0;
	return 1; // Always show directory, even if empty.
}


static void toggleHidden(char* path) {
	//printf("TEST FAV: %s\n",path);
	path += strlen(SDCARD_PATH); // makes paths platform agnostic
	//printf("TEST FAV: %s\n",path);
	int id = HiddenArray_indexOf(hiddens, path);
	//printf("TEST FAV: %s / ID = %d\n",path,id);
	if (id==-1) { // add
		Array_unshift(hiddens, Hidden_new(path));
	}
	else { // remove
		HiddenArray_splice(hiddens, id);
	}
	saveHiddens();
}

static int isHidden(char * parentpath, char *path) {
	char fullpath[256];
	parentpath += strlen(SDCARD_PATH); // makes paths platform agnostic
	sprintf(fullpath,"%s/%s", parentpath, path);
	int id = HiddenArray_indexOf(hiddens, fullpath);
	//printf("check if %s is hidden = %d\n", fullpath, id);
	return ++id;
}

static int hasFavorites(void) {
	int has = 0;
	FavoriteArray_clear(favorites); // getRoot() may run more than once; avoid duplicates
	Array* parent_paths = Array_new();
	FILE* file = fopen(FAVORITE_PATH, "r"); // newest at top
	if (file) {
		char line[256];
		while (fgets(line,256,file)!=NULL) {
			normalizeNewline(line);
			trimTrailingNewlines(line);
			if (strlen(line)==0) continue; // skip empty lines

			// Legacy files may contain an absolute m3u path (SDCARD prefix +
			// "/Folder/Folder.m3u"); normalize everything to a RELATIVE path
			// without SDCARD prefix before any other handling.
			char rel[256];
			if (prefixMatch(SDCARD_PATH, line)) {
				strcpy(rel, line + strlen(SDCARD_PATH));
			}
			else {
				strcpy(rel, line);
			}
			char sd_path[256];
			sprintf(sd_path, "%s%s", SDCARD_PATH, rel);
			if (exists(sd_path)) {
					char m3u_path[256];
					if (hasM3u(sd_path, m3u_path)) { // TODO: this might tank launch speed
						char parent_path[256];
						strcpy(parent_path, rel);
						char* tmp = strrchr(parent_path, '/') + 1;
						tmp[0] = '\0';

						int found = 0;
						for (int i=0; i<parent_paths->count; i++) {
							char* path = parent_paths->items[i];
							if (exactMatch(path, parent_path)) {
								found = 1;
								break;
							}
						}
						if (found) continue;

						Array_push(parent_paths, strdup(parent_path));
						// Collapse all discs of the same folder game into one
						// entry: store the RELATIVE folder path (no SDCARD
						// prefix, no absolute m3u path!) so Favorite_new(),
						// getFavorites() and toggleFavorite() all agree.
						strcpy(m3u_path, parent_path);
						// trim trailing '/' left by the parent_path computation
						int mlen = strlen(m3u_path);
						if (mlen>0 && m3u_path[mlen-1]=='/') m3u_path[mlen-1] = '\0';
					} else {
						strcpy(m3u_path ,rel);
					}
					if (FavoriteArray_indexOf(favorites, m3u_path)!=-1) continue; // same game listed twice
					Favorite* favorite = Favorite_new(m3u_path);
					if (favorite->available) has += 1;
					Array_push(favorites, favorite);
			}
		}
		fclose(file);
	}
	StringArray_free(parent_paths);
	saveFavorites();

	return has>0;
	//return 1; // Always show directory, even if empty.
}

static int hasCollections(void) {
	int has = 0;
	if (!exists(COLLECTIONS_PATH)) return has;

	DIR *dh = opendir(COLLECTIONS_PATH);
	struct dirent *dp;
	while((dp = readdir(dh)) != NULL) {
		if (hide(dp->d_name)) continue;
		has = 1;
		break;
	}
	closedir(dh);
	return has;
}

// metadata/junk files that should never be listed or counted as roms
// (map.txt, _map.txt, gamelist.xml, desktop.ini, Thumbs.db, ...)
// NOTE: also used by the search block above; keep non-static so the
// earlier use links fine (single .c file, no header change needed)
int isMetadataFile(char* name) {
	if (suffixMatch(".txt", name)) return 1; // map.txt, _map.txt, notes...
	if (suffixMatch(".xml", name)) return 1; // gamelist.xml
	if (suffixMatch(".ini", name)) return 1; // desktop.ini
	if (suffixMatch(".db", name)) return 1;  // Thumbs.db
	if (suffixMatch(".cfg", name)) return 1;
	return 0;
}

// folder is empty (no visible rom/subfolder, metadata files don't count)
static int hasVisibleEntries(char* path) {
	DIR* dh = opendir(path);
	if (dh==NULL) return 0;
	int has = 0;
	struct dirent *dp;
	while((dp = readdir(dh)) != NULL) {
		if (hide(dp->d_name)) continue;
		if (isMetadataFile(dp->d_name)) continue;
		has = 1;
		break;
	}
	closedir(dh);
	return has;
}

static int hasRoms(char* dir_name) {
	int has = 0;
	char emu_name[256];
	char rom_path[256];

	getEmuName(dir_name, emu_name);

	// check for emu pak
	if (!hasEmu(emu_name)) return has;

	// check for at least one non-hidden file (we're going to assume it's a rom)
	sprintf(rom_path, "%s/%s/", ROMS_PATH, dir_name);
	DIR *dh = opendir(rom_path);
	if (dh!=NULL) {
		struct dirent *dp;
		while((dp = readdir(dh)) != NULL) {
			if (hide(dp->d_name)) continue;
			if (isMetadataFile(dp->d_name)) continue; // map.txt/gamelist.xml/... are not roms
			has = 1;
			break;
		}
		closedir(dh);
	}
	// if (!has) printf("No roms for %s!\n", dir_name);
	return has;
}
static Array* getRoot(void) {
	Array* root = Array_new();

	if (hasRecents()) Array_push(root, Entry_new(FAUX_RECENT_PATH, ENTRY_DIR));
	if (hasFavorites()) Array_push(root, Entry_new(FAUX_FAVORITE_PATH, ENTRY_DIR));


	Array* entries = Array_new();
	DIR* dh = opendir(ROMS_PATH);
	if (dh!=NULL) {
		struct dirent *dp;
		char* tmp;
		char full_path[256];
		sprintf(full_path, "%s/", ROMS_PATH);
		tmp = full_path + strlen(full_path);
		Array* emus = Array_new();
		while((dp = readdir(dh)) != NULL) {
			if (hide(dp->d_name)) continue;
			//if (isHidden(dp->d_name)) continue;
			if (hasRoms(dp->d_name)) {
				strcpy(tmp, dp->d_name);
				Array_push(emus, Entry_new(full_path, ENTRY_DIR));
			}
		}
		EntryArray_sort(emus);
		Entry* prev_entry = NULL;
		for (int i=0; i<emus->count; i++) {
			Entry* entry = emus->items[i];
			if (prev_entry!=NULL) {
				if (exactMatch(prev_entry->name, entry->name)) {
					Entry_free(entry);
					continue;
				}
			}
			Array_push(entries, entry);
			prev_entry = entry;
		}
		Array_free(emus); // just free the array part, entries now owns emus entries
		closedir(dh);
	}

	// copied/modded from Directory_index
	char map_path[256];
	sprintf(map_path, "%s/map.txt", ROMS_PATH);
	if (entries->count>0 && exists(map_path)) {
		FILE* file = fopen(map_path, "r");
		if (file) {
			Hash* map = Hash_new();
			char line[256];
			int resort = 0;
			while (fgets(line,256,file)!=NULL) {
				normalizeNewline(line);
				trimTrailingNewlines(line);
				if (strlen(line)==0) continue; // skip empty lines

				char* tmp = strchr(line,'\t');
				if (tmp) {
					tmp[0] = '\0';
					char* key = line;
					char* value = tmp+1;
					Hash_set(map, key, value);
				}
			}
			fclose(file);

			for (int i=0; i<entries->count; i++) {
				Entry* entry = entries->items[i];
				char* filename = strrchr(entry->path, '/')+1;
				char* alias = Hash_get(map, filename);
				if (alias) {
					free(entry->name);
					entry->name = strdup(alias);
					resort = 1;
				}
			}
			if (resort) EntryArray_sort(entries);
			Hash_free(map);
		}
	}

	if (hasCollections()) {
		if (entries->count) Array_push(root, Entry_new(COLLECTIONS_PATH, ENTRY_DIR));
		else { // no visible systems, promote collections to root
			dh = opendir(COLLECTIONS_PATH);
			if (dh!=NULL) {
				struct dirent *dp;
				char* tmp;
				char full_path[256];
				sprintf(full_path, "%s/", COLLECTIONS_PATH);
				tmp = full_path + strlen(full_path);
				Array* collections = Array_new();
				while((dp = readdir(dh)) != NULL) {
					if (hide(dp->d_name)) continue;
					strcpy(tmp, dp->d_name);
					Array_push(collections, Entry_new(full_path, ENTRY_DIR)); // yes, collections are fake directories
				}
				EntryArray_sort(collections);
				for (int i=0; i<collections->count; i++) {
					Array_push(entries, collections->items[i]);
				}
				Array_free(collections); // just free the array part, entries now owns collections entries
				closedir(dh);
			}
		}
	}

	// add systems to root
	for (int i=0; i<entries->count; i++) {
		Array_push(root, entries->items[i]);
	}
	Array_free(entries); // root now owns entries' entries

	// NOTE: Tools folder removed from root on purpose; Tools/*.pak now live
	// inside the Y/Options menu (Search first, then pak list sorted A-Z).
	if (hasHiddens() && exists(SHOW_HIDDEN_FOLDER_PATH)) Array_push(root, Entry_new(FAUX_HIDDEN_PATH, ENTRY_DIR));
	return root;
}
static Array* getRecents(void) {
	Array* entries = Array_new();
	for (int i=0; i<recents->count; i++) {
		Recent* recent = recents->items[i];
		if (!recent->available) continue;

		char sd_path[256];
		sprintf(sd_path, "%s%s", SDCARD_PATH, recent->path);
		int type = suffixMatch(".pak", sd_path) ? ENTRY_PAK : ENTRY_ROM; // ???
		Entry* entry = Entry_new(sd_path, type);
		if (recent->alias) {
			free(entry->name);
			entry->name = strdup(recent->alias);
		}
		Array_push(entries, entry);
	}
	return entries;
}

static Array* getHiddens(void) {
	Array* entries = Array_new();
	for (int i=0; i<hiddens->count; i++) {
		Hidden* hidden = hiddens->items[i];
		if (!hidden->available) continue;

		char sd_path[256];
		sprintf(sd_path, "%s%s", SDCARD_PATH, hidden->path);
		int type = suffixMatch(".pak", sd_path) ? ENTRY_PAK : ENTRY_ROM; // ???
		Array_push(entries, Entry_new(sd_path, type));
	}
	return entries;
}



static Array* getFavorites(void) {
	Array* entries = Array_new();
	for (int i=0; i<favorites->count; i++) {
		Favorite* favorite = favorites->items[i];
		if (!favorite->available) continue;

		char sd_path[256];
		sprintf(sd_path, "%s%s", SDCARD_PATH, favorite->path);
		int type = suffixMatch(".pak", sd_path) ? ENTRY_PAK : ENTRY_ROM; // ???
		Array_push(entries, Entry_new(sd_path, type));
	}
	return entries;
}


static Array* getCollection(char* path) {
	Array* entries = Array_new();
	FILE* file = fopen(path, "r");
	if (file) {
		char line[256];
		while (fgets(line,256,file)!=NULL) {
			normalizeNewline(line);
			trimTrailingNewlines(line);
			if (strlen(line)==0) continue; // skip empty lines

			char sd_path[256];
			sprintf(sd_path, "%s%s", SDCARD_PATH, line);
			if (exists(sd_path)) {
				int type = suffixMatch(".pak", sd_path) ? ENTRY_PAK : ENTRY_ROM; // ???
				Array_push(entries, Entry_new(sd_path, type));

				// char emu_name[256];
				// getEmuName(sd_path, emu_name);
				// if (hasEmu(emu_name)) {
					// Array_push(entries, Entry_new(sd_path, ENTRY_ROM));
				// }
			}
		}
		fclose(file);
	}
	return entries;
}
/*
static Array* getDiscs(char* path){

	// TODO: does path have SDCARD_PATH prefix?

	Array* entries = Array_new();

	char base_path[256];
	strcpy(base_path, path);
	char* tmp = strrchr(base_path, '/') + 1;
	tmp[0] = '\0';

	// TODO: limit number of discs supported (to 9?)
	FILE* file = fopen(path, "r");
	if (file) {
		char line[256];
		int disc = 0;
		while (fgets(line,256,file)!=NULL) {
			normalizeNewline(line);
			trimTrailingNewlines(line);
			if (strlen(line)==0) continue; // skip empty lines

			char disc_path[256];
			sprintf(disc_path, "%s%s", base_path, line);

			if (exists(disc_path)) {
				disc += 1;
				Entry* entry = Entry_new(disc_path, ENTRY_ROM);
				free(entry->name);
				char name[16];
				sprintf(name, "Disc %i", disc);
				entry->name = strdup(name);
				Array_push(entries, entry);
			}
		}
		fclose(file);
	}
	return entries;
}
static int getFirstDisc(char* m3u_path, char* disc_path) { // based on getDiscs() natch
	int found = 0;

	char base_path[256];
	strcpy(base_path, m3u_path);
	char* tmp = strrchr(base_path, '/') + 1;
	tmp[0] = '\0';

	FILE* file = fopen(m3u_path, "r");
	if (file) {
		char line[256];
		while (fgets(line,256,file)!=NULL) {
			normalizeNewline(line);
			trimTrailingNewlines(line);
			if (strlen(line)==0) continue; // skip empty lines

			sprintf(disc_path, "%s%s", base_path, line);

			if (exists(disc_path)) found = 1;
			break;
		}
		fclose(file);
	}
	return found;
}
*/
static void addEntries(Array* entries, char* path) {
	DIR *dh = opendir(path);
	if (dh!=NULL) {
		struct dirent *dp;
		char* tmp;
		char full_path[256];
		sprintf(full_path, "%s/", path);
		tmp = full_path + strlen(full_path);
		while((dp = readdir(dh)) != NULL) {
			if (hide(dp->d_name)) continue;
			if (isHidden(path, dp->d_name)) continue;
			int is_dir = dp->d_type==DT_DIR;
			int in_collections = prefixMatch(COLLECTIONS_PATH, path);
			// map.txt, _map.txt, gamelist.xml, ... are not roms
			// (in Collections the .txt files ARE the collections, keep them)
			if (!in_collections && !is_dir && isMetadataFile(dp->d_name)) continue;
			if (in_collections && !is_dir && !suffixMatch(".txt", dp->d_name) && isMetadataFile(dp->d_name)) continue;
			strcpy(tmp, dp->d_name);
			int type;
			if (is_dir) {
				// TODO: this should make sure launch.sh exists
				if (suffixMatch(".pak", dp->d_name)) {
					type = ENTRY_PAK;
				}
				else if (!in_collections && !hasVisibleEntries(full_path)) {
					continue; // hide empty folders
				}
				else {
					type = ENTRY_DIR;
				}
			}
			else {
				if (prefixMatch(COLLECTIONS_PATH, full_path)) {
					type = ENTRY_DIR; // :shrug:
				}
				else {
					type = ENTRY_ROM;
				}
			}
			Array_push(entries, Entry_new(full_path, type));
		}
		closedir(dh);
	}
}

static int isConsoleDir(char* path) {
	char* tmp;
	char parent_dir[256];
	strcpy(parent_dir, path);
	tmp = strrchr(parent_dir, '/');
	tmp[0] = '\0';

	return exactMatch(parent_dir, ROMS_PATH);
}

static Array* getEntries(char* path){
	Array* entries = Array_new();

	if (isConsoleDir(path)) { // top-level console folder, might collate
		char collated_path[256];
		strcpy(collated_path, path);
		char* tmp = strrchr(collated_path, '(');
		// 1 because we want to keep the opening parenthesis to avoid collating "Game Boy Color" and "Game Boy Advance" into "Game Boy"
		// but conditional so we can continue to support a bare tag name as a folder name
		if (tmp) tmp[1] = '\0';

		DIR *dh = opendir(ROMS_PATH);
		if (dh!=NULL) {
			struct dirent *dp;
			char full_path[256];
			sprintf(full_path, "%s/", ROMS_PATH);
			tmp = full_path + strlen(full_path);
			// while loop so we can collate paths, see above
			while((dp = readdir(dh)) != NULL) {
				if (hide(dp->d_name)) continue;
				//if (isHidden(dp->d_name)) continue;
				if (dp->d_type!=DT_DIR) continue;
				strcpy(tmp, dp->d_name);

				if (!prefixMatch(collated_path, full_path)) continue;
				addEntries(entries, full_path);
			}
			closedir(dh);
		}
	}
	else addEntries(entries, path); // just a subfolder

	EntryArray_sort(entries);
	return entries;
}

///////////////////////////////////////

static void queueNext(char* cmd) {
	LOG_info("cmd: %s\n", cmd);
	putFile("/tmp/next", cmd);
	quit = 1;
}

// based on https://stackoverflow.com/a/31775567/145965
static int replaceString(char *line, const char *search, const char *replace) {
   char *sp; // start of pattern
   if ((sp = strstr(line, search)) == NULL) {
      return 0;
   }
   int count = 1;
   int sLen = strlen(search);
   int rLen = strlen(replace);
   if (sLen > rLen) {
      // move from right to left
      char *src = sp + sLen;
      char *dst = sp + rLen;
      while((*dst = *src) != '\0') { dst++; src++; }
   } else if (sLen < rLen) {
      // move from left to right
      int tLen = strlen(sp) - sLen;
      char *stop = sp + rLen;
      char *src = sp + sLen + tLen;
      char *dst = sp + rLen + tLen;
      while(dst >= stop) { *dst = *src; dst--; src--; }
   }
   memcpy(sp, replace, rLen);
   count += replaceString(sp + rLen, search, replace);
   return count;
}
static char* escapeSingleQuotes(char* str) {
	// why not call replaceString directly?
	// call points require the modified string be returned
	// but replaceString is recursive and depends on its
	// own return value (but does it need to?)
	replaceString(str, "'", "'\\''");
	return str;
}

///////////////////////////////////////

static void readyResumePath(char* rom_path, int type) {
	char* tmp;
	can_resume = 0;
	char path[256];
	strcpy(path, rom_path);

	if (!prefixMatch(ROMS_PATH, path)) return;

	char auto_path[256];
	if (type==ENTRY_DIR) {
		if (!hasCue(path, auto_path)) { // no cue?
			tmp = strrchr(auto_path, '.') + 1; // extension
			strcpy(tmp, "m3u"); // replace with m3u
			if (!exists(auto_path)) return; // no m3u
		}
		strcpy(path, auto_path); // cue or m3u if one exists
	}

	if (!suffixMatch(".m3u", path)) {
		char m3u_path[256];
		if (hasM3u(path, m3u_path)) {
			// change path to m3u path
			strcpy(path, m3u_path);
		}
	}

	char emu_name[256];
	getEmuName(path, emu_name);

	char rom_file[256];
	//tmp = strrchr(path, '/') + 1;
	//strcpy(rom_file, tmp);
	getDisplayNameParens(path,rom_file);
	sprintf(slot_path, "%s/.minui/%s/%s.txt", SHARED_USERDATA_PATH, emu_name, rom_file); // /.userdata/shared/.minui/<EMU>/<romname>.txt
	//sprintf(slot_path_rom, "%s/%s/%s", MYSAVESTATE_PATH, emu_name, rom_file); // /.userdata/shared/.minui/<EMU>/<romname>.ext
	//sprintf(slot_path, "%s.txt", slot_path_rom); // /.userdata/.minui/<EMU>/<romname>.ext.txt
	int last_know_slot = 0;
	//can_resume = exists(slot_path);
	if (exists(slot_path)) {
		char slot[16];
		getFile(slot_path, slot, 16);
		if (slot[0]!='\0') {
			last_know_slot = atoi(slot);
		}
	}
	last_selected_slot = canResume(path, last_know_slot);
	if (last_selected_slot) can_resume = 1;
}

static void readyResume(Entry* entry) {
	readyResumePath(entry->path, entry->type);
}

static void saveLast(char* path);
static void loadLast(void);

static int autoResume(void) {
	// NOTE: bypasses recents
	if (!exists(AUTO_RESUME_PATH)) return 0;

	char path[256];
	getFile(AUTO_RESUME_PATH, path, 256);
	unlink(AUTO_RESUME_PATH);
	sync();
	if (exists(AUTO_RESUME_MODIFIER_PATH)){
		selected_modifier = 1;
		unlink(AUTO_RESUME_MODIFIER_PATH);
	}
	// make sure rom still exists
	char sd_path[256];
	sprintf(sd_path, "%s%s", SDCARD_PATH, path);
	if (!exists(sd_path)) return 0;

	// make sure emu still exists
	char emu_name[256];
	getEmuName(sd_path, emu_name);

	char emu_path[256];
	getEmuPath(emu_name, emu_path);

	if (!exists(emu_path)) return 0;

	// putFile(LAST_PATH, FAUX_RECENT_PATH); // saveLast() will crash here because top is NULL
	char _romname[256];
	getDisplayNameParens(path,_romname);
	char cmd[256];
	sprintf(cmd, "'%s' '%s' %d '%s/%s/States/%s.state' %d ", escapeSingleQuotes(emu_path), escapeSingleQuotes(sd_path), AUTO_RESUME_SLOT, MYSAVESTATE_PATH, escapeSingleQuotes(emu_name),escapeSingleQuotes(_romname), selected_modifier );
	putInt(RESUME_SLOT_PATH, AUTO_RESUME_SLOT);
	selected_modifier = 0;
	queueNext(cmd);
	return 1;
}

static void openPak(char* path) {
	// NOTE: escapeSingleQuotes() modifies the passed string
	// so we need to save the path before we call that
	// if (prefixMatch(ROMS_PATH, path)) {
	// 	addRecent(path);
	// }
	saveLast(path);

	char cmd[256];
	sprintf(cmd, "'%s/launch.sh'", escapeSingleQuotes(path));
	queueNext(cmd);
}
static void openRom(char* path, char* last) {
	LOG_info("openRom(%s,%s)\nSELECT = %d\n", path, last, selected_modifier);
	char sd_path[256];
	strcpy(sd_path, path);
	int loadslot=-1;

	char m3u_path[256];
	char recent_path[256];
	if (hasM3u(sd_path, m3u_path)) {
		//the m3u path must be reduced to the folder path.
		char* tmp = strrchr(m3u_path, '/');
		tmp[0] = '\0'; // remove m3u file name
		strcpy(recent_path, m3u_path);
	} else {
		strcpy(recent_path, sd_path);
	}
	char emu_name[256];
	getEmuName(sd_path, emu_name);

	if (should_resume) {
		char slot[16];
		getFile(slot_path, slot, 16);
		putFile(RESUME_SLOT_PATH, slot);
		should_resume = 0;
		loadslot=last_selected_slot;
	}
	char emu_path[256];
	getEmuPath(emu_name, emu_path);

	// NOTE: escapeSingleQuotes() modifies the passed string
	// so we need to save the path before we call that
	addRecent(recent_path, recent_alias); // yiiikes
	saveLast(last==NULL ? sd_path : last);

	char statepath[256];
	char _romname[256];
	getDisplayNameParens(sd_path,_romname);
	getStatePath(sd_path, statepath);
	char cmd[512];
	sprintf(cmd, "'%s' '%s' %d '%s/%s.state%d' %d ", escapeSingleQuotes(emu_path), escapeSingleQuotes(sd_path), loadslot, escapeSingleQuotes(statepath), escapeSingleQuotes(_romname),loadslot, selected_modifier);
	selected_modifier = 0;
	queueNext(cmd);
}

static void openDirectory(char* path, int auto_launch) {
	char auto_path[256];
	if (hasCue(path, auto_path) && auto_launch) {
		openRom(auto_path, path);
		return;
	}

	char m3u_path[256];
	strcpy(m3u_path, auto_path);
	char* tmp = strrchr(m3u_path, '.') + 1; // extension
	strcpy(tmp, "m3u"); // replace with m3u
//	LOG_info("OpenDirectory: %s\n", path);
	//int ism3u = hasM3u(path, m3u_path);
//	int ism3u=exists(m3u_path);
//	LOG_info("OpenDirectory: %s - auto_path %s - M3U %d: %s\n", path, auto_path, ism3u, m3u_path);
	if (exists(m3u_path) && auto_launch) {
//		LOG_info("hasM3u %s\n", m3u_path);
//			auto_path[0] = '\0';
		openRom(m3u_path, path);
		return;
	}

//	if (hasM3u(path, m3u_path) && auto_launch) {
//		if (getFirstDisc(m3u_path, auto_path)) {
//			openRom(auto_path, path);
//			return;
//		}
		// TODO: doesn't handle empty m3u files
//	}

	int selected = 0;
	int start = selected;
	int end = 0;
	if (top && top->entries->count>0) {
		if (restore_depth==stack->count && top->selected==restore_relative) {
			selected = restore_selected;
			start = restore_start;
			end = restore_end;
		}
	}

	top = Directory_new(path, selected);
	// The favorites list may have shrunk (unfavorite) since the restore
	// snapshot was taken: clamp so selected/start/end can never point past
	// the freshly built entries array (grey-screen crash on re-enter).
	{
		int rows = MAIN_ROW_COUNT + fancy_mode;
		int count = top->entries->count;
		if (count==0) {
			top->selected = 0;
			top->start = 0;
			top->end = 0;
		}
		else {
			if (top->selected<0) top->selected = 0;
			if (top->selected>=count) top->selected = count-1;
			if (start<0) start = 0;
			if (start>=count) start = count-1;
			if (end<0) end = 0;
			if (end>count) end = count;
			// keep selected visible
			if (top->selected<start) start = top->selected;
			if (top->selected>=start+rows) start = top->selected-rows+1;
			if (start<0) start = 0;
			end = start+rows;
			if (end>count) end = count;
		}
	}
	top->start = start;
	top->end = end ? end : ((top->entries->count < ( MAIN_ROW_COUNT + fancy_mode )) ? top->entries->count : ( MAIN_ROW_COUNT + fancy_mode ));
	Array_push(stack, top);
}
static void closeDirectory(void) {
	restore_selected = top->selected;
	restore_start = top->start;
	restore_end = top->end;
	DirectoryArray_pop(stack);
	restore_depth = stack->count;
	top = stack->items[stack->count-1];
	restore_relative = top->selected;
}

static void Entry_open(Entry* self) {
	recent_alias = self->name;  // yiiikes
	if (self->type==ENTRY_ROM) {
		char *last = NULL;
		if (prefixMatch(COLLECTIONS_PATH, top->path)) {
			char* tmp;
			char filename[256];

			tmp = strrchr(self->path, '/');
			if (tmp) strcpy(filename, tmp+1);

			char last_path[256];
			sprintf(last_path, "%s/%s", top->path, filename);
			last = last_path;
		}
		// check if it is actually a folder with an m3u file inside, it could be a favorite.
		char m3upath[256];
		char tmpname[256];
		getDisplayNameParens(self->path,tmpname);
		sprintf(m3upath, "%s/%s.m3u", self->path, tmpname);
		if (exists(m3upath)){
			openDirectory(self->path, 1);
		} else {
			openRom(self->path, last);
		}
	}
	else if (self->type==ENTRY_PAK) {
		openPak(self->path);
	}
	else if (self->type==ENTRY_DIR) {
		openDirectory(self->path, 1);
	}
}

// search hits hold plain rom paths; launch them directly (the caller copies
// the hit before Search_close() frees search_results, so these pointers stay
// valid). Folder hits (multi-disc) go through openDirectory(auto_launch=1)
// which launches the cue/m3u inside; plain files go straight to openRom/openPak.
static void SearchResult_openAt(char* hit_path, char* hit_name, int hit_type) {
	char* recent_alias_saved = recent_alias;
	recent_alias = hit_name; // yiiikes
	if (hit_type==ENTRY_PAK) {
		openPak(hit_path);
	}
	else {
		Entry tmp;
		memset(&tmp, 0, sizeof(Entry));
		tmp.path = hit_path;
		tmp.name = hit_name;
		tmp.type = ENTRY_ROM;
		Entry_open(&tmp);
	}
	recent_alias = recent_alias_saved;
}

///////////////////////////////////////

static void saveLast(char* path) {
	// special case for recently played
	if (exactMatch(top->path, FAUX_RECENT_PATH)) {
		// NOTE: that we don't have to save the file because
		// your most recently played game will always be at
		// the top which is also the default selection
		path = FAUX_RECENT_PATH;
	}
	putFile(LAST_PATH, path);
}
static void loadLast(void) { // call after loading root directory
	if (!exists(LAST_PATH)) return;

	char last_path[256];
	getFile(LAST_PATH, last_path, 256);

	char full_path[256];
	strcpy(full_path, last_path);

	char* tmp;
	char filename[256];
	tmp = strrchr(last_path, '/');
	if (tmp) strcpy(filename, tmp);

	Array* last = Array_new();
	while (!exactMatch(last_path, SDCARD_PATH)) {
		Array_push(last, strdup(last_path));

		char* slash = strrchr(last_path, '/');
		last_path[(slash-last_path)] = '\0';
	}

	while (last->count>0) {
		char* path = Array_pop(last);
		if (!exactMatch(path, ROMS_PATH)) { // romsDir is effectively root as far as restoring state after a game
			char collated_path[256];
			collated_path[0] = '\0';
			if (suffixMatch(")", path) && isConsoleDir(path)) {
				strcpy(collated_path, path);
				tmp = strrchr(collated_path, '(');
				if (tmp) tmp[1] = '\0'; // 1 because we want to keep the opening parenthesis to avoid collating "Game Boy Color" and "Game Boy Advance" into "Game Boy"
			}

			for (int i=0; i<top->entries->count; i++) {
				Entry* entry = top->entries->items[i];

				// NOTE: strlen() is required for collated_path, '\0' wasn't reading as NULL for some reason
				if (exactMatch(entry->path, path) || (strlen(collated_path) && prefixMatch(collated_path, entry->path)) || (prefixMatch(COLLECTIONS_PATH, full_path) && suffixMatch(filename, entry->path))) {
					top->selected = i;
					if (i>=top->end) {
						top->start = i;
						top->end = top->start + ( MAIN_ROW_COUNT + fancy_mode );
						if (top->end>top->entries->count) {
							top->end = top->entries->count;
							top->start = top->end - ( MAIN_ROW_COUNT + fancy_mode );
						}
					}
					if (last->count==0 && !exactMatch(entry->path, FAUX_RECENT_PATH) && !exactMatch(entry->path, FAUX_HIDDEN_PATH) && !(!exactMatch(entry->path, COLLECTIONS_PATH) && prefixMatch(COLLECTIONS_PATH, entry->path))) break; // don't show contents of auto-launch dirs

					if (entry->type==ENTRY_DIR) {
						openDirectory(entry->path, 0);
						break;
					}
				}
			}
		}
		free(path); // we took ownership when we popped it
	}

	StringArray_free(last);
}

/* helper functions to draw boxart and savestate preview*/
int drawStatePreview(SDL_Surface* _screen, char* bmpPath, int stateIndex){
    #define WINDOW_RADIUS 4 // TODO: this logic belongs in blitRect?
	#define PAGINATION_HEIGHT 6
	// unscaled
	int hw = _screen->w * 3 / 5;
	int hh = _screen->h * 3 / 5;
	int pw = hw + SCALE1(WINDOW_RADIUS*2);
	int ph = hh + SCALE1(WINDOW_RADIUS*2 + PAGINATION_HEIGHT);
	int ox = _screen->w  - pw;
	int oy = (_screen->h - ph) / 2;
	GFX_blitRect(ASSET_STATE_BG, _screen, &(SDL_Rect){ox,oy,pw,ph});
	ox += SCALE1(WINDOW_RADIUS);
	oy += SCALE1(WINDOW_RADIUS);
	SDL_Surface* preview = NULL;
	SDL_Surface* unscaled_preview = IMG_Load(bmpPath);
	if (!unscaled_preview) {
        //printf("IMG_Load: %s\n", IMG_GetError());
        SDL_Rect preview = {ox,oy,hw,hh};
		SDL_FillRect(_screen, &preview, 0);
        GFX_blitMessage(font.large, "Empty Slot", _screen, &preview);
    } else {
		preview = zoomSurface(unscaled_preview, (1.0 * hw / unscaled_preview->w) , (1.0 * hh / unscaled_preview->h), 0);
	//	printf("SaveState BMP %s has size is %dx%d\n", bmpPath, unscaled_preview->w, unscaled_preview->h);
	}

    SDL_BlitSurface(preview, NULL, _screen, &(SDL_Rect){ox,oy});
	SDL_FreeSurface(preview);
	SDL_FreeSurface(unscaled_preview);

   // pagination
   #define MENU_SLOT_COUNT 8
	ox += (pw-SCALE1(16*MENU_SLOT_COUNT))/2;
	oy += hh+SCALE1(WINDOW_RADIUS)-SCALE1(PAGINATION_HEIGHT / 2 - 1);
	for (int i=1; i<MENU_SLOT_COUNT+1; i++) {
		if (i==stateIndex) GFX_blitAsset(ASSET_PAGE, NULL, _screen, &(SDL_Rect){ox+SCALE1((i-1)*16),oy});
		else GFX_blitAsset(ASSET_DOT, NULL, _screen, &(SDL_Rect){ox+SCALE1((i-1)*16)+4,oy+SCALE1(2)});
	}
	if (stateIndex == 0){
		ox -= SCALE1(16);
		GFX_blitAsset(ASSET_RED_PAGE, NULL, _screen, &(SDL_Rect){ox,oy});
	}
    return 1;
}

///////////////////////////////////////
// boxart cache + preload (single-threaded, no worker thread):
// every image is decoded and scaled to screen size only once, then kept
// in a round-robin cache so navigating back and forth is instant.
// un-cached images are preloaded while the menu is idle, one per frame,
// always in the main thread (no race conditions, no crashes on fast scrolling).

#define BOXART_CACHE_SIZE 24 // ~24 * 600KB of 16bit surfaces on a 640x480 screen
typedef struct BoxartCache {
	char* path;
	SDL_Surface* surface; // already scaled to screen size
} BoxartCache;
static BoxartCache boxart_cache[BOXART_CACHE_SIZE];
static int boxart_cache_next = 0;
static SDL_Surface* boxart_screen = NULL; // screen format used to scale surfaces

static void BoxartCache_clear(void) {
	for (int i=0; i<BOXART_CACHE_SIZE; i++) {
		if (boxart_cache[i].path) free(boxart_cache[i].path);
		if (boxart_cache[i].surface) SDL_FreeSurface(boxart_cache[i].surface);
		boxart_cache[i].path = NULL;
		boxart_cache[i].surface = NULL;
	}
	boxart_cache_next = 0;
}

static void BoxartCache_put(char* path, SDL_Surface* surface) {
	if (!surface || !path || !path[0]) return;
	// replace if already cached
	for (int i=0; i<BOXART_CACHE_SIZE; i++) {
		if (boxart_cache[i].path && exactMatch(boxart_cache[i].path, path)) {
			if (boxart_cache[i].surface && boxart_cache[i].surface!=surface) SDL_FreeSurface(boxart_cache[i].surface);
			boxart_cache[i].surface = surface;
			return;
		}
	}
	int slot = boxart_cache_next;
	boxart_cache_next = (boxart_cache_next+1)%BOXART_CACHE_SIZE;
	if (boxart_cache[slot].path) {
		free(boxart_cache[slot].path);
		if (boxart_cache[slot].surface) SDL_FreeSurface(boxart_cache[slot].surface);
	}
	boxart_cache[slot].path = strdup(path);
	boxart_cache[slot].surface = surface;
}

static SDL_Surface* BoxartCache_get(char* path) {
	if (!path || !path[0]) return NULL;
	for (int i=0; i<BOXART_CACHE_SIZE; i++) {
		if (boxart_cache[i].path && exactMatch(boxart_cache[i].path, path)) {
			return boxart_cache[i].surface;
		}
	}
	return NULL;
}

static SDL_Surface* loadBoxart(const char* path) { // NOTE: main thread only
	if (!boxart_screen) return NULL;
	SDL_Surface* img = IMG_Load(path);
	if (!img) return NULL;
	SDL_Surface* scaled = zoomSurface(img, (1.0 * boxart_screen->w / img->w), (1.0 * boxart_screen->h / img->h), 0);
	SDL_FreeSurface(img);
	if (!scaled) return NULL;
	// convert to the display format once so the per-frame blit is a fast copy
	SDL_Surface* display = SDL_ConvertSurface(scaled, boxart_screen->format, 0);
	if (display) {
		SDL_FreeSurface(scaled);
		scaled = display;
	}
	return scaled;
}

static void Boxart_init(SDL_Surface* screen) {
	boxart_screen = screen;
	BoxartCache_clear();
}

static void Boxart_quit(void) {
	BoxartCache_clear();
	boxart_screen = NULL;
}

// preload state: remembers which directory has been (or is being) preloaded
static char preload_dir[256] = "";
static int preload_cursor = 0;

static int getBoxartPath(Entry* entry, char* out); // defined below

static void boxartLoadEntry(Entry* entry) { // decode + cache one boxart if needed
	char path[512];
	if (!getBoxartPath(entry, path)) return; // no boxart for this entry
	if (BoxartCache_get(path)) return; // already cached
	SDL_Surface* surface = loadBoxart(path);
	if (surface) BoxartCache_put(path, surface);
}
static void boxartLoadSearchResult(SearchResult* hit) { // decode + cache one boxart for a search hit
	char emu_name[256];
	char rom_name[256];
	getParentFolderName(hit->path, emu_name);
	getDisplayNameParens(hit->path, rom_name);
	char path[512];
	if (hit->type==ENTRY_PAK) {
		sprintf(path, "%s/Imgs/%s.png", hit->path, hit->name);
		if (!exists(path)) return;
	}
	else {
		sprintf(path, ROMS_PATH "/%s/Imgs/%s.png", emu_name, rom_name);
		if (!exists(path)) return;
	}
	if (BoxartCache_get(path)) return;
	SDL_Surface* surface = loadBoxart(path);
	if (surface) BoxartCache_put(path, surface);
}

// resolves the boxart path for an entry (rom, folder, pak, faux dirs like
// Recently Played / Favorites / HiddenRoms / Collections / Tools), returns 1 if found
static int getBoxartPath(Entry* entry, char* out) {
	char emu_name[256];
	char rom_name[256];
	getParentFolderName(entry->path, emu_name);
	getDisplayNameParens(entry->path, rom_name);

  if (entry->type==ENTRY_ROM) {
		sprintf(out, ROMS_PATH "/%s/Imgs/%s.png", emu_name, rom_name);
		if (exists(out)) return 1;
	}
	else if (entry->type==ENTRY_PAK) {
		sprintf(out, "%s/Imgs/%s.png", entry->path, entry->name);
		if (exists(out)) return 1;
	}
	else if (entry->type==ENTRY_DIR) {
    char m3u_path[512];
		sprintf(m3u_path, "%s/%s.m3u", entry->path, rom_name);
		if (exists(m3u_path)) { // collated/multi-disc folder behaves like a rom
			sprintf(out, ROMS_PATH "/%s/Imgs/%s.png", emu_name, rom_name);
			if (exists(out)) return 1;
		}
		else {
			sprintf(out, "%s/Imgs/%s.png", entry->path, emu_name);
			if (exists(out)) return 1;
		}
	}

  // generic fallbacks (covers faux directories and subfolders)
	sprintf(out, "%s/Imgs/%s.png", entry->path, entry->name);
	if (exists(out)) return 1;

   sprintf(out, SDCARD_PATH "/Imgs/%s.png", entry->name);
	if (exists(out)) return 1;

   // NEW: system-wide default boxart fallback
	sprintf(out, SDCARD_PATH "/Imgs/default.png");
	if (exists(out)) return 1;
	out[0] = '\0';
	return 0;
}

// preload the boxart of the whole current directory while the menu is idle,
// at most one image per idle frame so input stays responsive (main thread only,
// this replaces the old worker thread to avoid crashes on fast scrolling)
static void boxartPreload(void) {
	if (!fancy_mode || !boxart_screen) return;

	// Y/Options menu (menu 0): preload Search/Shutdown/pak art while idle,
	// selected item first. The draw path is cache-only so D-pad never stalls.
	if (show_search && show_search_menu==0) {
		int total = MenuOpt_count();
		if (total<=0) return;
		if (!exactMatch(preload_dir, "<options>")) {
			strcpy(preload_dir, "<options>");
			preload_cursor = (search_option>=0 && search_option<total) ? search_option : 0;
		}
		for (int n = 0; n < total; n++) {
			int i = (preload_cursor + n) % total;
			MenuOpt_preloadAt(i);
			preload_cursor = (i + 1) % total;
			return;
		}
		return;
	}
	// When search is active, preload from search_results instead of top->entries.
	// NOTE: search_results holds SearchResult (not Entry), so use the dedicated loader.
	if (show_search && search_results && search_results->count > 0) {
		int total = search_results->count;
		if (!exactMatch(preload_dir, "<search>")) {
			strcpy(preload_dir, "<search>");
			preload_cursor = search_selected;
		}
		for (int n = 0; n < total; n++) {
			int i = (preload_cursor + n) % total;
			boxartLoadSearchResult(search_results->items[i]);
			preload_cursor = (i + 1) % total;
			return;
		}
		return;
	}
	if (!top) return;
	Array* source = top->entries;
	char* source_path = top->path;
	int source_selected = top->selected;
	if (!source || source->count == 0 || !source_path) return;

	int total = source->count;
	if (!exactMatch(preload_dir, source_path)) {
		strcpy(preload_dir, source_path);
		preload_cursor = source_selected;
	}
	for (int n = 0; n < total; n++) {
		int i = (preload_cursor + n) % total;
		boxartLoadEntry(source->items[i]);
		preload_cursor = (i + 1) % total;
		return;
	}
}

int drawBoxart(SDL_Surface* _screen, SDL_Surface* boxart){
    int ox = 0;
	int	oy = 0;
// window
	GFX_blitRect(ASSET_STATE_BG, _screen, &(SDL_Rect){ox,oy,_screen->w,_screen->h});
	if (boxart) {
		SDL_BlitSurface(boxart, NULL, _screen, &(SDL_Rect){ox,oy});
	}
	else { // safety: no surface available (image missing or failed to decode)
		SDL_Rect rect = {ox,oy,_screen->w,_screen->h};
		SDL_FillRect(_screen, &rect, 0);
	}
	return 1;
}


/* end helper functions*/



///////////////////////////////////////

static void Menu_init(void) {
	stack = Array_new(); // array of open Directories
	recents = Array_new();
	favorites = Array_new();
	hiddens = Array_new();

	openDirectory(SDCARD_PATH, 0);
	loadLast(); // restore state when available
}
static void Menu_quit(void) {
	RecentArray_free(recents);
	FavoriteArray_free(favorites);
	HiddenArray_free(hiddens);
	DirectoryArray_free(stack);
}

///////////////////////////////////////

int main (int argc, char *argv[]) {
	selected_modifier = 0;
	if (autoResume()) return 0; // nothing to do

	char tmpName[256];
	int itemnotchanged = 0;
	int page_seek_plus = BTN_RIGHT;
	int page_seek_minus = BTN_LEFT;
	int state_left = BTN_L2;
	int state_right = BTN_R2;

	if (exists(PAGE_SEEK_TRIGGERS)){
		page_seek_plus = BTN_R2;
		page_seek_minus = BTN_L2;
		state_left = BTN_LEFT;
		state_right = BTN_RIGHT;
	}

	// Fancy-only build: always fancy, no mode files.
	fancy_mode = 1;
	char pwractionstr[256];
	if (exists(PWR_SLEEP_PATH)){
		sprintf(pwractionstr,"SLP");
	} else {
		sprintf(pwractionstr,"OFF");
	}

	if (exists(HIDE_STATE_PATH)){
		hide_state = 1;
	}

	if (exists(HIDE_BOXART_IFSTATE_PATH)){
		hide_boxartifstate = 1;
	}
	LOG_info("MyMinUI\n");
	InitSettings();

	SDL_Surface* screen = GFX_init(MODE_MAIN);
	PAD_init();
	PWR_init();
	if (!HAS_POWER_BUTTON) PWR_disableSleep();
	Boxart_init(screen); // init the boxart cache (scaled to the screen format)

	SDL_Surface* version = NULL;

	Menu_init();

	// now that (most of) the heavy lifting is done, take a load off
	PWR_setCPUSpeed(CPU_SPEED_MENU);
	GFX_setVsync(VSYNC_STRICT);

	PAD_reset();
	int dirty = 1;
	int show_version = 0;
	int show_setting = 0; // 1=brightness,2=volume
	int was_online = PLAT_isOnline();
	while (!quit) {
		GFX_startFrame();
		unsigned long now = SDL_GetTicks();
	//	LOG_info("START PAD_poll\n");
		PAD_poll();
	//	LOG_info("END PAD_poll\n");

		int selected = top->selected;
		int total = top->entries->count;

		PWR_update(&dirty, &show_setting, NULL, NULL);

		int is_online = PLAT_isOnline();
		if (was_online!=is_online) dirty = 1;
		was_online = is_online;

		if (show_search) {
			// ---- search options popup + full 640px keyboard ----
			if (PAD_justPressed(BTN_B)) {
				if (show_search_menu==2) { // back from results to keyboard
					show_search_menu = 1;
					dirty = 1;
				}
				else if (show_search_menu==1) { // back from keyboard to options
					show_search_menu = 0;
					dirty = 1;
				}
				else { // close the popup entirely
					Search_close();
					dirty = 1;
				}
			}
			else if (show_search_menu==0) { // Y/Options menu via MenuOpt_at() (no Back)
				int topts = MenuOpt_count();
				// Same page size as Search_draw menu 0 (rows-1: one row
				// is taken by the header), and its own tools_start window
				// so results paging (search_start) never leaks in.
				int orows = MAIN_ROW_COUNT + fancy_mode - 1;
				if (orows<1) orows = 1;
				if (PAD_justPressed(BTN_UP) || PAD_justRepeated(BTN_UP)) {
					search_option -= 1;
					if (search_option<0) search_option = topts-1;
					// Keep the highlight inside the visible window.
					if (search_option<tools_start) tools_start = search_option;
					if (search_option==topts-1) tools_start = search_option-orows+1;
					if (tools_start<0) tools_start = 0;
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_DOWN) || PAD_justRepeated(BTN_DOWN)) {
					search_option += 1;
					if (search_option>=topts) search_option = 0;
					if (search_option>=tools_start+orows) tools_start = search_option-orows+1;
					if (search_option==0) tools_start = 0;
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_A)) {
					OptKind act_kind;
					Entry* act_tool = NULL;
					if (!MenuOpt_at(search_option, &act_kind, NULL, &act_tool)) {
						dirty = 1;
					}
					else if (act_kind==OPT_SEARCH) {
						show_search_menu = 1; // enter keyboard
						kb_text[0] = '\0';
						kb_col = 0;
						kb_row = 0;
						SearchResults_clear();
						search_selected = 0;
						search_start = 0;
					}
					else if (act_kind==OPT_SHUTDOWN) {
						// Shutdown: same path as SELECT+START (keymon) --
						// drop /tmp/poweroff and exit so MinUI.pak/launch.sh
						// runs led off-now + shutdown
						Search_close();
						putFile("/tmp/poweroff", "");
						quit = 1;
					}
					else {
						if (act_tool) {
							char tool_path[512];
							strcpy(tool_path, act_tool->path);
							Search_close();
							openPak(tool_path);
						}
					}
					dirty = 1;
				}
			}
			else if (show_search_menu==1) { // keyboard focus, results rebuild live
				if (PAD_justPressed(BTN_UP) || PAD_justRepeated(BTN_UP)) {
					kb_row -= 1;
					if (kb_row<0) kb_row = 3;
					if (kb_row<3 && kb_col>9) kb_col = 9;
					if (kb_row==3 && kb_col>2) kb_col = 2;
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_DOWN) || PAD_justRepeated(BTN_DOWN)) {
					kb_row += 1;
					if (kb_row>3) kb_row = 0;
					if (kb_row<3 && kb_col>9) kb_col = 9;
					if (kb_row==3 && kb_col>2) kb_col = 2;
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_LEFT) || PAD_justRepeated(BTN_LEFT)) {
					int cols = kb_row<3 ? 10 : 3;
					kb_col -= 1;
					if (kb_col<0) kb_col = cols-1;
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_RIGHT) || PAD_justRepeated(BTN_RIGHT)) {
					int cols = kb_row<3 ? 10 : 3;
					kb_col += 1;
					if (kb_col>=cols) kb_col = 0;
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_A)) {
					int len = strlen(kb_text);
					if (kb_row<3) {
						static const char* kb_rows_lower[3] = {"qwertyuiop","asdfghjkl_","zxcvbnm123"};
						static const char* kb_rows_upper[3] = {"QWERTYUIOP","ASDFGHJKL_","ZXCVBNM123"};
						int per_row[3] = {10,10,10};
						if (kb_col<per_row[kb_row] && len<255) {
							const char* row = kb_upper ? kb_rows_upper[kb_row] : kb_rows_lower[kb_row];
							kb_text[len] = row[kb_col];
							kb_text[len+1] = '\0';
							SearchResults_build(kb_text);
							search_selected = 0;
							search_start = 0;
						}
					}
					else {
						if (kb_col==0) { // SPACE
							if (len<255) {
								kb_text[len] = ' ';
								kb_text[len+1] = '\0';
								SearchResults_build(kb_text);
								search_selected = 0;
								search_start = 0;
							}
						}
						else if (kb_col==1) { // DEL
							if (len>0) {
								kb_text[len-1] = '\0';
								SearchResults_build(kb_text);
								search_selected = 0;
								search_start = 0;
							}
						}
						else if (kb_col==2) { // ABC/abc toggle
							kb_upper = !kb_upper;
						}
					}
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_L1) || PAD_justRepeated(BTN_L1)) {
					int len = strlen(kb_text); // DEL on shoulder too
					if (len>0) {
						kb_text[len-1] = '\0';
						SearchResults_build(kb_text);
						search_selected = 0;
						search_start = 0;
					}
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_R1)) { // SPACE on shoulder
					int len = strlen(kb_text);
					if (len<255) {
						kb_text[len] = ' ';
						kb_text[len+1] = '\0';
						SearchResults_build(kb_text);
						search_selected = 0;
						search_start = 0;
					}
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_START)) { // jump focus into results
					if (search_results && search_results->count>0) {
						show_search_menu = 2;
					}
					dirty = 1;
				}
			}
			else if (show_search_menu==2) { // results focus
				int rtotal = search_results ? search_results->count : 0;
				int srows = MAIN_ROW_COUNT + fancy_mode - 1; // query line takes one row
				if (srows<1) srows = 1;
				if (PAD_justPressed(BTN_UP) || PAD_justRepeated(BTN_UP)) {
					if (rtotal>0) {
						search_selected -= 1;
						if (search_selected<0) {
							// Wrap to bottom AND move window there so the
							// highlight never lands outside the visible page.
							search_selected = rtotal-1;
							search_start = rtotal-srows;
							if (search_start<0) search_start = 0;
						}
						else if (search_selected<search_start) search_start = search_selected;
					}
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_DOWN) || PAD_justRepeated(BTN_DOWN)) {
					if (rtotal>0) {
						search_selected += 1;
						if (search_selected>=rtotal) {
							// Wrap to top AND reset window to top.
							search_selected = 0;
							search_start = 0;
						}
						else if (search_selected>=search_start+srows) search_start = search_selected-srows+1;
					}
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_A)) {
					if (rtotal>0) {
						SearchResult* hit = search_results->items[search_selected];
						char hit_path[512];
						char hit_name[256];
						int hit_type = hit->type;
						strcpy(hit_path, hit->path);
						strcpy(hit_name, hit->name);
						Search_close();
						SearchResult_openAt(hit_path, hit_name, hit_type);
						total = top->entries->count;
						dirty = 1;
						if (total>0) readyResume(top->entries->items[top->selected]);
					}
				}
				else if (PAD_justPressed(BTN_Y)) { // back to keyboard
					show_search_menu = 1;
					dirty = 1;
				}
				else if (PAD_justPressed(BTN_START)) { // toggle favorite from results
					if (rtotal>0) {
						SearchResult* hit = search_results->items[search_selected];
						if (hit->type==ENTRY_ROM) {
							toggleFavorite(hit->path);
							refreshRootEntries();
						}
						dirty = 1;
					}
				}
			}
		}
		else if (show_version) {
			if (PAD_justPressed(BTN_B) || PAD_tappedMenu(now)) {
				show_version = 0;
				dirty = 1;
				if (!HAS_POWER_BUTTON) PWR_disableSleep();
			}
			// Fancy-only: no mode switching here anymore.
		}
		else {
			if (PAD_tappedMenu(now)) {
				show_version = 1;
				dirty = 1;
				if (!HAS_POWER_BUTTON) PWR_enableSleep();
			}
			else if (total>0) {
				if (PAD_justPressed(BTN_UP) || PAD_justRepeated(BTN_UP)) {
					itemnotchanged = 0;
					if (selected==0 && !PAD_justPressed(BTN_UP)) {
						// stop at top
					}
					else {
						selected -= 1;
						if (selected<0) {
							selected = total-1;
							int start = total - ( MAIN_ROW_COUNT + fancy_mode );
							top->start = (start<0) ? 0 : start;
							top->end = total;
						}
						else if (selected<top->start) {
							top->start -= 1;
							top->end -= 1;
						}
					}
				}
				else if (PAD_justPressed(BTN_DOWN) || PAD_justRepeated(BTN_DOWN)) {
					itemnotchanged = 0;
					if (selected==total-1 && !PAD_justPressed(BTN_DOWN)) {
						// stop at bottom
					}
					else {
						selected += 1;
						if (selected>=total) {
							selected = 0;
							top->start = 0;
							top->end = (total<( MAIN_ROW_COUNT + fancy_mode )) ? total : ( MAIN_ROW_COUNT + fancy_mode );
						}
						else if (selected>=top->end) {
							top->start += 1;
							top->end += 1;
						}
					}
				}
				if (PAD_justPressed(state_left)) {
					Entry *myentry = top->entries->items[top->selected];
					int ism3u = 0;
					if (myentry->type == ENTRY_DIR){
						//check if m3u
						char tmpname[256];
						char tmpname2[256];
						sprintf(tmpname, "%s/%s.m3u", myentry->path, myentry->name);
						ism3u = hasM3u(tmpname, tmpname2);
						//LOG_info("HASM3U: %d of %s\n", blb, myentry->path);
					}
					if ((myentry->type == ENTRY_ROM) || ism3u){
						if (can_resume) {
							//int curSaveIndex = getInt(slot_path);
							int curSaveIndex = last_selected_slot;
							curSaveIndex = curSaveIndex < 2  ? 8 : curSaveIndex-1;
							last_selected_slot = curSaveIndex;
							itemnotchanged = 1;
							//putInt(slot_path,curSaveIndex);
							dirty = 1;
						}
					}
					myentry=NULL;
				}
				if (PAD_justPressed(state_right)) {
					Entry *myentry = top->entries->items[top->selected];
					int ism3u = 0;
					if (myentry->type == ENTRY_DIR){
						//check if m3u
						char tmpname[256];
						char tmpname2[256];
						sprintf(tmpname, "%s/%s.m3u", myentry->path, myentry->name);
						ism3u = hasM3u(tmpname, tmpname2);
						//LOG_info("HASM3U: %d of %s\n", blb, myentry->path);
					}
					if ((myentry->type == ENTRY_ROM) || ism3u){
						if (can_resume) {
							//int curSaveIndex = getInt(slot_path);
							int curSaveIndex = last_selected_slot;
							curSaveIndex = curSaveIndex > 7 ? 1 : curSaveIndex+1;
							//putInt(slot_path,curSaveIndex);
							last_selected_slot = curSaveIndex;
							itemnotchanged = 1;
							dirty = 1;
						}
					}
					myentry = NULL;
				}
				if (PAD_justRepeated(page_seek_minus) || PAD_justPressed(page_seek_minus)) {
					itemnotchanged = 0;
					selected -= ( MAIN_ROW_COUNT + fancy_mode );
					if (selected<0) {
						selected = 0;
						top->start = 0;
						top->end = (total<( MAIN_ROW_COUNT + fancy_mode )) ? total : ( MAIN_ROW_COUNT + fancy_mode );
					}
					else if (selected<top->start) {
						top->start -= ( MAIN_ROW_COUNT + fancy_mode );
						if (top->start<0) top->start = 0;
						top->end = top->start + ( MAIN_ROW_COUNT + fancy_mode );
					}
				}
				else if (PAD_justRepeated(page_seek_plus) || PAD_justPressed(page_seek_plus)) {
					itemnotchanged = 0;
					selected += ( MAIN_ROW_COUNT + fancy_mode );
					if (selected>=total) {
						selected = total-1;
						int start = total - ( MAIN_ROW_COUNT + fancy_mode );
						top->start = (start<0) ? 0 : start;
						top->end = total;
					}
					else if (selected>=top->end) {
						top->end += ( MAIN_ROW_COUNT + fancy_mode );
						if (top->end>total) top->end = total;
						top->start = top->end - ( MAIN_ROW_COUNT + fancy_mode );
					}
				}
			}

			if ((PAD_justRepeated(BTN_L1) || PAD_justPressed(BTN_L1)) && !PAD_isPressed(BTN_R1) && !PWR_ignoreSettingInput(BTN_L1, show_setting)) { // previous alpha
				itemnotchanged = 0;
				if (total>0 && selected>=0 && selected<total) {
				Entry* entry = top->entries->items[selected];
				int i = entry->alpha-1;
				if (i>=0) {
					selected = top->alphas->items[i];
					if (total>( MAIN_ROW_COUNT + fancy_mode )) {
						top->start = selected;
						top->end = top->start + ( MAIN_ROW_COUNT + fancy_mode );
						if (top->end>total) top->end = total;
						top->start = top->end - ( MAIN_ROW_COUNT + fancy_mode );
					}
				}
				}
			}
			else if ((PAD_justRepeated(BTN_R1) || PAD_justPressed(BTN_R1)) && !PAD_isPressed(BTN_L1) && !PWR_ignoreSettingInput(BTN_R1, show_setting)) { // next alpha
				itemnotchanged = 0;
				if (total>0 && selected>=0 && selected<total) {
				Entry* entry = top->entries->items[selected];
				int i = entry->alpha+1;
				if (i<top->alphas->count) {
					selected = top->alphas->items[i];
					if (total>( MAIN_ROW_COUNT + fancy_mode )) {
						top->start = selected;
						top->end = top->start + ( MAIN_ROW_COUNT + fancy_mode );
						if (top->end>total) top->end = total;
						top->start = top->end - ( MAIN_ROW_COUNT + fancy_mode );
					}
				}
				}
			}

			if (selected!=top->selected) {
				top->selected = selected;
				dirty = 1;
			}

			if (dirty && total>0 && (!itemnotchanged)) readyResume(top->entries->items[top->selected]);

			if (PAD_justPressed(BTN_SELECT)){
				selected_modifier = 1;
				dirty = 1;
				//printf("\nisPressed SELECT\n");
			}
			if (PAD_justReleased(BTN_SELECT) || PAD_justReleasedShort(BTN_SELECT)){
				selected_modifier = 0;
				dirty = 1;
				//printf("\nisReleased SELECT\n");
			}

			if (total>0 && can_resume && (PAD_justReleasedShort(BTN_RESUME) || PAD_justReleased(BTN_RESUME))) {
				should_resume = 1;
				Entry_open(top->entries->items[top->selected]);
				dirty = 1;
			} else if (total>0 && PAD_justPressed(BTN_A)) {
				Entry_open(top->entries->items[top->selected]);
				total = top->entries->count;
				dirty = 1;

				if (total>0) readyResume(top->entries->items[top->selected]);
			}


			/*else if (total>0 && PAD_justPressed(BTN_SELECT)) {
				Entry* myentry = top->entries->items[top->selected];
				if (myentry->type == ENTRY_ROM) {
					toggleFavorite(myentry->path);
					quit = 1;
				}
			}*/

			else if (total>0 && PAD_justPressed(BTN_Y)) {
				if (!selected_modifier){
					Search_open();
					dirty = 1;
				}
				else {
				//SELECT pressed so toggle Hidden
					Entry* myentry = top->entries->items[top->selected];
					int ism3u = 0;
					if (myentry->type == ENTRY_DIR){
						//check if m3u
						char tmpname[256];
						char tmpname2[256];
						sprintf(tmpname, "%s/%s.m3u", myentry->path, myentry->name);
						ism3u = hasM3u(tmpname, tmpname2);
						//LOG_info("HASM3U: %d of %s\n", blb, myentry->path);
					}
					if ((myentry->type == ENTRY_ROM) || ism3u){
						toggleHidden(myentry->path);
						if (selected>0) {
							selected--;
						} else {
							selected=0;
						}
						dirty = 1;
						//quit = 1;
					}
				}
			}
			else if (total>0 && PAD_justReleasedShort(BTN_START)) {

				//toggle Favorites
				Entry* myentry = top->entries->items[top->selected];
				int ism3u = 0;
				if (myentry->type == ENTRY_DIR){
					//check if m3u
					char tmpname[256];
					//char tmpname2[256];
					char* tmpname2 = strrchr(myentry->path, '/') + 1; // get name
					sprintf(tmpname, "%s/%s.m3u", myentry->path, tmpname2 );
					ism3u = exists(tmpname);
				//	LOG_info("TOGGLE FAVORITES HASM3U: %d on %s but checked for %s\n", ism3u, myentry->path, tmpname);
				//	if (ism3u) {
				//		toggleFavorite(tmpname);
				//		quit = 1;
				//	}
				}
				if ((myentry->type == ENTRY_ROM ) || ism3u) {
					toggleFavorite(myentry->path);
					refreshRootEntries(); // show/hide the Favorites folder immediately
					dirty = 1;
				}

			} else if (PAD_justPressed(BTN_B) && stack->count>1) {
				closeDirectory();
				total = top->entries->count;
				dirty = 1;
				// can_resume = 0;
				if (total>0) readyResume(top->entries->items[top->selected]);
			}
		}

		if (dirty) {
			GFX_clear(screen);

			int ox;
			int oy;
			int ow = GFX_blitHardwareGroup(screen, show_setting, fancy_mode);

			if (show_search) {
				Search_draw(screen, show_setting, fancy_mode);
			}
			else if (show_version) {
				//if (!version) {
					char release[256];
					getFile(ROOT_SYSTEM_PATH "/version.txt", release, 256);

					char *tmp,*commit;
					commit = strrchr(release, '\n');
					commit[0] = '\0';
					commit = strrchr(release, '\n')+1;
					tmp = strchr(release, '\n');
					tmp[0] = '\0';

					// TODO: not sure if I want bare PLAT_* calls here
					char* extra_key = "Model";
					char* extra_val = PLAT_getModel();

					SDL_Surface* release_txt = TTF_RenderUTF8_Blended(font.large, "Release", COLOR_DARK_TEXT);
					SDL_Surface* version_txt = TTF_RenderUTF8_Blended(font.large, release, COLOR_WHITE);
					SDL_Surface* commit_txt = TTF_RenderUTF8_Blended(font.large, "Commit", COLOR_DARK_TEXT);
					SDL_Surface* hash_txt = TTF_RenderUTF8_Blended(font.large, commit, COLOR_WHITE);

					SDL_Surface* key_txt = TTF_RenderUTF8_Blended(font.large, extra_key, COLOR_DARK_TEXT);
					SDL_Surface* val_txt = TTF_RenderUTF8_Blended(font.large, extra_val, COLOR_WHITE);

					int l_width = 0;
					int r_width = 0;

					if (release_txt->w>l_width) l_width = release_txt->w;
					if (commit_txt->w>l_width) l_width = commit_txt->w;
					if (key_txt->w>l_width) l_width = commit_txt->w;

					if (version_txt->w>r_width) r_width = version_txt->w;
					if (hash_txt->w>r_width) r_width = hash_txt->w;
					if (val_txt->w>r_width) r_width = val_txt->w;

					#define VERSION_LINE_HEIGHT 24
					int x = l_width + SCALE1(8);
					int w = x + r_width;
					int h = SCALE1(VERSION_LINE_HEIGHT*(3+is_online));
					version = SDL_CreateRGBSurface(0,w,h,16,0,0,0,0);

					SDL_BlitSurface(release_txt, NULL, version, &(SDL_Rect){0, 0});
					SDL_BlitSurface(version_txt, NULL, version, &(SDL_Rect){x,0});
					SDL_BlitSurface(commit_txt, NULL, version, &(SDL_Rect){0,SCALE1(VERSION_LINE_HEIGHT)});
					SDL_BlitSurface(hash_txt, NULL, version, &(SDL_Rect){x,SCALE1(VERSION_LINE_HEIGHT)});

					SDL_BlitSurface(key_txt, NULL, version, &(SDL_Rect){0,SCALE1(VERSION_LINE_HEIGHT*2)});
					SDL_BlitSurface(val_txt, NULL, version, &(SDL_Rect){x,SCALE1(VERSION_LINE_HEIGHT*2)});

					SDL_FreeSurface(release_txt);
					SDL_FreeSurface(version_txt);
					SDL_FreeSurface(commit_txt);
					SDL_FreeSurface(hash_txt);
					SDL_FreeSurface(key_txt);
					SDL_FreeSurface(val_txt);

					if (is_online!=0) {
						//is connected, shows the IP Address
						SDL_Surface* ip_txt = TTF_RenderUTF8_Blended(font.large, "IP Addr", COLOR_DARK_TEXT);
						char * ip_addr = PLAT_getIPAddress();
						SDL_Surface* ip_addr_txt = TTF_RenderUTF8_Blended(font.large, ip_addr, COLOR_WHITE);
						SDL_BlitSurface(ip_txt, NULL, version, &(SDL_Rect){0,SCALE1(VERSION_LINE_HEIGHT*3)});
						SDL_BlitSurface(ip_addr_txt, NULL, version, &(SDL_Rect){x,SCALE1(VERSION_LINE_HEIGHT*3)});
						SDL_FreeSurface(ip_txt);
						SDL_FreeSurface(ip_addr_txt);
						free(ip_addr);
					}
				//}
				SDL_BlitSurface(version, NULL, screen, &(SDL_Rect){(screen->w-version->w)/2,(screen->h-version->h)/4});

				if (show_setting && !GetHDMI()) GFX_blitHardwareHints(screen, show_setting, fancy_mode);
				else GFX_blitButtonGroup((char*[]){ BTN_SLEEP==BTN_POWER?"PWR":"MENU",pwractionstr,  NULL }, 0, screen, 0, fancy_mode);

				GFX_blitButtonGroup((char*[]){ "B","BACK",  NULL }, 0, screen, 1, fancy_mode);
			}
			else {
				// list
				if (total>0) {
					int selected_row = top->selected - top->start;
						/*  start entry for boxart and save state preview window*/
					if (fancy_mode) {  //only when fancy_mode is active
						Entry* myentry = top->entries->items[top->selected];
						//if (myentry1->type == ENTRY_ROM) {
						// current filename path is entry->path

						char myslot_path[256];
						char myBoxart_path[256];
						char myslot_name[256];
						int myslotint;
						int ism3u = 0;
						//readyResume(entry);
						if (myentry->type == ENTRY_ROM){
							getStatePath(myentry->path,slot_path_rom);
							//sprintf(slot_path_rom, MYSAVESTATE_PATH "/MAME/States");
							getDisplayNameParens(myentry->path,myslot_name);
							//myslotint = getInt(slot_path);
							myslotint = last_selected_slot;
							if (myslotint){
								sprintf(myslot_path, "%s/%s.state%d.png",slot_path_rom, myslot_name, myslotint);
							} else {
								sprintf(myslot_path, "%s/%s.state.png",slot_path_rom,myslot_name);
							}
							//LOG_info("CIAO\n\n\n\n\n\n%s\n\n\n\n\n\n\n\n",myslot_name);
						}

						if (myentry->type == ENTRY_DIR){
							//check if it is a m3u present
						//	LOG_info("I'm inside the DIR %s, check if it an m3u\n", myentry->path);
							char m3upath[256];
							char tmpname[256];
							//char tmpname2[256];
							//sprintf(tmpname, "%s/%s.m3u", myentry->path, myentry->name);
							//int blb = hasM3u(tmpname, tmpname2);
							//LOG_info("HASM3U: %d of %s\n", blb, myentry->path);
							//tmpname[0] = '\0';
							getDisplayNameParens(myentry->path,tmpname);
							sprintf(m3upath, "%s/%s.m3u", myentry->path, tmpname);
							//LOG_info("tmpname: %s - m3upath %s\n", tmpname, m3upath);
							if (exists(m3upath)){
							//	LOG_info("It is a m3u\n");
								getStatePath(myentry->path,slot_path_rom);
								myslotint = last_selected_slot;
							//	LOG_info("myslotint %d - slot_path_rom %s\n", myslotint, slot_path_rom);
								if (myslotint){
									sprintf(myslot_path, "%s/%s.state%d.png",slot_path_rom, tmpname, myslotint);
								} else {
									sprintf(myslot_path, "%s/%s.state.png",slot_path_rom,tmpname);
								}
							//	LOG_info("myslotint %d - myslot_path %s\n", myslotint, myslot_path);fflush(stdout);
								ism3u = 1;
							}
						}


						// resolve the boxart path (roms, collated folders, paks and
						// faux/system dirs like Recently Played, Favorites, Collections, ...)
						int has_boxart = getBoxartPath(myentry, myBoxart_path);
						//LOG_info("%s - IMMAGINE = %s\n",myentry->name, myBoxart_path);
						/*
						printf("\n\nCurrent item name = %s\nCurrent Item path = %s\nCurrent Item Type = %d\nCurrent Item Save present = %d\nCurrent Item Last Save Slot = %d\nCurrent Item Slot bmp file = %s\nCurrent Item boxart Img = %s\n",
										myentry->name, myentry->path, myentry->type, can_resume, (can_resume) ? getInt(slot_path) : -1, myslot_path, myBoxart_path);
						fflush(stdout);
						*/
						// the boxart should be entry->path ../Imgs/entry->name.png

						//check if a save state should be painted
						int showstate = 0;
						int showboxart = 1;
						if (can_resume && ((myentry->type == ENTRY_ROM) || ism3u) && (!(hide_state))) {
							showstate = 1;
							if (hide_boxartifstate) {
								showboxart = 0;
							}
						}

						if (has_boxart && (showboxart)){
						// print the boxart from the cache; if it was not preloaded
						// yet decode it now (it gets cached for next time)
							SDL_Surface* boxart = BoxartCache_get(myBoxart_path);
							if (!boxart) {
								boxart = loadBoxart(myBoxart_path);
								if (boxart) BoxartCache_put(myBoxart_path, boxart);
							}
							drawBoxart(screen,boxart);
						}
						// end print boxart
						//print the state slot preview if present
						if (showstate) {
							drawStatePreview(screen, myslot_path, myslotint);
						}

						//}
						//myentry1 = NULL;
						GFX_blitHardwareGroup(screen, show_setting, fancy_mode);
						// the slot bmp should be in minui_path/EmuName/entry->name
						/* end for boxart and save state preview window, now print the text and the buttons */
				}

					for (int i=top->start,j=0; i<top->end; i++,j++) {
						Entry* entry = top->entries->items[i];
						char* entry_name = entry->name;
						char* entry_unique = entry->unique;

						int available_width = 0;
						TTF_Font *_font = font.large;
						SDL_Color text_color = COLOR_WHITE;
						available_width = screen->w  - SCALE1((PADDING - (PADDING*fancy_mode)) * 2);
						if (fancy_mode) {
							_font = font.medium;
							available_width = screen->w - ( screen->w  * 3 / 5);
							text_color = COLOR_GRAY;
						}

						if ((j==selected_row) && (fancy_mode)) {
							text_color = COLOR_WHITE;
							_font = font.large;
							available_width = screen->w  - SCALE1((PADDING - (PADDING*fancy_mode)) * 2);
						}
						if ((i==top->start) && !(fancy_mode) ) available_width -= ow;

						if (isFavorite(entry->path)) {
							text_color = COLOR_GOLD;
						}


						trimSortingMeta(&entry_name);

						char display_name[256];
						int text_width = GFX_truncateText(_font, entry_unique ? entry_unique : entry_name, display_name, available_width, SCALE1(BUTTON_PADDING*2));
						int max_width = MIN(available_width, text_width);
						if ((j==selected_row) && !(fancy_mode)) {
							GFX_blitPill(ASSET_WHITE_PILL, screen, &(SDL_Rect){
								SCALE1((PADDING - (PADDING*fancy_mode))),
								SCALE1((PADDING - (PADDING*fancy_mode))+(j*PILL_SIZE)),
								max_width,
								SCALE1(PILL_SIZE)
							});
							text_color = COLOR_BLACK;
						}	else if (entry->unique) {
							trimSortingMeta(&entry_unique);
							char unique_name[256];
							GFX_truncateText(_font, entry_unique, unique_name, available_width, SCALE1(BUTTON_PADDING*2));

							SDL_Surface* text = TTF_RenderUTF8_Blended(_font, unique_name, COLOR_DARK_TEXT);
							SDL_BlitSurface(text, &(SDL_Rect){
								0,
								0,
								max_width-SCALE1(BUTTON_PADDING*2),
								text->h
							}, screen, &(SDL_Rect){
								//SCALE1(PADDING+BUTTON_PADDING),
								//SCALE1(PADDING+(j*PILL_SIZE)+4)
								SCALE1((PADDING - (PADDING*fancy_mode))+BUTTON_PADDING-(PADDING*fancy_mode)),
								SCALE1((PADDING - (PADDING*fancy_mode))+(j*(PILL_SIZE-(5*fancy_mode)))+((PILL_SIZE-(5*fancy_mode))*fancy_mode)+4)
							});

							GFX_truncateText(_font, entry_name, display_name, available_width, SCALE1(BUTTON_PADDING*2));
						}
						SDL_Surface* text = TTF_RenderUTF8_Blended(_font, display_name, text_color);
						if ((j==selected_row) && (fancy_mode) && (entry->type == ENTRY_ROM)) {  //print the text twice, outline black and body white
							SDL_Surface* textout = TTF_RenderUTF8_Blended(font.largeoutline, display_name, COLOR_BLACK);
							SDL_BlitSurface(textout, &(SDL_Rect){
								0,
								0,
								max_width-SCALE1(BUTTON_PADDING*2),
								textout->h
							}, screen, &(SDL_Rect){
								SCALE1((PADDING - (PADDING*fancy_mode))+BUTTON_PADDING-(PADDING*fancy_mode)),
								SCALE1((PADDING - (PADDING*fancy_mode))+(j*(PILL_SIZE-(5*fancy_mode))+((PILL_SIZE-(5*fancy_mode))*fancy_mode)+4))
							});
							SDL_FreeSurface(textout);
						}
						SDL_BlitSurface(text, &(SDL_Rect){
							0,
							0,
							max_width-SCALE1(BUTTON_PADDING*2),
							text->h
						}, screen, &(SDL_Rect){
							SCALE1((PADDING - (PADDING*fancy_mode))+BUTTON_PADDING-(PADDING*fancy_mode)),
							SCALE1((PADDING - (PADDING*fancy_mode))+(j*(PILL_SIZE-(5*fancy_mode))+((PILL_SIZE-(5*fancy_mode))*fancy_mode)+4))
						});
						SDL_FreeSurface(text);
						}
						if (fancy_mode){
								//sprintf(tmpName,"                 ");
								Entry * entry =top->entries->items[top->selected];

								int ism3u = 0;
								if (entry->type == ENTRY_DIR){
									//check if m3u
									char tmpname[256];
									char tmpname2[256];
									sprintf(tmpname, "%s/%s.m3u", entry->path, entry->name);
									ism3u = hasM3u(tmpname, tmpname2);
									//LOG_info("HASM3U: %d of %s\n", blb, myentry->path);
								}

								if ((entry->type == ENTRY_ROM)|| ism3u) {
									getDisplayParentFolderName(entry->path, tmpName);
									//getEmuName(entry->path, tmpName);
								} else if (entry->type == ENTRY_PAK){
									sprintf(tmpName, "Tools");
								} else {
									sprintf(tmpName, "Main Menu");
								}
								SDL_Surface* title_txt = TTF_RenderUTF8_Blended(font.small, tmpName, COLOR_WHITE);
								SDL_BlitSurface(title_txt, NULL, screen, &(SDL_Rect){15, 0});
								SDL_FreeSurface(title_txt);
					}
				}
				else {
					// TODO: for some reason screen's dimensions end up being 0x0 in GFX_blitMessage...
					GFX_blitMessage(font.large, "Empty folder", screen, &(SDL_Rect){0,0,screen->w,screen->h}); //, NULL);
				}

				// buttons
				if (show_setting && !GetHDMI()) GFX_blitHardwareHints(screen, show_setting, fancy_mode);
				else if (can_resume) GFX_blitButtonGroup((char*[]){ "X","RSM", "Y","OPT", selected_modifier?"START":"Y",selected_modifier?"HIDE":"FAV",  NULL }, 0, screen, 0, fancy_mode);
				else {
					if (stack->count>1){
						GFX_blitButtonGroup((char*[]){
						BTN_SLEEP==BTN_POWER?"PWR":"MENU",
						BTN_SLEEP==BTN_POWER?"INFO":pwractionstr, "Y","OPT", selected_modifier?"START":"Y", (selected_modifier?"HIDE":"FAV"),
						NULL }, 0, screen, 0, fancy_mode);
					} else {
						GFX_blitButtonGroup((char*[]){
						BTN_SLEEP==BTN_POWER?"PWR":"MENU",
						BTN_SLEEP==BTN_POWER?"INFO":pwractionstr, "Y","OPT",
						NULL }, 0, screen, 0, fancy_mode);
					}
				}
				if (total==0) {
					if (stack->count>1) {
						GFX_blitButtonGroup((char*[]){ "B","BACK",  NULL }, 0, screen, 1, fancy_mode);
					}
				}
				else {
					if (stack->count>1) {
						GFX_blitButtonGroup((char*[]){ "B","BACK", "A",selected_modifier?"RUN2":"RUN", NULL }, 1, screen, 1, fancy_mode);
					}
					else {
						GFX_blitButtonGroup((char*[]){ "A","OPEN", NULL }, 0, screen, 1, fancy_mode);
					}
				}
			}

			GFX_flip(screen);
			GFX_pan();
			dirty = 0;
		}
		else {
			// idle: preload the boxart of the current directory (main thread,
			// one image per frame so input stays responsive)
			boxartPreload();
			GFX_sync();
		}
	}

	if (version) SDL_FreeSurface(version);

	Boxart_quit(); // free the boxart cache before exiting
	Menu_quit();
	PWR_quit();
	PAD_quit();
	GFX_quit();
	QuitSettings();
}
