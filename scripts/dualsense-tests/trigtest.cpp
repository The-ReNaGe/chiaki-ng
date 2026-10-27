// Sends adaptive trigger effects through SDL exactly like chiaki-ng's Controller::SetTriggerEffects
#include <SDL.h>
#include <cstdio>
#include <cstring>
typedef struct {
	Uint8 ucEnableBits1, ucEnableBits2, ucRumbleRight, ucRumbleLeft, ucHeadphoneVolume, ucSpeakerVolume,
	      ucMicrophoneVolume, ucAudioEnableBits, ucMicLightMode, ucAudioMuteBits;
	Uint8 rgucRightTriggerEffect[11], rgucLeftTriggerEffect[11], rgucUnknown1[6];
	Uint8 ucEnableBits3, rgucUnknown2[2], ucLedAnim, ucLedBrightness, ucPadLights, ucLedRed, ucLedGreen, ucLedBlue;
} DS5EffectsState_t;
static int send(SDL_GameController *c, const Uint8 *left, const Uint8 *right, const char *what) {
	DS5EffectsState_t s; SDL_zero(s);
	s.ucEnableBits1 |= 0x04 | 0x08;
	memcpy(s.rgucLeftTriggerEffect, left, 11);
	memcpy(s.rgucRightTriggerEffect, right, 11);
	int r = SDL_GameControllerSendEffect(c, &s, sizeof(s));
	printf("[%s] SendEffect -> %d %s\n", what, r, r < 0 ? SDL_GetError() : "");
	fflush(stdout);
	return r;
}
int main() {
	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1");
	if(SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) { printf("SDL_Init: %s\n", SDL_GetError()); return 1; }
	SDL_GameController *c = nullptr;
	for(int i = 0; i < SDL_NumJoysticks(); i++)
		if(SDL_IsGameController(i) && SDL_GameControllerTypeForIndex(i) == SDL_CONTROLLER_TYPE_PS5) { c = SDL_GameControllerOpen(i); break; }
	if(!c) { printf("No DualSense found by SDL\n"); return 1; }
	printf("Opened: %s path=%s\n", SDL_GameControllerName(c), SDL_GameControllerPath(c));
	const Uint8 off[11] = {0x05};
	const Uint8 rigid[11] = {0x01, 0x00, 0xFF};      // continuous resistance from the start, max force
	const Uint8 rigid_mid[11] = {0x01, 0x60, 0xC0};  // resistance starting mid-travel
	const Uint8 pulse[11] = {0x06, 0x0F, 0x8A, 0x1F}; // vibrating trigger
	SDL_Delay(3000);
	if(send(c, off, rigid, "R2 resistance") < 0) return 1;
	SDL_Delay(5000);
	send(c, rigid_mid, off, "L2 resistance mid-travel");
	SDL_Delay(5000);
	send(c, pulse, pulse, "both vibrating");
	SDL_Delay(5000);
	send(c, off, off, "off");
	SDL_Delay(300);
	SDL_GameControllerClose(c); SDL_Quit();
	return 0;
}
