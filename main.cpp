/*
 * Copyright (c) 2009 Tias Guns
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*#include "main_common.hpp"
#include "gui/gui_x11.cpp"
*/

#include <unistd.h>
#include <cstdio>

#include "gui_x11.hpp"
#include "calibration.hpp"

int main(int argc, char** argv)
{

    GuiCalibratorX11 gui;
    Calibrator  calib;

    gui.set_add_click([&](int x, int y) -> bool{
        return calib.add_click(x, y);
    });
    gui.set_reset([&](){
        return calib.reset();
    });


    // wait for timer signal, processes events
    auto ret = gui.mainloop();

    if (ret) {
        printf("Values:\n");
        for( auto [x, y] : gui.get_points()) {
            printf("  x=%d, y=%d\n", x, y);
        }
    } else {
        printf("No results\n");
    }

    auto [w, h] = gui.get_display_size();

    calib.finish(w, h);

    calib.save_calibration();
    calib.output_xinput();
    calib.output_xorgconfd();

    //delete calibrator;
    return 0;
}
