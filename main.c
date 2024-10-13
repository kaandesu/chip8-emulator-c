#include "raylib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 4096
#define REGISTERS 16
#define STACK_SIZE 32
#define SCREEN_HEIGHT 32
#define SCREEN_WIDTH 64
#define MEMORY_OFFSET 0x200

#define SCALE 8.0f

typedef struct Emulator {
  uint8_t memory[MEMORY_SIZE];
  uint8_t registers[REGISTERS];
  uint16_t pc;
  uint16_t stack[STACK_SIZE];
  uint16_t I;
  int screen[SCREEN_WIDTH][SCREEN_HEIGHT];
} Emulator;

typedef struct FetchResponse {
  uint8_t b0;
  uint8_t b1;
} FetchResponse;

typedef struct DecodeResponse {
  uint8_t inst;
  uint8_t X;
  uint8_t Y;
  uint8_t N;
  uint8_t NN;
  uint16_t NNN;
} DecodeResponse;

Emulator *NewEmulator();
void CleanEmulator(Emulator *);
FetchResponse Fetch(Emulator *);
DecodeResponse Decode(Emulator *);
void Execute(Emulator *);
void draw(Emulator *);
void drawSprite(Emulator *, uint8_t VX, uint8_t VY, uint8_t N);
int LoadRom(Emulator *self, const char *filename);

Image image;
Texture2D texture;

int main(void) {
  const int screenWidth = SCREEN_WIDTH;
  const int screenHeight = SCREEN_HEIGHT;
  InitWindow(screenWidth * SCALE, screenHeight * SCALE, "chip8-emulator-c");

  image = GenImageColor(screenWidth, screenHeight, BLACK);
  texture = LoadTextureFromImage(image);

  Emulator *emulator = NewEmulator();

  if (LoadRom(emulator, "./demos/IBM Logo.ch8") == 0) {
    printf("ROM loaded!\n");
  } else {
    printf("Failed to load the ROM");
  }

  SetTargetFPS(60);
  while (!WindowShouldClose()) {

    BeginDrawing();

    Execute(emulator);
    UpdateTexture(texture, LoadImageColors(image));
    DrawTextureEx(texture, (Vector2){0.0f, 0.0f}, 0.0f, SCALE, WHITE);

    EndDrawing();
  }

  CleanEmulator(emulator);
  CloseWindow();

  return 0;
}

FetchResponse Fetch(Emulator *self) {
  FetchResponse fr;
  fr.b0 = self->memory[self->pc];
  fr.b1 = self->memory[self->pc + 1];
  // printf("Fetched [0x%02X 0x%02X] \n", fr.b0, fr.b1);
  self->pc += 2;
  return fr;
}

DecodeResponse Decode(Emulator *self) {
  DecodeResponse dr;
  FetchResponse fr = Fetch(self);
  dr.inst = (fr.b0 & 0xF0) >> 4;
  dr.X = (fr.b0 & 0x0F);
  dr.Y = (fr.b1 & 0xF0) >> 4;
  dr.N = (fr.b1 & 0x0F);
  dr.NN = fr.b1;
  dr.NNN = UINT16_C(dr.X) << 8 | UINT16_C(dr.NN);
  return dr;
}

void Execute(Emulator *self) {
  DecodeResponse dr = Decode(self);

  switch (dr.inst) {
  case 0x0:
    switch (dr.Y) {
    case 0xE:
      switch (dr.N) {
      case 0x0:
        for (int x = 0; x < SCREEN_HEIGHT; x++) {
          for (int y = 0; y < SCREEN_WIDTH; y++) {
            self->screen[x][y] = 0;
          }
        }
        break;
      }
      break;
    }
    break;

  case 0x1:
    self->pc = dr.NNN;
    break;

  case 0x6:
    self->registers[dr.X] = dr.NN;
    break;

  case 0x7:
    self->registers[dr.X] += dr.NN;
    break;

  case 0xA:
    self->I = dr.NNN;
    break;

  case 0xD:
    drawSprite(self, self->registers[dr.X], self->registers[dr.Y], dr.N);
    break;

  default:
    printf("Unknown instruction 0x%X\n", dr.inst);
    break;
  }
}

void draw(Emulator *emulator) {
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
      if (emulator->screen[x][y] > 0) {
        ImageDrawPixel(&image, (int)x, (int)y, WHITE);
      }
    }
  }
}

void drawSprite(Emulator *emulator, uint8_t VX, uint8_t VY, uint8_t N) {
  uint8_t x = VX % SCREEN_WIDTH;
  uint8_t y = VY % SCREEN_HEIGHT;
  emulator->registers[0xF] = 0;

  for (uint8_t row = 0; row < N; row++) {
    uint8_t spriteByte = emulator->memory[emulator->I + row];

    for (uint8_t col = 0; col < 8; col++) {
      for (uint8_t bit = 0; bit < 8; bit++) {
        if (col == bit && (spriteByte & (1 << (7 - bit)))) {
          uint8_t pixelX = (x + col) % UINT8_C(SCREEN_WIDTH);
          uint8_t pixelY = (y + row) % UINT8_C(SCREEN_HEIGHT);

          if (emulator->screen[pixelX][pixelY] == 1) {
            emulator->registers[0xF] = 1;
          }

          emulator->screen[pixelX][pixelY] ^= 1; // Flip pixel
        }
      }
    }
  }

  draw(emulator);
}

int LoadRom(Emulator *self, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (file == NULL) {
    perror("Error opening ROM FILE");
    return -1;
  }

  size_t bytesRead =
      fread(self->memory + MEMORY_OFFSET, 1, MEMORY_SIZE - MEMORY_OFFSET, file);

  if (bytesRead == 0) {
    printf("Error reading the ROM file, or ROM is empty\n");
    fclose(file);
    return -1;
  }

  fclose(file);

  return 0;
}

Emulator *NewEmulator() {
  Emulator *emulator = (Emulator *)malloc(sizeof(Emulator));
  if (emulator == NULL) {
    perror("Could not init emulator");
    exit(EXIT_FAILURE);
  }

  memset(emulator->memory, 0, sizeof(emulator->memory));
  memset(emulator->registers, 0, sizeof(emulator->registers));
  emulator->pc = MEMORY_OFFSET;
  emulator->I = -1;

  for (int x = 0; x < SCREEN_HEIGHT; x++) {
    for (int y = 0; y < SCREEN_WIDTH; y++) {
      emulator->screen[x][y] = 0;
    }
  }

  return emulator;
}

void CleanEmulator(Emulator *e) {
  free(e);
  UnloadImage(image);
  UnloadTexture(texture);
}
