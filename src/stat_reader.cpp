/*
 * Usage:
 * Call with the paths of the files to be processed. The output is hardcoded to out.txt.
 *
 * */

#include <cstdlib>
#include <thread>
#include <mutex>
#include <iostream>
#include <fstream>
#include <string>
#include <zlib.h>
#include <map>
#include <exception>

#include "lightning.h"

#define NTHREADS 8

using namespace std;

std::mutex mtx;

// Thread-safe writing to an open file
void save_to_file(string s, string fn, std::ofstream* f) {
    std::lock_guard<std::mutex> file_lock (mtx);
    std::cout << "Saving " << fn << " data" << std::endl;
    *f << s;
}


void process_files( char* input[], int start, int end,
        string (*process)(gzFile,Cities*), std::ofstream* f,
        Cities * img) {
    for (int i = start; i < end; ++i) {
        //cout << "File " << input[i] << endl;
        gzFile file = gzopen(input[i], "rb");

        if (file == NULL) {
            std::cout << "Error opening " << input[i] << std::endl;
            return;
        }

        string s = (*process)(file, img);

        save_to_file(s, input[i], f);
    }
}

// Sakamoto's algorithm
int day_of_the_week(int y, int m, int d) {
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}


std::string get_dayly_data ( gzFile file, Cities* img ) {
    char line[150];
    std::string tmp, result;
    std::map<int, int> cities;
    string date;

    while ( !gzeof(file) ) {
        gzgets(file, line, 150);
        if (line == NULL) { break; }

        tmp = line;
        Lightning lightning(img, tmp);
        if (lightning.code == 16777215 || lightning.code == -1) { continue; }
        // Filter city
        if (lightning.code != 3550308 && lightning.code != 3304557) { continue; }
        if (lightning.quality == 2) { continue; }
        date = lightning.date;

        ++cities[lightning.code];
    }

    if (date.size() != 0) {
        int week_day = day_of_the_week(stoi(date.substr(0, 4)),  // Year
                                       stoi(date.substr(5, 2)),  // Month
                                       stoi(date.substr(8, 2))); // Day

        for (auto it = cities.begin(); it != cities.end(); ++it) {
            if (it->second > 10) {
                result += date + ", " + to_string(week_day) + ", " +
                          std::to_string(it->first) + ", " +
                          std::to_string(it->second) + "\n";
            }
        }
    } else {
        result = "";
    }

    return result;
}


int main(int argc, char* argv[]) {
    std::ofstream out_file;
    out_file.open("out.txt");
    Cities img("data/brasil_1km.png");

    if (argc == 1) {
        std::cout << "No files specified." << std::endl;
        return -1;
    }

    std::thread threads[NTHREADS];

    int size = argc - 1;
    int step = size / NTHREADS;

    for (int i = 0; i < NTHREADS; ++i) {
        int end = (i != NTHREADS - 1)? (i + 1) * step + 1 : argc;
        //cout << i << " : " << i * step + 1 << "-" << end - 1<< endl;
        threads[i] = std::thread(process_files, argv, i * step + 1,
                end, &get_dayly_data, &out_file, &img);
    }

    for (auto& th : threads) th.join();

    out_file.close();

    return 0;
}

