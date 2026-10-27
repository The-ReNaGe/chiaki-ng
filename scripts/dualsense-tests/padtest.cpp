// Records DualSense touchpad input through SDL using the same conversion as chiaki-ng's
// Controller::HandleTouchpadEvent and the real libchiaki touch tracking, then draws it.
#include <SDL.h>
#include <chiaki/controller.h>
#include <cmath>
#include <cstdio>
#include <map>
#include <vector>
#include <utility>

#define MAXX 1919
#define MAXY 1079
#define COLS 64
#define ROWS 18
#define RECORD_MS 24000
static const char *phase_names[] = {"get ready", "LETTER (0-8 s after start: 3..8 s)", "SWIPES (8-16 s)", "TWO FINGERS + CLICK (16-24 s)"};
static int phase_of(Uint32 ms) { return ms < 3000 ? 0 : ms < 8000 ? 1 : ms < 16000 ? 2 : 3; }

struct Stroke { int phase; int chiaki_id; std::vector<std::pair<int,int>> pts; Uint32 t0, t1; };

int main() {
	SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
	if(SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) { printf("SDL_Init: %s\n", SDL_GetError()); return 1; }
	SDL_GameController *c = nullptr;
	for(int i = 0; i < SDL_NumJoysticks(); i++)
		if(SDL_IsGameController(i) && SDL_GameControllerTypeForIndex(i) == SDL_CONTROLLER_TYPE_PS5) { c = SDL_GameControllerOpen(i); break; }
	if(!c) { printf("No DualSense found by SDL\n"); return 1; }
	printf("Touchpads: %d, fingers: %d. Recording %d s...\n", SDL_GameControllerGetNumTouchpads(c),
		SDL_GameControllerGetNumTouchpadFingers(c, 0), RECORD_MS / 1000);
	fflush(stdout);

	ChiakiControllerState state; chiaki_controller_state_set_idle(&state);
	std::map<std::pair<Sint64,Sint64>, int> touch_ids;   // (touchpad, finger) -> stroke index
	std::vector<Stroke> strokes;
	int clicks = 0, max_fingers = 0, rejected = 0; int click_phase[4] = {0}; int fingers_phase[4] = {0};
	float raw_min_x = 2, raw_max_x = -1, raw_min_y = 2, raw_max_y = -1;
	Uint32 start = SDL_GetTicks();
	while(SDL_GetTicks() - start < RECORD_MS) {
		SDL_Event e;
		while(SDL_PollEvent(&e)) {
			if(e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.button == SDL_CONTROLLER_BUTTON_TOUCHPAD) { clicks++; click_phase[phase_of(SDL_GetTicks() - start)]++; }
			if(e.type != SDL_CONTROLLERTOUCHPADDOWN && e.type != SDL_CONTROLLERTOUCHPADMOTION && e.type != SDL_CONTROLLERTOUCHPADUP) continue;
			auto &t = e.ctouchpad;
			raw_min_x = fminf(raw_min_x, t.x); raw_max_x = fmaxf(raw_max_x, t.x);
			raw_min_y = fminf(raw_min_y, t.y); raw_max_y = fmaxf(raw_max_y, t.y);
			// same conversion as the patched Controller::HandleTouchpadEvent
			float nx = t.x < 0.0f ? 0.0f : (t.x > 1.0f ? 1.0f : t.x);
			float ny = t.y < 0.0f ? 0.0f : (t.y > 1.0f ? 1.0f : t.y);
			uint16_t x = (uint16_t)(nx * MAXX + 0.5f), y = (uint16_t)(ny * MAXY + 0.5f);
			auto key = std::make_pair((Sint64)t.touchpad, (Sint64)t.finger);
			auto it = touch_ids.find(key);
			if(e.type == SDL_CONTROLLERTOUCHPADDOWN && it == touch_ids.end()) {
				int8_t id = chiaki_controller_state_start_touch(&state, x, y);
				if(id < 0) { rejected++; continue; }
				strokes.push_back({phase_of(SDL_GetTicks() - start), id, {{x, y}}, e.ctouchpad.timestamp, e.ctouchpad.timestamp});
				touch_ids[key] = (int)strokes.size() - 1;
				if((int)touch_ids.size() > max_fingers) max_fingers = (int)touch_ids.size();
				{ int ph = phase_of(SDL_GetTicks() - start); if((int)touch_ids.size() > fingers_phase[ph]) fingers_phase[ph] = (int)touch_ids.size(); }
			} else if(it != touch_ids.end()) {
				Stroke &s = strokes[it->second];
				if(e.type == SDL_CONTROLLERTOUCHPADUP) {
					chiaki_controller_state_stop_touch(&state, (uint8_t)s.chiaki_id);
					s.t1 = e.ctouchpad.timestamp;
					touch_ids.erase(it);
				} else {
					chiaki_controller_state_set_touch_pos(&state, (uint8_t)s.chiaki_id, x, y);
					s.pts.push_back({x, y}); s.t1 = e.ctouchpad.timestamp;
				}
			}
		}
		SDL_Delay(2);
	}

	printf("SDL raw range x [%.3f, %.3f] y [%.3f, %.3f], rejected touches %d\n", raw_min_x, raw_max_x, raw_min_y, raw_max_y, rejected);
	for(int ph = 1; ph <= 3; ph++) {
		char grid[ROWS][COLS];
		for(auto &r : grid) for(auto &ch : r) ch = ' ';
		int n = 0;
		for(size_t i = 0; i < strokes.size(); i++) {
			if(strokes[i].phase != ph) continue;
			char mark = n < 9 ? (char)('1' + n) : (char)('a' + (n - 9) % 26);
			n++;
			auto &p = strokes[i].pts;
			for(size_t k = 0; k < p.size(); k++) {
				auto pa = p[k], pb = k + 1 < p.size() ? p[k + 1] : p[k];
				for(int st = 0; st <= 8; st++) {
					double fx = pa.first + (pb.first - pa.first) * st / 8.0, fy = pa.second + (pb.second - pa.second) * st / 8.0;
					int col = (int)(fx * (COLS - 1) / MAXX + 0.5), row = (int)(fy * (ROWS - 1) / MAXY + 0.5);
					grid[row][col] = mark;
				}
			}
			// mark the start point so the drawing direction is visible
			auto s0 = p.front();
			grid[(int)(s0.second * (ROWS - 1) / MAXY + 0.5)][(int)(s0.first * (COLS - 1) / MAXX + 0.5)] = '@';
		}
		printf("\n=== %s: %d strokes, max fingers %d, clicks %d ('@' = where a stroke starts)\n", phase_names[ph], n, fingers_phase[ph], click_phase[ph]);
		printf("+"); for(int i = 0; i < COLS; i++) putchar('-'); printf("+\n");
		for(auto &r : grid) { putchar('|'); fwrite(r, 1, COLS, stdout); printf("|\n"); }
		printf("+"); for(int i = 0; i < COLS; i++) putchar('-'); printf("+\n");
		n = 0;
		for(size_t i = 0; i < strokes.size(); i++) {
			auto &st = strokes[i];
			if(st.phase != ph) continue;
			char mark = n < 9 ? (char)('1' + n) : (char)('a' + (n - 9) % 26);
			n++;
			auto pa = st.pts.front(), pb = st.pts.back();
			int dx = pb.first - pa.first, dy = pb.second - pa.second;
			int minx = MAXX, maxx = 0, miny = MAXY, maxy = 0;
			double path = 0;
			for(size_t k = 0; k < st.pts.size(); k++) {
				auto &q = st.pts[k];
				minx = std::min(minx, q.first); maxx = std::max(maxx, q.first); miny = std::min(miny, q.second); maxy = std::max(maxy, q.second);
				if(k) path += std::hypot(q.first - st.pts[k-1].first, q.second - st.pts[k-1].second);
			}
			double straight = std::hypot(dx, dy);
			const char *kind;
			if(path < 150) kind = "tap / hold";
			else if(straight < 0.7 * path) kind = "shape (curves or corners)";
			else if(abs(dx) > 2 * abs(dy)) kind = dx > 0 ? "swipe right" : "swipe left";
			else if(abs(dy) > 2 * abs(dx)) kind = dy > 0 ? "swipe down" : "swipe up";
			else kind = "diagonal swipe";
			printf("  %c: id %3d, %4zu pts, %4u ms, path %5.0f, (%4d,%4d)->(%4d,%4d), box x[%d..%d] y[%d..%d] -> %s\n",
				mark, st.chiaki_id, st.pts.size(), st.t1 - st.t0, path, pa.first, pa.second, pb.first, pb.second, minx, maxx, miny, maxy, kind);
		}
	}
	SDL_GameControllerClose(c); SDL_Quit();
	return 0;
}
