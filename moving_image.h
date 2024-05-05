#ifndef MOVING_IMG_H
#define MOVING_IMG_H

#include <deque>
#include <queue>
#include <stack>
#include <string>

using namespace std;

typedef unsigned char byte_t;

typedef struct rgb {
    byte_t r;
    byte_t g;
    byte_t b;
} rgb, rgb_t;

enum IMAGE_ACTION_TYPE {
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_DOWN,
    ROTATE_COUNTERCLOCKWISE,
    UNDO,
    REDO,
    REPEAT,
    ROTATE_CLOCKWISE,
    APPLY_ALL,
};
typedef enum IMAGE_ACTION_TYPE IMAGE_ACTION_TYPE;

extern int _id;

class image_action {
  private:
    const int id = _id++;
    IMAGE_ACTION_TYPE type;
    IMAGE_ACTION_TYPE origin;
    int argument;

  public:
    image_action(IMAGE_ACTION_TYPE type, int argument, IMAGE_ACTION_TYPE origin);
    image_action(IMAGE_ACTION_TYPE type, int argument);
    ~image_action();
    IMAGE_ACTION_TYPE get_type();
    int get_argument();
    string to_string();
    image_action *get_inverse();
    image_action *copy_with_origin(IMAGE_ACTION_TYPE origin);
};

class moving_image {
  private:
    rgb **image;
    bool modified_image;
    int index_last_drawn;
    deque<image_action *> history;
    stack<image_action *> undone;
    queue<image_action *> redone;

  public:
    moving_image();
    ~moving_image();

    class exception : public std::exception {
      private:
        const char *message;

      public:
        exception(const char *msg);
        ~exception();
        const char *what() const throw();
    };

    void draw(const char *file_name);
    void move_left(int pixels);
    void move_right(int pixels);
    void move_up(int pixels);
    void move_down(int pixels);
    void rotate();
    void undo();
    void redo();
    void repeat();
    void repeat_all();

  private:
    void _save_action(IMAGE_ACTION_TYPE type, int argument = 0);
    void _apply_undone(stack<image_action *> inverses);
    void _draw(const char *file_name);
    void _draw(const string file_name);
    void _print_history();
    void _move(int pixels, bool horizontal);
    void _swap_pixels(int i1, int j1, int i2, int j2);
    void _block_swap(int fi, int si, int length, bool horizontal, int pivot);
    void _rotate(bool clockwise);
    void _perform_action(image_action *action, bool inverse = false);
    void _reset_image();
    void _make_gif();
};

#endif
