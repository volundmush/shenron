//
// Created by volund on 8/12/22.
//

#include "shenron/utils.h"
#include "shenron/screen.h"
#include "shenron/legacy.h"
#include <random>
#include <fstream>
#include "effolkronium/random.hpp"
#include "spdlog/spdlog.h"


namespace shenron::utils {
    char *ANSI[] = {
            "@",
            AA_NORMAL,
            AA_NORMAL ANSISEPSTR AF_BLACK,
            AA_NORMAL ANSISEPSTR AF_BLUE,
            AA_NORMAL ANSISEPSTR AF_GREEN,
            AA_NORMAL ANSISEPSTR AF_CYAN,
            AA_NORMAL ANSISEPSTR AF_RED,
            AA_NORMAL ANSISEPSTR AF_MAGENTA,
            AA_NORMAL ANSISEPSTR AF_YELLOW,
            AA_NORMAL ANSISEPSTR AF_WHITE,
            AA_BOLD ANSISEPSTR AF_BLACK,
            AA_BOLD ANSISEPSTR AF_BLUE,
            AA_BOLD ANSISEPSTR AF_GREEN,
            AA_BOLD ANSISEPSTR AF_CYAN,
            AA_BOLD ANSISEPSTR AF_RED,
            AA_BOLD ANSISEPSTR AF_MAGENTA,
            AA_BOLD ANSISEPSTR AF_YELLOW,
            AA_BOLD ANSISEPSTR AF_WHITE,
            AB_BLACK,
            AB_BLUE,
            AB_GREEN,
            AB_CYAN,
            AB_RED,
            AB_MAGENTA,
            AB_YELLOW,
            AB_WHITE,
            AA_BLINK,
            AA_UNDERLINE,
            AA_BOLD,
            AA_REVERSE,
            "!"
    };

    const char CCODE[] = "@ndbgcrmywDBGCRMYW01234567luoex!";
/*
  Codes are:      @n - normal
  @d - black      @D - gray           @0 - background black
  @b - blue       @B - bright blue    @1 - background blue
  @g - green      @G - bright green   @2 - background green
  @c - cyan       @C - bright cyan    @3 - background cyan
  @r - red        @R - bright red     @4 - background red
  @m - magneta    @M - bright magneta @5 - background magneta
  @y - yellow     @Y - bright yellow  @6 - background yellow
  @w - white      @W - bright white   @7 - background white
  @x - random
Extra codes:      @l - blink          @o - bold
  @u - underline  @e - reverse video  @@ - single @

  @[num] - user color choice num, [] are required
*/
    const char RANDOM_COLORS[] = "bgcrmywBGCRMWY";

    static char *default_color_choices[NUM_COLOR+1] = {
/* COLOR_NORMAL */	AA_NORMAL,
/* COLOR_ROOMNAME */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_ROOMOBJS */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_ROOMPEOPLE */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_HITYOU */	AA_NORMAL ANSISEPSTR AF_RED,
/* COLOR_YOUHIT */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_OTHERHIT */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_CRITICAL */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_HOLLER */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_SHOUT */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_GOSSIP */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_AUCTION */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_CONGRAT */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_TELL */	AA_NORMAL ANSISEPSTR AF_RED,
/* COLOR_YOUSAY */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_ROOMSAY */	AA_NORMAL ANSISEPSTR AF_WHITE,
            NULL
    };


// A C++ version of proc_color from comm.c. it returns the colored string.
    std::string processColors(const std::string &txt, int parse, char **choices) {

        char *dest_char, *source_char, *color_char, *save_pos, *replacement = nullptr;
        int i, temp_color;
        size_t wanted;

        if (!txt.size() || !strchr(txt.c_str(), '@')) /* skip out if no color codes     */
            return txt;


        std::string out;
        source_char = (char *) txt.c_str();

        save_pos = dest_char;
        for (; *source_char;) {
            /* no color code - just copy */
            if (*source_char != '@') {
                out.push_back(*source_char++);
                continue;
            }

            /* if we get here we have a color code */

            source_char++; /* source_char now points to the code */

            /* look for a random color code picks a random number between 1 and 14 */
            if (*source_char == 'x') {
                temp_color = (rand() % 14);
                *source_char = RANDOM_COLORS[temp_color];
            }

            if (*source_char == '\0') { /* string was terminated with color code - just put it in */
                out.push_back('@');
                /* source_char will now point to '\0' in the for() check */
                continue;
            }

            if (!parse) { /* not parsing, just skip the code, unless it's @@ */
                if (*source_char == '@') {
                    out.push_back('@');
                }
                if (*source_char == '[') { /* Multi-puppet code */
                    source_char++;
                    while (*source_char && isdigit(*source_char))
                        source_char++;
                    if (!*source_char)
                        source_char--;
                }
                source_char++; /* skip to next (non-colorcode) char */
                continue;
            }

            /* parse the color code */
            if (*source_char == '[') { /* User configurable color */
                source_char++;
                if (*source_char) {
                    i = atoi(source_char);
                    if (i < 0 || i >= NUM_COLOR)
                        i = COLOR_NORMAL;
                    replacement = default_color_choices[i];
                    if (choices && choices[i])
                        replacement = choices[i];
                    while (*source_char && isdigit(*source_char))
                        source_char++;
                    if (!*source_char)
                        source_char--;
                }
            } else if (*source_char == 'n') {
                replacement = default_color_choices[COLOR_NORMAL];
                if (choices && choices[COLOR_NORMAL])
                    replacement = choices[COLOR_NORMAL];
            } else {
                for (i = 0; CCODE[i] != '!'; i++) { /* do we find it ? */
                    if ((*source_char) == CCODE[i]) {           /* if so :*/
                        replacement = ANSI[i];
                        break;
                    }
                }
            }
            if (replacement) {
                if (isdigit(replacement[0]))
                    for (color_char = ANSISTART; *color_char;)
                        out.push_back(*color_char++);
                for (color_char = replacement; *color_char;)
                    out.push_back(*color_char++);
                if (isdigit(replacement[0]))
                    out.push_back(ANSIEND);
                replacement = nullptr;
            }
            /* If we couldn't find any correct color code, or we found it and
             * substituted above, let's just process the nextcharacter.
             * - Welcor
             */
            source_char++;

        } /* for loop */

        return out;
    }
    
    nlohmann::json json_from_file(const std::filesystem::path &p) {
        if(!std::filesystem::exists(p) || !std::filesystem::is_regular_file(p)) return {};
        try {
            std::ifstream f(p);
            nlohmann::json j;
            f >> j;
            f.close();
            return j;
        }
        catch(nlohmann::detail::parse_error &e) {
            spdlog::error(e.what());
            return {};
        }
    }

    std::string random_string(std::size_t length)
    {
        const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        std::random_device random_device;
        std::mt19937 generator(random_device());
        std::uniform_int_distribution<> distribution(0, characters.size() - 1);

        std::string random_string;

        for (std::size_t i = 0; i < length; ++i)
        {
            random_string += characters[distribution(generator)];
        }

        return random_string;
    }

    std::string generate_id(const std::string &prf, std::size_t length, std::set<std::string> &existing) {
        auto generated = prf + "_" + random_string(length);
        while(existing.count(generated)) {
            generated = prf + "_" + random_string(length);
        }
        existing.insert(generated);
        return generated;
    }

/* creates a random number in interval [from;to] */
    int64_t random_number(int64_t from, int64_t to)
    {
        /* error checking in case people call this incorrectly */
        if (from > to) {
            int tmp = from;
            from = to;
            to = tmp;
        }
        return effolkronium::random_static::get(from, to);
    }



    std::string tabularTable(const std::list<std::string> &elem, int field_width, int line_length, const std::string &out_sep) {
        std::vector<std::string> processed;
        for(auto &e : elem) {
            auto diff = field_width - processColors(e, false, nullptr).size();
            if(diff > 0) processed.push_back(e + std::string(diff, ' '));
            else processed.push_back(e);
        }

        int separator_length = processColors(out_sep, false, nullptr).size();
        int per_line = line_length / (field_width + separator_length);

        std::stringstream out;

        int count = 0;
        int total = processed.size();
        for(int i = 0; i < total; i++) {
            count++;
            if(count == 1) out << processed[i];
            else if (count == per_line) {
                out << out_sep << processed[i] << std::endl;
                count = 0;
            }
            else if (count > 1) out << out_sep << processed[i];
        }
        return out.str();
    }

    Timer::Timer(long ms) {
        dur = ms;
    }

    void Timer::reset(long ms) {
        dur = ms;
    }

    void Timer::start() {
        start_time = std::chrono::steady_clock::now();
    }

    bool Timer::expired() {
        auto now = std::chrono::steady_clock::now();
        auto delta = now - start_time;
        return delta.count() >= dur;
    }

    long Timer::remaining() const {
        auto now = std::chrono::steady_clock::now();
        auto delta = now - start_time;
        return std::max(0L, dur - delta.count());
    }

}