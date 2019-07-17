#define RAPIDJSON_SSE42

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"
#include <iostream>
#include "zstr.hpp"
#include <chrono>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;
using namespace std::chrono;
using namespace rapidjson;

class Handler {
    protected:
        unsigned long int lines;
        unsigned long int depth;
        bool id64FieldNext;
        high_resolution_clock::time_point t1,t2;
    public:
        Handler() {
            this->lines = 1;
            this->depth = 0;
            this->id64FieldNext = false;
            this->t1 = high_resolution_clock::now();
        }
        ~Handler() {
        }
        bool Key(const char* str, SizeType length, bool copy) {
            (void)copy; (void)length;
            if (this->depth == 2 && strncmp(str,"id64",4) == 0) {
                this->id64FieldNext = true;
            } else {
                this->id64FieldNext = false;
            }
            return true;
        }
        bool Null() {
            return true;
        }
        bool Bool(bool b) {
            (void) b;
            return true;
        }
        bool Int(int i) {
            (void) i;
            return true;
        }
        bool Uint(unsigned u) {
            (void) u;
            return true;
        }
        bool Int64(int64_t i) {
            (void) i;
            return true;
        }
        bool Uint64(uint64_t u) {
            (void) u;
            return true;
        }
        bool Double(double d) {
            (void) d;
            return true;
        }
        bool RawNumber(const char* str, SizeType length, bool copy) {
            (void) length; (void) copy;
            if (this->id64FieldNext) {
                unsigned long int lastId64 = strtoul(str,NULL,0);
                printf("%lX\n", lastId64);
            }
            return true;
        }
        bool String(const char* str, SizeType length, bool copy) {
            (void) str; (void) length; (void) copy;
            return true;
        }
        bool StartObject() {
            this->depth++;
            return true;
        }
        bool StartArray() {
            this->depth++;
            return true;
        }
        bool EndObject(SizeType memberCount) {
            (void)memberCount;
            this->depth--;
            if (this->depth == 1) {
                this->lines++;
                if (this->lines % 5000000 == 0) {
                    this->t2 = high_resolution_clock::now();
                    duration<double> total_time_span = duration_cast<duration<double>>(t2 - t1);
                    cerr << "Read line " << this->lines << " in " << total_time_span.count() << " seconds at " << (this->lines / total_time_span.count()) << " lines/second" << endl;
                }
            }
            return true;
        }
        bool EndArray(SizeType elementCount) {
            (void)elementCount;
            this->depth--;
            return true;
        }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file to parse>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    string filename = argv[1];

    struct stat buffer;
    if (stat (filename.c_str(), &buffer) != 0) {
        cerr << "File " << filename << " does not exist." << endl;
        exit(1);
    }

    Reader reader;
    zstr::ifstream* ifs;
    IStreamWrapper* stream;
    Handler handler;

    ifs = new zstr::ifstream(filename);
    stream = new IStreamWrapper(*ifs);
    reader.IterativeParseInit();

    reader.Parse<kParseNumbersAsStringsFlag>(*stream, handler);
    if (reader.HasParseError()) {
        cerr << "Parse error in " <<
            filename << " [" <<
            GetParseError_En(reader.GetParseErrorCode()) <<
            "] offset [" <<
            reader.GetErrorOffset() <<
            endl;
        throw "Parse error in systems";
    }

    delete stream;
    delete ifs;
}
