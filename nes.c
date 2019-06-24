#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <6502.h>
#include "mem.h"
#include "ppu.h"

extern u32 readfile(const char* fname, u8* mem, u32 ramsize);
void *nes(const char *rom);

#define NOLCD 0

#if (NOLCD)
#else
#include <pthread.h>
#include <GLFW/glfw3.h>
#define AUTO_REFRESH 60
#define OFFSET 64
#define WIDTH 256
#define HEIGHT 256

static GLFWwindow* window;
u8 gl_ok=0;
u8 flip_y=1;
u8 new_frame=1;
u8 limit_speed=0;
u32 cyclelimit = 0;
u8 debugmode = 0;

double t0; // global start time
double get_time() {
  struct timeval tv; gettimeofday(&tv, NULL);
  return (tv.tv_sec + tv.tv_usec * 1e-6);
}

#define bind_key(x,y) \
{ if (action == GLFW_PRESS && key == (x)) (y) = 1; if (action == GLFW_RELEASE && key == (x)) (y) = 0; if (y) {printf(#y "\n");} }

static void error_callback(s32 error, const char* description) { }
static void key_callback(GLFWwindow* window, s32 key, s32 scancode, s32 action, s32 mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_0) show_debug ^= 1;
    if (action == GLFW_PRESS && key == GLFW_KEY_9) limit_speed ^= 1;
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
}
static GLFWwindow* open_window(const char* title, GLFWwindow* share, s32 posX, s32 posY)
{
    GLFWwindow* window;

    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, title, NULL, share);
    if (!window) return NULL;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowPos(window, posX, posY);
    glfwShowWindow(window);

    glfwSetKeyCallback(window, key_callback);

    return window;
}

static void draw_quad()
{
    s32 width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float incr_x = 1.0f/(float)IM_W; float incr_y = 1.0f/(float)IM_H;
    glOrtho(0.f, 1.f, 0.f, 1.f, 0.f, 1.f);
    glBegin(GL_QUADS);
    float i,j;
    u8 px,py;
    for (u16 x=0; x<IM_W; x++) for (u16 y=0; y<IM_H; y++ ) {
         i = x * incr_x; j = y * incr_y; px = x; py = flip_y ? IM_H - y - 1 : y; // FLIP vert

          glColor4f(pix[3*(px+py*IM_W)+0]/255.0f,
                    pix[3*(px+py*IM_W)+1]/255.0f,
                    pix[3*(px+py*IM_W)+2]/255.0f,
                    255.0f/255.0f);

          glVertex2f(i,      j     );     glVertex2f(i+incr_x, j     );
          glVertex2f(i+incr_x, j+incr_y); glVertex2f(i,      j+incr_y);
    }
    glEnd();
};

s32 lcd_init() {
  s32 x, y, width;
  //glfwSetErrorCallback(error_callback);
  if (!glfwInit()) return -1;

  window = open_window("NES", NULL, OFFSET, OFFSET);
  if (!window) { glfwTerminate(); return -1; }

  glfwGetWindowPos(window, &x, &y);
  glfwGetWindowSize(window, &width, NULL);
  glfwMakeContextCurrent(window);

  gl_ok=1;
  printf("%9.6f, GL_init OK\n", get_time()-t0);

  double frame_start=get_time();
  while (!glfwWindowShouldClose(window))
  {
    double fps = 1.0/(get_time()-frame_start);
    char wtitle[256]; wtitle[255] = '\0';
    frame_start = get_time();
    sprintf(wtitle, "%4.1f FPS", fps);
    glfwSetWindowTitle(window, wtitle);
    glfwMakeContextCurrent(window);
    glClear(GL_COLOR_BUFFER_BIT);
    draw_quad();
    glfwSwapBuffers(window);
    if (AUTO_REFRESH > 0) glfwWaitEventsTimeout(1.0/(double)AUTO_REFRESH - (get_time()-frame_start));
    else glfwWaitEvents();
  }

  glfwTerminate();
  printf("%9.6f, GL terminating\n", get_time()-t0);
  gl_ok=0;
  return 0;
}
#endif

int main(int argc, char **argv) {

  if (argc < 2)  {
    printf("usage: %s <rom> [debug=0] [cyclelimit=0]\n" , argv[0]);
    return -1;
  };

  if (argc >= 3) debugmode  = atoi(argv[2]);
  if (argc >= 4) cyclelimit = atoi(argv[3]);

  printf("rom: %s debug: %d cyclelimit: %d\n", argv[1], debugmode, cyclelimit);

  #if NOLCD
    has_bcd=0;
    nes(argv[1]);
  #else
    pthread_t nes_thread;
    if(pthread_create(&nes_thread, NULL, nes, argv[1])) {
      fprintf(stderr, "Error creating thread\n");
      return 1;
    }

    lcd_init();

    if(pthread_join(nes_thread, NULL)) {
      fprintf(stderr, "Error joining thread\n");
      return 2;
    }

  #endif

  return 0;
}

void *nes(const char *rom) {

  u16 load_at = 0xbff0;
  memset(mem,0x0,0x10000);
  u8 tmp[0x10000];

  read_ines(rom);

  while (!gl_ok) usleep(10);
  //if (!readfile((const char*)rom, tmp, 0x100000)) { printf("couldn't read %s\n", rom); } else
  {
    memcpy(mem+load_at, tmp, 0x10000-load_at);
    reset(0xc000);

    u32 t = 0;
    show_debug = debugmode;
    prev_cyc=0;
    while (gl_ok && (t<cyclelimit || cyclelimit<=0)) {
      cpu_step(1);
      ppu_step(3*(cyc - prev_cyc));
      prev_cyc=cyc;
      t++;
    }

  }
  return 0;
}
