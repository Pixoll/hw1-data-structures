#ifndef MOVING_IMG_H
#define MOVING_IMG_H

#include "basics.h"
#include "small_mario_rgb.h"

#include <deque>
#include <iostream>
#include <queue>
#include <stack>
#include <string>
#include <unistd.h>

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

const char *image_action_type_strings[APPLY_ALL] = {
    "move_left", "move_right", "move_up", "move_down", "rotate", "undo", "redo", "repeat", "undo_rotate",
};

int _id            = 0;
bool created_image = false;

class image_action {
  private:
    const int id = _id++;
    IMAGE_ACTION_TYPE type;
    IMAGE_ACTION_TYPE origin;
    int argument;

  public:
    image_action(IMAGE_ACTION_TYPE type, int argument, IMAGE_ACTION_TYPE origin) {
        this->type     = type;
        this->origin   = origin;
        this->argument = argument;
    }

    image_action(IMAGE_ACTION_TYPE type, int argument) : image_action(type, argument, type) {}

    ~image_action() {}

    IMAGE_ACTION_TYPE get_type() {
        return this->type;
    }

    int get_argument() {
        return this->argument;
    }

    string to_string() {
        string action_string = std::to_string(this->id) + ": " + image_action_type_strings[this->type];
        if (this->argument != 0)
            action_string += "(" + std::to_string(this->argument) + ")";

        if (this->type == this->origin)
            return action_string;

        return action_string + " - from " + image_action_type_strings[this->origin];
    }

    image_action *get_inverse() {
        IMAGE_ACTION_TYPE type;
        switch (this->type) {
            case MOVE_LEFT:               type = MOVE_RIGHT; break;
            case MOVE_RIGHT:              type = MOVE_LEFT; break;
            case MOVE_UP:                 type = MOVE_DOWN; break;
            case MOVE_DOWN:               type = MOVE_UP; break;
            case ROTATE_COUNTERCLOCKWISE: type = ROTATE_CLOCKWISE; break;
            case ROTATE_CLOCKWISE:        type = ROTATE_COUNTERCLOCKWISE; break;
        }

        return new image_action(type, this->argument, UNDO);
    }

    image_action *copy_with_origin(IMAGE_ACTION_TYPE origin) {
        return new image_action(this->type, this->argument, origin);
    }
};

// #include <chrono>

// class performance {
//   private:
//     typedef chrono::_V2::system_clock::time_point chrono_time_point;
//     chrono_time_point start;
//     chrono_time_point last;

//     static chrono_time_point now() {
//         return chrono::high_resolution_clock::now();
//     }

//   public:
//     performance() {
//         this->reset();
//     }

//     void reset() {
//         this->start = performance::now();
//         this->last  = this->start;
//     }

//     void log(const char *message = "performance") {
//         const float ms = performance::get_ms();
//         cout << message << ": " << ms << " ms" << endl;
//         this->last = performance::now();
//     }

//     void end(const char *message = "performance total") {
//         const float ms = performance::get_ms(true);
//         cout << message << ": " << ms << " ms" << endl;
//     }

//   private:
//     float get_ms(bool from_beginning = false) {
//         chrono_time_point end   = performance::now();
//         chrono_time_point begin = from_beginning ? this->start : this->last;
//         return chrono::duration_cast<chrono::nanoseconds>(end - begin).count() / 1000000.0f;
//     }
// };

string pad_zeroes(int n, int length) {
    string n_string   = to_string(n);
    const int str_len = n_string.length();
    while (length > str_len) {
        n_string = "0" + n_string;
        length--;
    }
    return n_string;
}

class moving_image {
  private:
    rgb **image;
    bool modified_image;
    int index_last_drawn;
    deque<image_action *> history;
    stack<image_action *> undone;
    queue<image_action *> redone;

  public:
    moving_image() {
        if (created_image)
            throw "Don't create a 2nd image";

        this->image = new rgb *[H_IMG];
        for (int i = 0; i < H_IMG; i++)
            this->image[i] = new rgb[W_IMG];

        this->index_last_drawn = -1;
        this->modified_image   = true;
        this->_reset_image();

        system("mkdir img");
        cout << "Draw output in ~/img" << endl;

        created_image = true;
    }

    ~moving_image() {
        for (int i = 0; i < H_IMG; i++)
            delete this->image[i];
        delete this->image;

        while (!this->history.empty()) {
            image_action *action = this->history.back();
            this->history.pop_back();
            action->~image_action();
        }
        this->history.shrink_to_fit();

        while (!this->undone.empty()) {
            image_action *action = this->undone.top();
            this->undone.pop();
            action->~image_action();
        }

        while (!this->redone.empty()) {
            image_action *action = this->redone.front();
            this->redone.pop();
            action->~image_action();
        }
    }

    class exception : public std::exception {
      private:
        const char *message;

      public:
        exception(const char *msg) : message(msg) {}

        ~exception() {}

        const char *what() const throw() {
            return this->message;
        }
    };

    void draw(const char *file_name) {
        // It's cheaty but it works
        deque<image_action *> backup_history = this->history;
        stack<image_action *> backup_undone  = this->undone;
        queue<image_action *> backup_redone  = this->redone;

        this->_save_action(APPLY_ALL);

        const int size = this->history.size();
        for (int i = this->index_last_drawn + 1; i < size; i++)
            this->_perform_action(this->history.at(i));

        this->index_last_drawn = size - 1;

        this->_draw(file_name);

        this->history = backup_history;
        this->undone  = backup_undone;
        this->redone  = backup_redone;
    }

    void move_left(int pixels) {
        pixels %= H_IMG;
        if (pixels < 0)
            pixels += H_IMG;

        this->_save_action(MOVE_LEFT, pixels);
    }

    void move_right(int pixels) {
        pixels %= H_IMG;
        if (pixels < 0)
            pixels += H_IMG;

        this->_save_action(MOVE_RIGHT, pixels);
    }

    void move_up(int pixels) {
        pixels %= W_IMG;
        if (pixels < 0)
            pixels += W_IMG;

        this->_save_action(MOVE_UP, pixels);
    }

    void move_down(int pixels) {
        pixels %= W_IMG;
        if (pixels < 0)
            pixels += W_IMG;

        this->_save_action(MOVE_DOWN, pixels);
    }

    void rotate() {
        this->_save_action(ROTATE_COUNTERCLOCKWISE);
    }

    void undo() {
        if (this->history.empty())
            throw moving_image::exception("There's nothing to undo.");

        this->_save_action(UNDO);
    }

    void redo() {
        if (this->undone.empty())
            throw moving_image::exception("There's nothing to redo.");

        this->_save_action(REDO);
    }

    void repeat() {
        if (this->history.empty() && this->redone.empty())
            throw moving_image::exception("There's nothing to repeat.");

        this->_save_action(REPEAT);
    }

    void repeat_all() {
        this->_save_action(APPLY_ALL);
        this->_reset_image();
        this->_print_history();

        const int size          = this->history.size();
        const int number_length = to_string(size).length();

        this->_draw("repeat_all_" + pad_zeroes(0, number_length) + ".png");

        for (int i = 0; i < size; i++) {
            this->_perform_action(this->history.at(i));
            this->_draw("repeat_all_" + pad_zeroes(i + 1, number_length) + ".png");
        }

        if (size > 0)
            this->_make_gif();

        return;
    }

  private:
    void _save_action(IMAGE_ACTION_TYPE type, int argument = 0) {
        if (type == UNDO) {
            this->undone.push(this->history.back());
            this->history.pop_back();
            return;
        }

        if (type == REDO) {
            this->redone.push(this->undone.top());
            this->undone.pop();
            return;
        }

        if (!this->redone.empty()) {
            queue<image_action *> copy = this->redone;
            stack<image_action *> inverses;

            while (!this->redone.empty()) {
                image_action *action = this->redone.front();
                inverses.push(action);
                this->history.push_back(action);
                this->redone.pop();
            }

            this->_apply_undone(inverses);

            while (!copy.empty()) {
                image_action *action = copy.front();
                this->history.push_back(action->copy_with_origin(REDO));
                copy.pop();
            }
        }

        image_action *repeated = type == REPEAT ? this->history.back() : nullptr;

        if (!this->undone.empty()) {
            stack<image_action *> inverses;
            this->_apply_undone(inverses);
        }

        if (type == APPLY_ALL)
            return;

        if (type == REPEAT) {
            this->history.push_back(repeated->copy_with_origin(REPEAT));
            return;
        }

        // move_x() or rotate()
        image_action *action = new image_action(type, argument);
        this->history.push_back(action);
    }

    void _apply_undone(stack<image_action *> inverses) {
        while (!this->undone.empty()) {
            image_action *action = this->undone.top();
            this->history.push_back(action);
            inverses.push(action);
            this->undone.pop();
        }

        while (!inverses.empty()) {
            image_action *action = inverses.top();
            this->history.push_back(action->get_inverse());
            inverses.pop();
        }
    }

    void _draw(const char *file_name) {
        string file_name_in_img = "img/";
        file_name_in_img += file_name;

        cout << "Drawing " << file_name << "..." << endl;

        byte_t *rgb = new byte_t[H_IMG * W_IMG * 3];
        byte_t *p   = rgb;

        FILE *fp = fopen(file_name_in_img.c_str(), "wb");

        for (int i = 0; i < H_IMG; i++)
            for (int j = 0; j < W_IMG; j++) {
                rgb_t pixel = this->image[i][j];
                *p++        = pixel.r;
                *p++        = pixel.g;
                *p++        = pixel.b;
            }

        svpng(fp, W_IMG, H_IMG, rgb, 0);
        fclose(fp);

        delete rgb;
    }

    void _draw(const string file_name) {
        this->_draw(file_name.c_str());
    }

    void _print_history() {
        cout << endl << "Action history:" << endl;
        if (this->history.empty()) {
            cout << "Empty" << endl << endl;
            return;
        }

        for (image_action *&action : this->history)
            cout << action->to_string() << endl;
        cout << endl;
    }

    // Block-Swap Algorithm
    // Programming Pearls, Second Edition by Jon Bentley
    // Vector Rotation section, page 54
    void _move(int pixels, bool horizontal) {
        if (pixels == 0)
            return;

        const int pivot_max      = horizontal ? H_IMG : W_IMG;
        const int opposite_pixel = (horizontal ? W_IMG : H_IMG) - pixels;

        for (int p = 0; p < pivot_max; p++) {
            int a = pixels;
            int b = opposite_pixel;
            while (a != b) {
                if (a > b) {
                    this->_block_swap(pixels - a, pixels, b, horizontal, p);
                    a -= b;
                } else {
                    this->_block_swap(pixels - a, pixels + b - a, a, horizontal, p);
                    b -= a;
                }
            }

            this->_block_swap(pixels - a, pixels, a, horizontal, p);
        }
    }

    void _swap_pixels(int i1, int j1, int i2, int j2) {
        const rgb temp      = this->image[i1][j1];
        this->image[i1][j1] = this->image[i2][j2];
        this->image[i2][j2] = temp;
    }

    void _block_swap(int fi, int si, int length, bool horizontal, int pivot) {
        if (horizontal) {
            for (int k = 0; k < length; k++)
                this->_swap_pixels(pivot, fi + k, pivot, si + k);
        } else {
            for (int k = 0; k < length; k++)
                this->_swap_pixels(fi + k, pivot, si + k, pivot);
        }
    }

    // Sadly transposing -> flipping cols/rows was slower by a 2-3x factor
    // Both that and the new one are O(n^2) tho
    void _rotate(bool clockwise) {
        if (clockwise) {
            for (int i = 0; i < H_IMG / 2; i++) {
                for (int j = i; j < W_IMG - i - 1; j++) {
                    const rgb temp                            = this->image[i][j];
                    this->image[i][j]                         = this->image[H_IMG - 1 - j][i];
                    this->image[H_IMG - 1 - j][i]             = this->image[H_IMG - 1 - i][W_IMG - 1 - j];
                    this->image[H_IMG - 1 - i][W_IMG - 1 - j] = this->image[j][W_IMG - 1 - i];
                    this->image[j][W_IMG - 1 - i]             = temp;
                }
            }
        } else {
            for (int i = 0; i < H_IMG / 2; i++) {
                for (int j = i; j < W_IMG - i - 1; j++) {
                    const rgb temp                            = this->image[i][j];
                    this->image[i][j]                         = this->image[j][W_IMG - 1 - i];
                    this->image[j][W_IMG - 1 - i]             = this->image[H_IMG - 1 - i][W_IMG - 1 - j];
                    this->image[H_IMG - 1 - i][W_IMG - 1 - j] = this->image[H_IMG - 1 - j][i];
                    this->image[H_IMG - 1 - j][i]             = temp;
                }
            }
        }
    }

    void _perform_action(image_action *action, bool inverse = false) {
        const IMAGE_ACTION_TYPE type = action->get_type();
        const int argument           = action->get_argument();

        switch (type) {
            case MOVE_LEFT:               this->_move(inverse ? W_IMG - argument : argument, true); break;
            case MOVE_RIGHT:              this->_move(inverse ? argument : W_IMG - argument, true); break;
            case MOVE_UP:                 this->_move(inverse ? H_IMG - argument : argument, false); break;
            case MOVE_DOWN:               this->_move(inverse ? argument : H_IMG - argument, false); break;
            case ROTATE_COUNTERCLOCKWISE: this->_rotate(inverse); break;
            case ROTATE_CLOCKWISE:        this->_rotate(!inverse); break;
        }

        this->modified_image = true;
    }

    void _reset_image() {
        if (!this->modified_image)
            return;

        for (int i = 0; i < H_IMG; i++)
            for (int j = 0; j < W_IMG; j++)
                this->image[i][j] = {DEFAULT_R, DEFAULT_G, DEFAULT_B};

        for (int i = 0; i < MARIO_H; i++)
            for (int j = 0; j < MARIO_W; j++)
                this->image[INIT_Y + i][INIT_X + j] = !s_R[i][j] && !s_G[i][j] && !s_B[i][j]
                                                          ? rgb{DEFAULT_R, DEFAULT_G, DEFAULT_B}
                                                          : rgb{(byte_t)s_R[i][j], (byte_t)s_G[i][j], (byte_t)s_B[i][j]};

        this->modified_image = false;
    }

    void _make_gif() {
        cout << endl;
        const string length = to_string(to_string(this->history.size()).length());
        const string ffmpeg = "ffmpeg -i img/repeat_all_%0" + length + "d.png img/repeat_all.gif";
        if (system(ffmpeg.c_str()) != 0) {
            cout << endl << "Install ffmpeg if you want a gif" << endl << endl;
        }
    }
};

#endif
