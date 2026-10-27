// Measures how far each touchpad edge is reached, raw from SDL and after chiaki-ng's conversion
#include <SDL.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <utility>

#define MAXX 1919
#define MAXY 1079
#define COLS 64
#define ROWS 18
#define RECORD_MS 15000

int main() {
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	if(SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) { printf("SDL_Init: %s\n", SDL_GetError()); return 1; }
	SDL_GameController *c = nullptr;
	for(int i = 0; i < SDL_NumJoysticks(); i++)
		if(SDL_IsGameController(i) && SDL_GameControllerTypeForIndex(i) == SDL_CONTROLLER_TYPE_PS5) { c = SDL_GameControllerOpen(i); break; }
	if(!c) { printf("No DualSense found by SDL\n"); return 1; }
	printf("Recording %d s: trace slowly along all four edges...\n", RECORD_MS / 1000);
	fflush(stdout);

	std::vector<std::pair<int,int>> pts;
	float rmin_x = 9, rmax_x = -9, rmin_y = 9, rmax_y = -9;
	int below0 = 0, above1 = 0, events = 0;
	Uint32 start = SDL_GetTicks();
	while(SDL_GetTicks() - start < RECORD_MS) {
		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			if(e.type != SDL_CONTROLLERTOUCHPADDOWN && e.type != SDL_CONTROLLERTOUCHPADMOTION) continue;
			auto &t = e.ctouchpad;
			events++;
			rmin_x = fminf(rmin_x, t.x); rmax_x = fmaxf(rmax_x, t.x);
			rmin_y = fminf(rmin_y, t.y); rmax_y = fmaxf(rmax_y, t.y);
			if(t.x < 0 || t.y < 0) below0++;
			if(t.x > 1 || t.y > 1) above1++;
			// same conversion as the patched Controller::HandleTouchpadEvent
			float nx = t.x < 0.0f ? 0.0f : (t.x > 1.0f ? 1.0f : t.x);
			float ny = t.y < 0.0f ? 0.0f : (t.y > 1.0f ? 1.0f : t.y);
			pts.push_back({(int)(uint16_t)(nx * MAXX + 0.5f), (int)(uint16_t)(ny * MAXY + 0.5f)});
		}
		SDL_Delay(2);
	}
	if(pts.empty()) { printf("No touch recorded\n"); return 0; }

	char grid[ROWS][COLS];
	for(auto &r : grid) for(auto &ch : r) ch = ' ';
	int minx = MAXX, maxx = 0, miny = MAXY, maxy = 0;
	for(auto &p : pts) {
		minx = std::min(minx, p.first); maxx = std::max(maxx, p.first);
		miny = std::min(miny, p.second); maxy = std::max(maxy, p.second);
		grid[(int)(p.second * (ROWS - 1) / (double)MAXY + 0.5)][(int)(p.first * (COLS - 1) / (double)MAXX + 0.5)] = '#';
	}
	printf("\n+"); for(int i = 0; i < COLS; i++) putchar('-'); printf("+\n");
	for(auto &r : grid) { putchar('|'); fwrite(r, 1, COLS, stdout); printf("|\n"); }
	printf("+"); for(int i = 0; i < COLS; i++) putchar('-'); printf("+\n\n");

	printf("%d touch events\n", events);
	printf("SDL raw      : x [%.4f .. %.4f]   y [%.4f .. %.4f]   (expected 0..1)\n", rmin_x, rmax_x, rmin_y, rmax_y);
	printf("out of range : %d samples < 0, %d samples > 1 (clamped by chiaki-ng)\n", below0, above1);
	printf("sent to PS5  : x [%d .. %d] of 0..%d   y [%d .. %d] of 0..%d\n", minx, maxx, MAXX, miny, maxy, MAXY);
	printf("coverage     : left %.1f%%  right %.1f%%  top %.1f%%  bottom %.1f%% of the way to each edge\n",
		100.0 * (1.0 - minx / (double)MAXX), 100.0 * maxx / MAXX, 100.0 * (1.0 - miny / (double)MAXY), 100.0 * maxy / MAXY);
	SDL_GameControllerClose(c); SDL_Quit();
	return 0;
}
