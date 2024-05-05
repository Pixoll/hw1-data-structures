#include "moving_image.h"

#include <cstdlib>
#include <ctime>

using namespace std;

int main(const int argc, const char *argv[]) {
    srand(time(nullptr));

    moving_image im;

    // const int total = argc >= 2 ? atoi(argv[1]) : 100;
    // for (int i = 0; i < total; i++) {
    //     const IMAGE_ACTION_TYPE action = (IMAGE_ACTION_TYPE)(rand() % (REPEAT + 1));

    //     try {
    //         switch (action) {
    //             case MOVE_LEFT:               im.move_left(rand()); break;
    //             case MOVE_RIGHT:              im.move_right(rand()); break;
    //             case MOVE_UP:                 im.move_up(rand()); break;
    //             case MOVE_DOWN:               im.move_down(rand()); break;
    //             case ROTATE_COUNTERCLOCKWISE: im.rotate(); break;
    //             case UNDO:                    im.undo(); break;
    //             case REDO:                    im.redo(); break;
    //             case REPEAT:                  im.repeat(); break;
    //         }
    //     } catch (moving_image::exception e) {
    //         cerr << e.what() << endl;
    //         i--;
    //     }
    // }

    im.move_down(100);
    im.move_right(100);
    im.rotate();
    im.undo();
    im.undo();
    im.move_left(100);
    im.rotate();
    im.move_up(100);
    im.undo();
    im.redo();
    im.repeat();
    im.undo();
    im.move_up(100);
    im.undo();
    im.redo();
    im.move_down(100);
    im.move_right(100);
    im.move_left(100);
    im.repeat();
    im.undo();
    im.undo();
    im.undo();
    im.redo();
    im.rotate();
    im.undo();
    im.draw("1.png");
    im.redo();
    im.draw("2.png");

    im.repeat_all();

    return 0;
}
